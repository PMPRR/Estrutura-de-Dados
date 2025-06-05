// DataReceiver.cpp
#include "network/data_receiver.h"
#include <iostream>
#include <iomanip>   // For std::hex, std::setw, std::setfill
#include <algorithm> // For std::min
#include <cstring>   // For strlen, strncmp
#include <cctype>    // For isprint
#include <vector>    // For std::vector
#include <zmq.h>     // Include C API for ZMQ_ constants if zmq::sockopt is not available

// Constructor
DataReceiver::DataReceiver(const std::string& publisher_address,
                           const std::string& zmq_topic_filter,
                           const std::string& data_prefix_to_process)
    : context_(1), 
      subscriber_socket_(context_, ZMQ_SUB), // Use C ZMQ_SUB macro
      publisher_address_(publisher_address),
      zmq_topic_filter_(zmq_topic_filter),
      data_prefix_to_process_(data_prefix_to_process),
      running_(false),
      successfully_started_(false) {
    if (data_prefix_to_process_.empty() && !zmq_topic_filter_.empty()) {
        data_prefix_to_process_ = zmq_topic_filter_;
    }
}

// Destructor
DataReceiver::~DataReceiver() {
    stop(); 
    join(); 

    try {
        // Check if the socket object itself is valid (often overloaded operator bool)
        // or if it was successfully started, implying it was valid at some point.
        if (successfully_started_.load()) { 
            // Unsubscribe. Using the original filter string.
            // The ZMQ_UNSUBSCRIBE option takes the topic as a char* and its length.
            if (!zmq_topic_filter_.empty()) {
                 subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
            } else {
                // To unsubscribe from all when subscribed with empty topic,
                // you might need to close or it might be implicit.
                // For safety, if an empty topic was used for subscribe, an explicit unsubscribe with empty string is fine.
                subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, "", 0);
            }

            // Check if connected before trying to disconnect (if connected() method exists)
            // If not, this might throw if already closed or invalid, so wrap it.
            try {
                if (!publisher_address_.empty()) { // Check if we actually connected to an address
                    // subscriber_socket_.disconnect(publisher_address_); // disconnect might not be needed if close is called
                }
            } catch (const zmq::error_t& e) {
                // std::cerr << "Ignoring error during disconnect: " << e.what() << std::endl;
            }
        }
        // Close the socket. This is generally safe to call even if already closed.
        subscriber_socket_.close();
        context_.close(); // Close context
    } catch (const zmq::error_t& e) {
        std::cerr << "Exception during DataReceiver cleanup: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception during DataReceiver cleanup: " << e.what() << std::endl;
    }
}

// Starts the receiving loop
bool DataReceiver::start() {
    if (running_.load()) {
        std::cout << "DataReceiver is already running." << std::endl;
        return true; 
    }

    try {
        std::cout << "DataReceiver connecting to " << publisher_address_ << "..." << std::endl;
        subscriber_socket_.connect(publisher_address_);
        std::cout << "DataReceiver connected." << std::endl;

        std::cout << "DataReceiver subscribing with ZMQ filter: '" << (zmq_topic_filter_.empty() ? "<ALL MESSAGES>" : zmq_topic_filter_) << "'" << std::endl;
        // Use setsockopt with C-style ZMQ_SUBSCRIBE
        // It takes a pointer to the topic string and its length.
        subscriber_socket_.setsockopt(ZMQ_SUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
        
        int rcvtimeo_ms = 100; // 100 ms timeout
        subscriber_socket_.setsockopt(ZMQ_RCVTIMEO, &rcvtimeo_ms, sizeof(rcvtimeo_ms));

        successfully_started_ = true;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to setup ZeroMQ subscriber: " << e.what() << std::endl;
        successfully_started_ = false;
        return false;
    }

    running_ = true;
    receiver_thread_ = std::thread(&DataReceiver::receiveLoop, this);
    std::cout << "DataReceiver started. Will process messages prefixed with: '"
              << (data_prefix_to_process_.empty() ? "<ANY (if ZMQ filter was also empty)>" : data_prefix_to_process_) << "'"
              << std::endl;
    return true;
}

// Stops the receiving loop
void DataReceiver::stop() {
    if (running_.load()) {
        std::cout << "Stopping DataReceiver..." << std::endl;
        running_ = false; 
    }
}

// Joins the receiver thread
void DataReceiver::join() {
    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
        std::cout << "DataReceiver thread joined." << std::endl;
    }
}

// Retrieves a copy of the collected data
std::vector<Data> DataReceiver::getCollectedData() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return collected_data_; 
}

// Checks if the receiver is running
bool DataReceiver::isRunning() const {
    return running_.load();
}

// Main receiving loop
void DataReceiver::receiveLoop() {
    std::cout << "DataReceiver loop started. Waiting for messages..." << std::endl;
    while (running_.load()) {
        zmq::message_t message;
        bool recv_ok = false; // Changed to bool

        try {
            // Use the bool recv(message_t*, int) overload.
            // Pass the address of 'message'.
            // The flags argument is an int. 0 typically means no special flags.
            // With ZMQ_RCVTIMEO set, a blocking recv (flags=0) will wait for the timeout period.
            // If it times out, it will return false (and set errno to EAGAIN) or throw zmq::error_t.
            // The specific behavior of timeout (return false vs throw) can depend on cppzmq version
            // and how it wraps the C API. Assuming it might throw for EAGAIN here based on previous structure.
            recv_ok = subscriber_socket_.recv(&message, 0); 

        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM) { 
                std::cerr << "ZeroMQ context terminated, shutting down subscriber loop." << std::endl;
                running_ = false; 
            } else if (e.num() == EAGAIN) { 
                // Timeout occurred if recv throws for EAGAIN.
                // If recv returns false on timeout, this catch block won't be hit for timeout.
                // The `if (recv_ok)` below will handle the false return.
            } else {
                std::cerr << "Error during recv: " << e.what() << " (errno: " << e.num() << ")" << std::endl;
                if (running_.load()) { 
                    running_ = false; 
                }
            }
            continue; 
        }

        if (recv_ok && message.size() > 0) { // Check recv_ok and if message actually has content
            // Successfully received a message
            // print_message_details(message, "[DEBUG] Raw message received by DataReceiver");

            size_t data_prefix_len = data_prefix_to_process_.length();
            const char* msg_data_ptr = static_cast<const char*>(message.data());
            bool prefix_match = false;
            size_t payload_offset = 0;

            if (data_prefix_len > 0) {
                if (message.size() > data_prefix_len && msg_data_ptr[data_prefix_len] == ' ' &&
                    strncmp(msg_data_ptr, data_prefix_to_process_.c_str(), data_prefix_len) == 0) {
                    prefix_match = true;
                    payload_offset = data_prefix_len + 1; 
                }
            } else {
                prefix_match = true; 
                payload_offset = 0;
            }

            if (prefix_match) {
                const char* packed_data_start_ptr = msg_data_ptr + payload_offset;
                size_t packed_data_total_size = message.size() - payload_offset;

                if (sizeof(Data) == 0) {
                    std::cerr << "Error: sizeof(Data) is 0. Ensure 'data.h' is correct and 'Data' struct is defined and packed." << std::endl;
                } else if (packed_data_total_size > 0 && packed_data_total_size % sizeof(Data) == 0) {
                    size_t num_structs = packed_data_total_size / sizeof(Data);
                    std::lock_guard<std::mutex> lock(data_mutex_); 
                    collected_data_.reserve(collected_data_.size() + num_structs); 
                    for (size_t i = 0; i < num_structs; ++i) {
                        const Data* received_data = reinterpret_cast<const Data*>(packed_data_start_ptr + i * sizeof(Data));
                        collected_data_.push_back(*received_data);
                    }
                } else if (packed_data_total_size == 0 && data_prefix_len > 0) {
                    // std::cout << "Received prefix '" << data_prefix_to_process_ << "' with empty data payload." << std::endl;
                } else if (packed_data_total_size > 0) { 
                    std::cerr << "Error: For message matching prefix '" << data_prefix_to_process_ 
                              << "', received data payload size (" << packed_data_total_size 
                              << ") is not a multiple of Data struct size (" << sizeof(Data) << ")" << std::endl;
                }
            } else if (data_prefix_len > 0) { 
                 // print_message_details(message, "[DEBUG] Unhandled message (prefix mismatch)");
            }

        } else if (recv_ok && message.size() == 0) { // recv_ok was true, but message is empty
            // std::cout << "[DEBUG] Received empty message part (size 0)." << std::endl;
        }
        // If !recv_ok, it means timeout (if recv returns false on timeout) or some other non-exception failure.
        // The loop continues, which is the correct behavior for timeouts.
    }
    std::cout << "DataReceiver loop finished." << std::endl;
}

// Helper function to print message content for debugging
void DataReceiver::print_message_details(const zmq::message_t& msg, const std::string& context_msg) {
    std::cout << context_msg << " - Size: " << msg.size() << " bytes." << std::endl;
    if (msg.size() == 0) {
        std::cout << "  Message is empty." << std::endl;
        return;
    }

    size_t preview_len = std::min<size_t>(msg.size(), 50); 
    std::string preview_str;
    bool is_all_printable = true;
    const char* msg_char_ptr = static_cast<const char*>(msg.data());

    for (size_t i = 0; i < preview_len; ++i) {
        char c = msg_char_ptr[i];
        if (isprint(static_cast<unsigned char>(c))) { 
            preview_str += c;
        } else if (c == '\n') {
            preview_str += "\\n"; 
        } else if (c == '\r') {
            preview_str += "\\r"; 
        } else if (c == '\t') {
            preview_str += "\\t"; 
        }
        else {
            is_all_printable = false;
        }
    }

    if (is_all_printable && !preview_str.empty()) {
        std::cout << "  Preview (as string): \"" << preview_str << (msg.size() > preview_len ? "..." : "") << "\"" << std::endl;
    } else {
        std::cout << "  Preview (first " << preview_len << " bytes, as hex): ";
        const unsigned char* msg_byte_ptr = static_cast<const unsigned char*>(msg.data());
        for (size_t i = 0; i < preview_len; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg_byte_ptr[i]) << " ";
        }
        std::cout << std::dec << (msg.size() > preview_len ? "..." : "") << std::endl; 
    }
}


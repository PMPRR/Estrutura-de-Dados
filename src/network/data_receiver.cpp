//// DataReceiver.cpp
//#include "network/data_receiver.h"
//#include <iostream>
//#include <iomanip>   // For std::hex, std::setw, std::setfill
//#include <algorithm> // For std::min
//#include <cstring>   // For strlen, strncmp
//#include <cctype>    // For isprint
//#include <vector>    // For std::vector
//#include <zmq.h>     // Include C API for ZMQ_ constants if zmq::sockopt is not available
//#include <thread>    // For std::this_thread::sleep_for
//#include <chrono>    // For std::chrono::seconds
//
//// Constructor
//DataReceiver::DataReceiver(const std::string& publisher_address,
//                           const std::string& zmq_topic_filter,
//                           const std::string& data_prefix_to_process)
//    : context_(1), 
//      subscriber_socket_(context_, ZMQ_SUB), // Use C ZMQ_SUB macro
//      publisher_address_(publisher_address),
//      zmq_topic_filter_(zmq_topic_filter),
//      data_prefix_to_process_(data_prefix_to_process),
//      running_(false),
//      successfully_started_(false) {
//    if (data_prefix_to_process_.empty() && !zmq_topic_filter_.empty()) {
//        data_prefix_to_process_ = zmq_topic_filter_;
//    }
//}
//
//// Destructor
//DataReceiver::~DataReceiver() {
//    stop(); 
//    join(); 
//
//    try {
//        if (successfully_started_.load()) { 
//            if (!zmq_topic_filter_.empty()) {
//                try { // Add try-catch for individual ZMQ operations
//                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
//                } catch (const zmq::error_t& e) {
//                     std::cerr << "Ignoring error during ZMQ_UNSUBSCRIBE: " << e.what() << std::endl;
//                }
//            } else {
//                 try {
//                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, "", 0);
//                 } catch (const zmq::error_t& e) {
//                     std::cerr << "Ignoring error during ZMQ_UNSUBSCRIBE (empty topic): " << e.what() << std::endl;
//                 }
//            }
//        }
//        subscriber_socket_.close();
//        context_.close(); // Closing context here should interrupt blocking calls on sockets associated with it.
//    } catch (const zmq::error_t& e) {
//        std::cerr << "Exception during DataReceiver ZMQ cleanup: " << e.what() << std::endl;
//    } catch (const std::exception& e) {
//        std::cerr << "Standard exception during DataReceiver cleanup: " << e.what() << std::endl;
//    } catch (...) {
//        std::cerr << "Unknown exception during DataReceiver cleanup." << std::endl;
//    }
//}
//
//// Starts the receiving loop
//bool DataReceiver::start() {
//    if (running_.load()) {
//        std::cout << "DataReceiver is already running." << std::endl;
//        return true; 
//    }
//
//    try {
//        std::cout << "DataReceiver connecting to " << publisher_address_ << "..." << std::endl;
//        subscriber_socket_.connect(publisher_address_);
//        std::cout << "DataReceiver connected." << std::endl;
//
//        std::cout << "DataReceiver: Setting TCP Keepalive options..." << std::endl;
//        int keepalive = 1;
//        int keepalive_idle_sec = 60; 
//        int keepalive_interval_sec = 5; 
//        int keepalive_count = 3;   
//
//        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE, &keepalive, sizeof(keepalive));
//        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_IDLE, &keepalive_idle_sec, sizeof(keepalive_idle_sec));
//        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_INTVL, &keepalive_interval_sec, sizeof(keepalive_interval_sec));
//        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_CNT, &keepalive_count, sizeof(keepalive_count));
//        std::cout << "DataReceiver: TCP Keepalive options set." << std::endl;
//
//
//        std::cout << "DataReceiver subscribing with ZMQ filter: '" << (zmq_topic_filter_.empty() ? "<ALL MESSAGES>" : zmq_topic_filter_) << "'" << std::endl;
//        subscriber_socket_.setsockopt(ZMQ_SUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
//        
//        // REMOVED ZMQ_RCVTIMEO - using blocking receive now
//        // int rcvtimeo_ms = 100; 
//        // subscriber_socket_.setsockopt(ZMQ_RCVTIMEO, &rcvtimeo_ms, sizeof(rcvtimeo_ms));
//
//        std::cout << "DataReceiver: Allowing 1 second for subscription to establish..." << std::endl;
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::cout << "DataReceiver: Subscription delay complete." << std::endl;
//
//        successfully_started_ = true;
//    } catch (const zmq::error_t& e) {
//        std::cerr << "Failed to setup ZeroMQ subscriber: " << e.what() << std::endl;
//        successfully_started_ = false;
//        return false;
//    }
//
//    running_ = true;
//    receiver_thread_ = std::thread(&DataReceiver::receiveLoop, this);
//    std::cout << "DataReceiver started. Will process messages prefixed with: '"
//              << (data_prefix_to_process_.empty() ? "<ANY (if ZMQ filter was also empty)>" : data_prefix_to_process_) << "'"
//              << std::endl;
//    return true;
//}
//
//// Stops the receiving loop
//void DataReceiver::stop() {
//    if (running_.load()) {
//        std::cout << "Stopping DataReceiver..." << std::endl;
//        running_ = false; 
//        // When using blocking recv, closing the context is a robust way to interrupt
//        // the recv() call. This is done in the destructor.
//        // If the destructor isn't called immediately upon stop(), explicitly closing
//        // the context here might be an option, but can be tricky if the socket is still in use.
//        // For now, rely on setting running_ = false and context termination in destructor.
//    }
//}
//
//// Joins the receiver thread
//void DataReceiver::join() {
//    if (receiver_thread_.joinable()) {
//        receiver_thread_.join();
//        std::cout << "DataReceiver thread joined." << std::endl;
//    }
//}
//
//// Retrieves a copy of the collected data
//std::vector<Data> DataReceiver::getCollectedData() const {
//    std::lock_guard<std::mutex> lock(data_mutex_);
//    return collected_data_; 
//}
//
//// Checks if the receiver is running
//bool DataReceiver::isRunning() const {
//    return running_.load();
//}
//
//// Main receiving loop
//void DataReceiver::receiveLoop() {
//    std::cout << "[DataReceiver::receiveLoop] Loop started. Waiting for messages (blocking)..." << std::endl;
//    while (running_.load()) {
//        zmq::message_t message;
//        bool recv_ok = false; 
//
//        try {
//            // Using blocking recv (flags = 0, no ZMQ_RCVTIMEO set)
//            // This call will block indefinitely until a message is received or an error occurs.
//            std::cout << "[DataReceiver::receiveLoop] Attempting blocking recv..." << std::endl;
//            recv_ok = subscriber_socket_.recv(&message, 0); 
//            // If running_ is set to false by another thread while recv() is blocking,
//            // recv() should be interrupted when the context is terminated (e.g., by context_.close() in ~DataReceiver).
//        } catch (const zmq::error_t& e) {
//            if (e.num() == ETERM) { 
//                std::cerr << "[DataReceiver::receiveLoop] ZeroMQ context terminated during recv, shutting down subscriber loop." << std::endl;
//                running_ = false; // Ensure loop terminates
//                break; // Exit loop immediately
//            } else if (e.num() == EINTR) { // Interrupted system call, e.g. by signal while blocking
//                 std::cerr << "[DataReceiver::receiveLoop] Blocking recv interrupted (EINTR). Continuing check for running_ flag." << std::endl;
//                 recv_ok = false; // Mark as not okay, loop will re-check running_
//            }
//            // EAGAIN should not typically happen with a fully blocking recv unless specific socket options are set.
//            // If it does, it might indicate a deeper issue or misconfiguration.
//            else {
//                std::cerr << "[DataReceiver::receiveLoop] Error during blocking recv: " << e.what() << " (errno: " << e.num() << ")" << std::endl;
//                // For persistent errors, it might be prudent to stop the loop.
//                if (running_.load()) { 
//                    // Consider if other errors should also set running_ = false;
//                }
//                recv_ok = false;
//            }
//        }
//
//        if (!running_.load()) { // Check running_ flag again after potential blocking recv or exception
//            std::cout << "[DataReceiver::receiveLoop] running_ is false, exiting loop after recv attempt." << std::endl;
//            break;
//        }
//
//        if (recv_ok && message.size() > 0) {
//            std::cout << "[DataReceiver::receiveLoop] Message received successfully!" << std::endl;
//            print_message_details(message, "[DEBUG DataReceiver] Raw ZMQ message received");
//
//            size_t data_prefix_len = data_prefix_to_process_.length();
//            const char* msg_data_ptr = static_cast<const char*>(message.data());
//            bool prefix_match = false;
//            size_t payload_offset = 0;
//
//            if (data_prefix_len > 0) {
//                if (message.size() > data_prefix_len && msg_data_ptr[data_prefix_len] == ' ' &&
//                    strncmp(msg_data_ptr, data_prefix_to_process_.c_str(), data_prefix_len) == 0) {
//                    prefix_match = true;
//                    payload_offset = data_prefix_len + 1; 
//                }
//            } else { 
//                prefix_match = true; 
//                payload_offset = 0;
//            }
//
//            if (prefix_match) {
//                const char* packed_data_start_ptr = msg_data_ptr + payload_offset;
//                size_t packed_data_total_size = message.size() - payload_offset;
//                //std::cout << "[DEBUG DataReceiver] C++ sizeof(Data) = " << sizeof(Data) << ", incoming payload = " << packed_data_total_size << " bytes" << std::endl;
//
//                //std::cout << "[DEBUG DataReceiver] Prefix '" << data_prefix_to_process_ << "' matched. " << "Payload size: " << packed_data_total_size << ", sizeof(Data) in C++: " << sizeof(Data) << std::endl;
//                if (sizeof(Data) == 0) {
//                    std::cerr << "[DataReceiver] Error: sizeof(Data) is 0. Ensure 'data.h' is correct and 'Data' struct is defined and packed." << std::endl;
//                } else if (packed_data_total_size > 0 && packed_data_total_size % sizeof(Data) == 0) {
//                    size_t num_structs = packed_data_total_size / sizeof(Data);
//                    std::cout << "[DEBUG DataReceiver] Processing " << num_structs << " Data struct(s) from payload." << std::endl;
//                    
//                    std::lock_guard<std::mutex> lock(data_mutex_); 
//                    collected_data_.reserve(collected_data_.size() + num_structs); 
//                    for (size_t i = 0; i < num_structs; ++i) {
//                        const Data* received_data_ptr = reinterpret_cast<const Data*>(packed_data_start_ptr + i * sizeof(Data));
//                        collected_data_.push_back(*received_data_ptr);
//                        //std::cout << "[DEBUG DataReceiver] Added Data struct to collected_data_. ID: " << received_data_ptr->id << ", Current collected_data_ size: " << collected_data_.size() << std::endl;
//                    }
//                } else if (packed_data_total_size == 0 && data_prefix_len > 0) {
//                     std::cout << "[DEBUG DataReceiver] Received prefix '" << data_prefix_to_process_ << "' with empty data payload." << std::endl;
//                } else if (packed_data_total_size > 0) { 
//                    std::cerr << "[DataReceiver] Error: For message matching prefix '" << data_prefix_to_process_ 
//                              << "', received data payload size (" << packed_data_total_size 
//                              << ") is not a multiple of Data struct size (" << sizeof(Data) << "). Message possibly corrupted or sizeof(Data) mismatch." << std::endl;
//                }
//            } else if (data_prefix_len > 0) { 
//                 print_message_details(message, "[DEBUG DataReceiver] Unhandled message (prefix mismatch)");
//            }
//
//        } else if (recv_ok && message.size() == 0) { 
//             std::cout << "[DEBUG DataReceiver] Received empty ZMQ message part (size 0)." << std::endl;
//        }
//        // No explicit timeout logging needed here as recv is blocking.
//        // If running_ becomes false, loop will terminate on next iteration or if recv is interrupted.
//    }
//    std::cout << "DataReceiver loop finished." << std::endl;
//}
//
//// Helper function to print message content for debugging
//void DataReceiver::print_message_details(const zmq::message_t& msg, const std::string& context_msg) {
//    std::cout << context_msg << " - Size: " << msg.size() << " bytes." << std::endl;
//    if (msg.size() == 0) {
//        std::cout << "  Message is empty." << std::endl;
//        return;
//    }
//
//    size_t preview_len = std::min<size_t>(msg.size(), 64); 
//    std::string preview_str_printable;
//    bool is_any_printable = false;
//    const char* msg_char_ptr = static_cast<const char*>(msg.data());
//
//    for (size_t i = 0; i < preview_len; ++i) {
//        char c = msg_char_ptr[i];
//        if (isprint(static_cast<unsigned char>(c))) { 
//            preview_str_printable += c;
//            is_any_printable = true;
//        }
//    }
//    
//    std::cout << "  Preview (first " << preview_len << " bytes, as hex): ";
//    const unsigned char* msg_byte_ptr = static_cast<const unsigned char*>(msg.data());
//    for (size_t i = 0; i < preview_len; ++i) {
//        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg_byte_ptr[i]) << " ";
//    }
//    std::cout << std::dec << (msg.size() > preview_len ? "..." : "") << std::endl; 
//
//    if (is_any_printable && !preview_str_printable.empty()) {
//        std::cout << "  Printable Preview: \"" << preview_str_printable << (msg.size() > preview_len && preview_str_printable.length() == preview_len ? "..." : "") << "\"" << std::endl;
//    }
//}
//
// DataReceiver.cpp
// DataReceiver.cpp
#include "network/data_receiver.h"
#include <iostream>
#include <iomanip>   // For std::hex, std::setw, std::setfill
#include <algorithm> // For std::min
#include <cstring>   // For strlen, strncmp
#include <cctype>    // For isprint
#include <vector>    // For std::vector (if needed for some helper, but not for collected_data_)
#include <zmq.h>     // Include C API for ZMQ_ constants if zmq::sockopt is not available
#include <thread>    // For std::this_thread::sleep_for
#include <chrono>    // For std::chrono::seconds

// Constructor
DataReceiver::DataReceiver(const std::string& publisher_address,
                           const std::string& zmq_topic_filter,
                           const std::string& data_prefix_to_process)
    : context_(1), 
      subscriber_socket_(context_, ZMQ_SUB),
      publisher_address_(publisher_address),
      zmq_topic_filter_(zmq_topic_filter),
      data_prefix_to_process_(data_prefix_to_process),
      running_(false),
      successfully_started_(false),
      current_data_count_(0) { // Initialize current_data_count_
    if (data_prefix_to_process_.empty() && !zmq_topic_filter_.empty()) {
        data_prefix_to_process_ = zmq_topic_filter_;
    }
}

// Destructor
DataReceiver::~DataReceiver() {
    stop(); 
    join(); 

    try {
        if (successfully_started_.load()) { 
            if (!zmq_topic_filter_.empty()) {
                try { // Add try-catch for individual ZMQ operations
                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
                } catch (const zmq::error_t& e) {
                     std::cerr << "Ignoring error during ZMQ_UNSUBSCRIBE: " << e.what() << std::endl;
                }
            } else {
                 try {
                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, "", 0);
                 } catch (const zmq::error_t& e) {
                     std::cerr << "Ignoring error during ZMQ_UNSUBSCRIBE (empty topic): " << e.what() << std::endl;
                 }
            }
        }
        subscriber_socket_.close();
        context_.close(); // Closing context here should interrupt blocking calls on sockets associated with it.
    } catch (const zmq::error_t& e) {
        std::cerr << "Exception during DataReceiver ZMQ cleanup: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception during DataReceiver cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during DataReceiver cleanup." << std::endl;
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

        std::cout << "DataReceiver: Setting TCP Keepalive options..." << std::endl;
        int keepalive = 1;
        int keepalive_idle_sec = 60; 
        int keepalive_interval_sec = 5; 
        int keepalive_count = 3;   

        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE, &keepalive, sizeof(keepalive));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_IDLE, &keepalive_idle_sec, sizeof(keepalive_idle_sec));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_INTVL, &keepalive_interval_sec, sizeof(keepalive_interval_sec));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_CNT, &keepalive_count, sizeof(keepalive_count));
        std::cout << "DataReceiver: TCP Keepalive options set." << std::endl;


        std::cout << "DataReceiver subscribing with ZMQ filter: '" << (zmq_topic_filter_.empty() ? "<ALL MESSAGES>" : zmq_topic_filter_) << "'" << std::endl;
        subscriber_socket_.setsockopt(ZMQ_SUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
        
        // REMOVED ZMQ_RCVTIMEO - using blocking receive now
        // int rcvtimeo_ms = 100; 
        // subscriber_socket_.setsockopt(ZMQ_RCVTIMEO, &rcvtimeo_ms, sizeof(rcvtimeo_ms));

        std::cout << "DataReceiver: Allowing 1 second for subscription to establish..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "DataReceiver: Subscription delay complete." << std::endl;

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
        // When using blocking recv, closing the context is a robust way to interrupt
        // the recv() call. This is done in the destructor.
        // If the destructor isn't called immediately upon stop(), explicitly closing
        // the context here might be an option, but can be tricky if the socket is still in use.
        // For now, rely on setting running_ = false and context termination in destructor.
    }
}

// Joins the receiver thread
void DataReceiver::join() {
    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
        std::cout << "DataReceiver thread joined." << std::endl;
    }
}

// Retrieves a shallow view of the collected data.
std::pair<const Data*, size_t> DataReceiver::getCollectedData() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return {collected_data_array_.data(), current_data_count_.load()}; 
}

// Checks if the receiver is running
bool DataReceiver::isRunning() const {
    return running_.load();
}

// Main receiving loop
void DataReceiver::receiveLoop() {
    std::cout << "[DataReceiver::receiveLoop] Loop started. Waiting for messages (blocking)..." << std::endl;
    while (running_.load()) {
        zmq::message_t message;
        bool recv_ok = false; 

        try {
            // Using blocking recv (flags = 0, no ZMQ_RCVTIMEO set)
            // This call will block indefinitely until a message is received or an error occurs.
            std::cout << "[DataReceiver::receiveLoop] Attempting blocking recv..." << std::endl;
            recv_ok = subscriber_socket_.recv(&message, 0); 
            // If running_ is set to false by another thread while recv() is blocking,
            // recv() should be interrupted when the context is terminated (e.g., by context_.close() in ~DataReceiver).
        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM) { 
                std::cerr << "[DataReceiver::receiveLoop] ZeroMQ context terminated during recv, shutting down subscriber loop." << std::endl;
                running_ = false; // Ensure loop terminates
                break; // Exit loop immediately
            } else if (e.num() == EINTR) { // Interrupted system call, e.g. by signal while blocking
                 std::cerr << "[DataReceiver::receiveLoop] Blocking recv interrupted (EINTR). Continuing check for running_ flag." << std::endl;
                 recv_ok = false; // Mark as not okay, loop will re-check running_
            }
            // EAGAIN should not typically happen with a fully blocking recv unless specific socket options are set.
            // If it does, it might indicate a deeper issue or misconfiguration.
            else {
                std::cerr << "[DataReceiver::receiveLoop] Error during blocking recv: " << e.what() << " (errno: " << e.num() << ")" << std::endl;
                // For persistent errors, it might be prudent to stop the loop.
                if (running_.load()) { 
                    // Consider if other errors should also set running_ = false;
                }
                recv_ok = false;
            }
        }

        if (!running_.load()) { // Check running_ flag again after potential blocking recv or exception
            std::cout << "[DataReceiver::receiveLoop] running_ is false, exiting loop after recv attempt." << std::endl;
            break;
        }

        if (recv_ok && message.size() > 0) {
            std::cout << "[DataReceiver::receiveLoop] Message received successfully!" << std::endl;
            print_message_details(message, "[DEBUG DataReceiver] Raw ZMQ message received");

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
                //std::cout << "[DEBUG DataReceiver] C++ sizeof(Data) = " << sizeof(Data) << ", incoming payload = " << packed_data_total_size << " bytes" << std::endl;

                //std::cout << "[DEBUG DataReceiver] Prefix '" << data_prefix_to_process_ << "' matched. " << "Payload size: " << packed_data_total_size << ", sizeof(Data) in C++: " << sizeof(Data) << std::endl;
                if (sizeof(Data) == 0) {
                    std::cerr << "[DataReceiver] Error: sizeof(Data) is 0. Ensure 'data.h' is correct and 'Data' struct is defined and packed." << std::endl;
                } else if (packed_data_total_size > 0 && packed_data_total_size % sizeof(Data) == 0) {
                    size_t num_structs = packed_data_total_size / sizeof(Data);
                    std::cout << "[DEBUG DataReceiver] Processing " << num_structs << " Data struct(s) from payload." << std::endl;
                    
                    std::lock_guard<std::mutex> lock(data_mutex_); 
                    // Check for capacity before adding
                    if (current_data_count_.load() + num_structs > DATA_RECEIVER_CAPACITY) {
                        std::cerr << "[DataReceiver] Warning: Not enough capacity in collected_data_array_ to store all incoming structs. Dropping " 
                                  << (current_data_count_.load() + num_structs - DATA_RECEIVER_CAPACITY) << " structs." << std::endl;
                        num_structs = DATA_RECEIVER_CAPACITY - current_data_count_.load(); // Only process what fits
                    }

                    for (size_t i = 0; i < num_structs; ++i) {
                        const Data* received_data_ptr = reinterpret_cast<const Data*>(packed_data_start_ptr + i * sizeof(Data));
                        collected_data_array_[current_data_count_] = *received_data_ptr; // Copy into array
                        current_data_count_++; // Increment count of valid items
                    }
                    std::cout << "[DEBUG DataReceiver] Added " << num_structs << " Data struct(s) to collected_data_array_. Current count: " << current_data_count_.load() << std::endl;

                } else if (packed_data_total_size == 0 && data_prefix_len > 0) {
                     std::cout << "[DEBUG DataReceiver] Received prefix '" << data_prefix_to_process_ << "' with empty data payload." << std::endl;
                } else if (packed_data_total_size > 0) { 
                    std::cerr << "[DataReceiver] Error: For message matching prefix '" << data_prefix_to_process_ 
                              << "', received data payload size (" << packed_data_total_size 
                              << ") is not a multiple of Data struct size (" << sizeof(Data) << "). Message possibly corrupted or sizeof(Data) mismatch." << std::endl;
                }
            } else if (data_prefix_len > 0) { 
                 print_message_details(message, "[DEBUG DataReceiver] Unhandled message (prefix mismatch)");
            }

        } else if (recv_ok && message.size() == 0) { 
             std::cout << "[DEBUG DataReceiver] Received empty ZMQ message part (size 0)." << std::endl;
        }
        // No explicit timeout logging needed here as recv is blocking.
        // If running_ becomes false, loop will terminate on next iteration or if recv is interrupted.
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

    size_t preview_len = std::min<size_t>(msg.size(), 64); 
    std::string preview_str_printable;
    bool is_any_printable = false;
    const char* msg_char_ptr = static_cast<const char*>(msg.data());

    for (size_t i = 0; i < preview_len; ++i) {
        char c = msg_char_ptr[i];
        if (isprint(static_cast<unsigned char>(c))) { 
            preview_str_printable += c;
            is_any_printable = true;
        }
    }
    
    std::cout << "  Preview (first " << preview_len << " bytes, as hex): ";
    const unsigned char* msg_byte_ptr = static_cast<const unsigned char*>(msg.data());
    for (size_t i = 0; i < preview_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg_byte_ptr[i]) << " ";
    }
    std::cout << std::dec << (msg.size() > preview_len ? "..." : "") << std::endl; 

    if (is_any_printable && !preview_str_printable.empty()) {
        std::cout << "  Printable Preview: \"" << preview_str_printable << (msg.size() > preview_len && preview_str_printable.length() == preview_len ? "..." : "") << "\"" << std::endl;
    }
}


// Criado por GEMINI 2.5 Pro AI
// DataReceiver.cpp
#include "network/data_receiver.h"
#include <iostream>
#include <iomanip>   // For std::hex, std::setw, std::setfill
#include <algorithm> // For std::min
#include <cstring>   // For strlen, strncmp
#include <cctype>    // For isprint
#include <vector>    // Required for std::vector in getCollectedDataCopy
#include <zmq.h>     // Include C API for ZMQ_ constants like ETERM
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
      head_(0),
      tail_(0),
      data_ready_(0) {
}

// Destructor
DataReceiver::~DataReceiver() {
    stop();
    join();

    try {
        if (successfully_started_.load()) {
            if (!zmq_topic_filter_.empty()) {
                try {
                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
                } catch (const zmq::error_t& e) {
                     std::cerr << "[DataReceiver] Ignoring error during ZMQ_UNSUBSCRIBE: " << e.what() << std::endl;
                }
            } else {
                 try {
                    subscriber_socket_.setsockopt(ZMQ_UNSUBSCRIBE, "", 0);
                 } catch (const zmq::error_t& e) {
                     std::cerr << "[DataReceiver] Ignoring error during ZMQ_UNSUBSCRIBE (empty topic): " << e.what() << std::endl;
                 }
            }
        }
    } catch (const zmq::error_t& e) {
        std::cerr << "[DataReceiver] Exception during ZMQ cleanup: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DataReceiver] Standard exception during cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[DataReceiver] Unknown exception during cleanup." << std::endl;
    }
}

// Starts the receiving loop
bool DataReceiver::start() {
    if (running_.load()) {
        std::cout << "[DataReceiver] Already running." << std::endl;
        return true;
    }

    try {
        std::cout << "[DataReceiver] Connecting to " << publisher_address_ << "..." << std::endl;
        subscriber_socket_.connect(publisher_address_);
        std::cout << "[DataReceiver] Connected." << std::endl;

        std::cout << "[DataReceiver] Setting TCP Keepalive options..." << std::endl;
        int keepalive = 1;
        int keepalive_idle_sec = 60;
        int keepalive_interval_sec = 5;
        int keepalive_count = 3;

        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE, &keepalive, sizeof(keepalive));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_IDLE, &keepalive_idle_sec, sizeof(keepalive_idle_sec));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_INTVL, &keepalive_interval_sec, sizeof(keepalive_interval_sec));
        subscriber_socket_.setsockopt(ZMQ_TCP_KEEPALIVE_CNT, &keepalive_count, sizeof(keepalive_count));
        std::cout << "[DataReceiver] TCP Keepalive options set." << std::endl;


        std::cout << "[DataReceiver] Subscribing with ZMQ filter: '" << (zmq_topic_filter_.empty() ? "<ALL MESSAGES>" : zmq_topic_filter_) << "'" << std::endl;
        subscriber_socket_.setsockopt(ZMQ_SUBSCRIBE, zmq_topic_filter_.c_str(), zmq_topic_filter_.length());
        

        std::cout << "[DataReceiver] Allowing 1 second for subscription to establish..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[DataReceiver] Subscription delay complete." << std::endl;

        successfully_started_ = true;
    } catch (const zmq::error_t& e) {
        std::cerr << "[DataReceiver] Failed to setup ZeroMQ subscriber: " << e.what() << std::endl;
        successfully_started_ = false;
        return false;
    }

    running_ = true;
    receiver_thread_ = std::thread(&DataReceiver::receiveLoop, this);
    std::cout << "[DataReceiver] Started. ZMQ Topic Filter: '"
              << (zmq_topic_filter_.empty() ? "<NONE - expecting prefix in payload>" : zmq_topic_filter_) << "'. "
              << "Internal Payload Prefix Check: '"
              << (data_prefix_to_process_.empty() ? "<NONE - any payload>" : data_prefix_to_process_) << "' (only used if ZMQ filter is empty)."
              << std::endl;
    return true;
}

// Stops the receiving loop
void DataReceiver::stop() {
    if (running_.load()) {
        std::cout << "[DataReceiver] Stopping DataReceiver..." << std::endl;
        running_ = false;
        try {
             context_.close(); 
        } catch (const zmq::error_t& e) {
            if (e.num() != ETERM) { 
                 std::cerr << "[DataReceiver::stop] Error closing context: " << e.what() << std::endl;
            }
        }
    }
}

// Joins the receiver thread
void DataReceiver::join() {
    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
        std::cout << "[DataReceiver] Thread joined." << std::endl;
    }
}

// Retrieves a view of a contiguous block of currently collected data.
// The second element of the pair indicates the number of items in this contiguous block.
// Returns {nullptr, 0} if no data is available.
std::pair<const Data*, size_t> DataReceiver::getCollectedDataView() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_); // Protects head_ and data_ready_ for consistent view
    if (data_ready_.load() == 0) {
        return {nullptr, 0};
    }
    
    // Calculate the number of contiguous items from head_ to the end of the array,
    // or until all available data is accounted for.
    size_t num_contiguous_items = DATA_RECEIVER_CAPACITY - head_.load();
    if (num_contiguous_items > data_ready_.load()) {
        num_contiguous_items = data_ready_.load();
    }
    
    return {&collected_data_array_[head_], num_contiguous_items};
}

// Marks 'count' data items as consumed from the beginning of the unread data.
// This effectively frees up space in the circular buffer.
void DataReceiver::markDataAsConsumed(size_t count) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (count > data_ready_.load()) {
        std::cerr << "[DataReceiver ERROR] Attempted to consume more data than available. Consuming all " 
                  << data_ready_.load() << " available items." << std::endl;
        count = data_ready_.load(); // Cap count to available data
    }
    head_ = (head_ + count) % DATA_RECEIVER_CAPACITY;
    data_ready_ -= count;

    // Optional debug print for buffer state
    // std::cout << "[DEBUG DataReceiver] Consumed " << count << " items. head: " << head_.load() 
    //           << ", tail: " << tail_.load() << ", data_ready: " << data_ready_.load() << std::endl;
}

// Checks if the receiver is running
bool DataReceiver::isRunning() const {
    return running_.load();
}

// Main receiving loop
void DataReceiver::receiveLoop() {
    std::cout << "[DataReceiver::receiveLoop] Loop started. Waiting for messages (blocking)..." << std::endl;
    while (running_.load()) {
        zmq::message_t received_message; 
        bool recv_ok = false;

        try {
            recv_ok = subscriber_socket_.recv(&received_message); 
        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM ) { 
                std::cerr << "[DataReceiver::receiveLoop] ZeroMQ context terminated, shutting down." << std::endl;
                running_ = false; 
                break; 
            } else if (e.num() == EINTR) { 
                 std::cerr << "[DataReceiver::receiveLoop] Blocking recv interrupted (EINTR). Continuing." << std::endl;
                 recv_ok = false; 
            }
            else {
                std::cerr << "[DataReceiver::receiveLoop] Error during recv: " << e.what() << " (ZMQ errno: " << e.num() << ")" << std::endl;
                recv_ok = false;
                if(running_.load()) { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }

        if (!running_.load()) { 
            break;
        }

        if (recv_ok && received_message.size() > 0) {
            const char* payload_to_process_ptr = nullptr;
            size_t payload_to_process_size = 0;
            bool is_valid_payload = false;

            if (!zmq_topic_filter_.empty()) {
                if (received_message.size() == zmq_topic_filter_.length()) {
                    std::string content(static_cast<const char*>(received_message.data()), received_message.size());
                    if (content == zmq_topic_filter_) {
                        is_valid_payload = false; 
                    } else {
                        payload_to_process_ptr = static_cast<const char*>(received_message.data());
                        payload_to_process_size = received_message.size();
                        is_valid_payload = true;
                    }
                } else {
                    payload_to_process_ptr = static_cast<const char*>(received_message.data());
                    payload_to_process_size = received_message.size();
                    is_valid_payload = true;
                }
            } else {
                size_t data_prefix_len = data_prefix_to_process_.length();
                if (data_prefix_len > 0) {
                    if (received_message.size() > data_prefix_len && received_message.data<char>()[data_prefix_len] == ' ' &&
                        strncmp(received_message.data<char>(), data_prefix_to_process_.c_str(), data_prefix_len) == 0) {
                        
                        payload_to_process_ptr = static_cast<const char*>(received_message.data()) + data_prefix_len + 1;
                        payload_to_process_size = received_message.size() - (data_prefix_len + 1);
                        is_valid_payload = true;
                    }
                } else { 
                    payload_to_process_ptr = static_cast<const char*>(received_message.data());
                    payload_to_process_size = received_message.size();
                    is_valid_payload = true;
                }
            }

            if (is_valid_payload && payload_to_process_ptr) {
                if (sizeof(Data) == 0) {
                    std::cerr << "[DataReceiver] Error: sizeof(Data) is 0. Ensure 'data.h' is correct." << std::endl;
                } else if (payload_to_process_size > 0 && payload_to_process_size % sizeof(Data) == 0) {
                    size_t num_structs_in_payload = payload_to_process_size / sizeof(Data);
                    std::lock_guard<std::mutex> lock(buffer_mutex_); 

                    // Number of contiguous slots from current tail_ to end of array
                    size_t contiguous_free_slots_at_end = DATA_RECEIVER_CAPACITY - tail_.load();
                    // Number of slots available before wrapping around (or hitting head_)
                    size_t total_free_slots = DATA_RECEIVER_CAPACITY - data_ready_.load();
                    
                    size_t structs_to_copy = std::min(num_structs_in_payload, total_free_slots);

                    if (structs_to_copy == 0 && num_structs_in_payload > 0) {
                         std::cerr << "[DataReceiver] Warning: Buffer FULL. Dropping "
                                   << num_structs_in_payload << " structs (tail: " << tail_.load() << ", head: " << head_.load() << ", data_ready: " << data_ready_.load() << ")." << std::endl;
                    } else if (structs_to_copy < num_structs_in_payload) {
                         std::cerr << "[DataReceiver] Warning: Buffer NEARLY FULL. Dropping "
                                   << (num_structs_in_payload - structs_to_copy) << " structs (tail: " << tail_.load() << ", head: " << head_.load() << ", data_ready: " << data_ready_.load() << ")." << std::endl;
                    }

                    if (structs_to_copy > 0) { 
                        // Copy data into the circular buffer
                        for (size_t i = 0; i < structs_to_copy; ++i) {
                            collected_data_array_[tail_] = *(reinterpret_cast<const Data*>(payload_to_process_ptr) + i);
                            tail_ = (tail_ + 1) % DATA_RECEIVER_CAPACITY;
                        }
                        data_ready_ += structs_to_copy;
                    }
                } else if (payload_to_process_size != 0) { 
                    std::cerr << "[DataReceiver] Error: Received data payload size (" << payload_to_process_size
                              << ") is not a multiple of Data struct size (" << sizeof(Data) << "). Corrupted or mismatched." << std::endl;
                }
            }
        } else if (recv_ok && received_message.size() == 0) {
             // std::cout << "[DEBUG DataReceiver] Received empty ZMQ message part (size 0)." << std::endl;
        }
    }
    std::cout << "[DataReceiver::receiveLoop] Loop finished." << std::endl;
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


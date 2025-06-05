//#ifndef DATA_RECEIVER_H
//#define DATA_RECEIVER_H
//
//#include <vector>
//#include <string>
//#include <thread>
//#include <atomic>
//#include <mutex>
//#include <zmq.hpp> // For the C++ ZeroMQ bindings
//#include "data.h"  // Assumes data.h defines your 'Data' struct
//
//class DataReceiver {
//public:
//    // Constructor
//    // publisher_address: e.g., "tcp://localhost:5556"
//    // zmq_topic_filter: Topic for ZMQ_SUBSCRIBE (e.g., "" for all, "data_batch" for specific)
//    // data_prefix_to_process: If subscribing to all (e.g. ""), this prefix is checked in the message payload.
//    DataReceiver(const std::string& publisher_address,
//                 const std::string& zmq_topic_filter = "",
//                 const std::string& data_prefix_to_process = "data_batch");
//    ~DataReceiver();
//
//    // Starts the receiving loop in a separate thread.
//    // Returns true if connection and subscription were successful, false otherwise.
//    bool start();
//
//    // Stops the receiving loop.
//    void stop();
//
//    // Waits for the receiver thread to complete.
//    void join();
//
//    // Retrieves a copy of the collected data.
//    // This is thread-safe.
//    std::vector<Data> getCollectedData() const;
//
//    // Checks if the receiver is currently running.
//    bool isRunning() const;
//
//private:
//    // The main loop for receiving messages.
//    void receiveLoop();
//
//    // Helper function to print message details for debugging.
//    void print_message_details(const zmq::message_t& msg, const std::string& context_msg);
//
//    zmq::context_t context_;
//    zmq::socket_t subscriber_socket_;
//    std::string publisher_address_;
//    std::string zmq_topic_filter_;       // Topic used for ZMQ_SUBSCRIBE
//    std::string data_prefix_to_process_; // String prefix to identify relevant data messages
//
//    std::vector<Data> collected_data_;
//    mutable std::mutex data_mutex_; // Protects access to collected_data_
//
//    std::thread receiver_thread_;
//    std::atomic<bool> running_;          // Controls the receiver loop
//    std::atomic<bool> successfully_started_; // Tracks if ZMQ setup was okay
//};
//
//#endif // DATA_RECEIVER_H
////
///

#ifndef DATA_RECEIVER_H
#define DATA_RECEIVER_H

#include <vector> // Still needed for std::vector in getCollectedData return type (for now)
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>   // For std::array
#include <utility> // For std::pair
#include <zmq.hpp> // For the C++ ZeroMQ bindings
#include "data.h"  // Assumes data.h defines your 'Data' struct

// Define the fixed capacity for the internal data array
const size_t DATA_RECEIVER_CAPACITY = 10000;

class DataReceiver {
public:
    // Constructor
    // publisher_address: e.g., "tcp://localhost:5556"
    // zmq_topic_filter: Topic for ZMQ_SUBSCRIBE (e.g., "" for all, "data_batch" for specific)
    // data_prefix_to_process: If subscribing to all (e.g. ""), this prefix is checked in the message payload.
    DataReceiver(const std::string& publisher_address,
                 const std::string& zmq_topic_filter = "",
                 const std::string& data_prefix_to_process = "data_batch");
    ~DataReceiver();

    // Starts the receiving loop in a separate thread.
    // Returns true if connection and subscription were successful, false otherwise.
    bool start();

    // Stops the receiving loop.
    void stop();

    // Waits for the receiver thread to complete.
    void join();

    // Retrieves a shallow view of the collected data.
    // Returns a pair: {pointer to the start of data, count of valid data items}.
    // This is thread-safe.
    std::pair<const Data*, size_t> getCollectedData() const;

    // Checks if the receiver is currently running.
    bool isRunning() const;

private:
    // The main loop for receiving messages.
    void receiveLoop();

    // Helper function to print message details for debugging.
    void print_message_details(const zmq::message_t& msg, const std::string& context_msg);

    zmq::context_t context_;
    zmq::socket_t subscriber_socket_;
    std::string publisher_address_;
    std::string zmq_topic_filter_;       // Topic used for ZMQ_SUBSCRIBE
    std::string data_prefix_to_process_; // String prefix to identify relevant data messages

    std::array<Data, DATA_RECEIVER_CAPACITY> collected_data_array_; // Changed to std::array
    std::atomic<size_t> current_data_count_;                        // Tracks valid elements in the array
    mutable std::mutex data_mutex_;                                 // Protects access to collected_data_array_ and current_data_count_

    std::thread receiver_thread_;
    std::atomic<bool> running_;          // Controls the receiver loop
    std::atomic<bool> successfully_started_; // Tracks if ZMQ setup was okay
};

#endif // DATA_RECEIVER_H


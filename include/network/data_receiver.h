#ifndef DATA_RECEIVER_H
#define DATA_RECEIVER_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>   // For std::array
#include <utility> // For std::pair
#include <zmq.hpp> // For the C++ ZeroMQ bindings
#include "data.h"  // Assumes data.h defines your 'Data' struct

// Define the fixed capacity for the internal data array
const size_t DATA_RECEIVER_CAPACITY = 30000;

class DataReceiver {
public:
    // Constructor
    DataReceiver(const std::string& publisher_address,
                 const std::string& zmq_topic_filter = "",
                 const std::string& data_prefix_to_process = "data_batch");
    ~DataReceiver();

    bool start();
    void stop();
    void join();

    // Retrieves a view of a contiguous block of currently collected data.
    // The second element of the pair indicates the number of items in this contiguous block.
    // Returns {nullptr, 0} if no data is available.
    std::pair<const Data*, size_t> getCollectedDataView() const; // <<-- Adicionado esta declaração

    // Marks 'count' data items as consumed from the beginning of the unread data.
    // This effectively frees up space in the circular buffer.
    void markDataAsConsumed(size_t count);

    // Checks if the receiver is currently running.
    bool isRunning() const;

private:
    void receiveLoop();
    void print_message_details(const zmq::message_t& msg, const std::string& context_msg);

    zmq::context_t context_;
    zmq::socket_t subscriber_socket_;
    std::string publisher_address_;
    std::string zmq_topic_filter_;       
    std::string data_prefix_to_process_; 

    std::array<Data, DATA_RECEIVER_CAPACITY> collected_data_array_; 
    std::atomic<size_t> head_;         // Read pointer (index of the oldest unconsumed item)
    std::atomic<size_t> tail_;         // Write pointer (index where the next new item should be placed)
    std::atomic<size_t> data_ready_;   // Number of items currently ready for consumption

    mutable std::mutex buffer_mutex_; // Protects access to collected_data_array_, head_, tail_, data_ready_
    std::atomic<bool> successfully_started_; // Tracks if ZMQ setup was okay

    std::thread receiver_thread_;
    std::atomic<bool> running_;          // Controls the receiver loop
};

#endif // DATA_RECEIVER_H


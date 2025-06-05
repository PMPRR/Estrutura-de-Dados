// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <csignal>  // For signal handling (Ctrl+C)
#include <chrono>   // For std::chrono::seconds
#include <thread>   // For std::this_thread::sleep_for
#include "network/data_receiver.h" // Our new class
#include "data.h"         // The Data struct definition from your project

// Global atomic boolean to signal termination for the main loop
std::atomic<bool> keep_running_main(true);

// Signal handler function
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
        keep_running_main = false;
    }
}

int main() {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Configuration for the DataReceiver
    // In your docker-compose.yml, the publisher service is named 'python_publisher'.
    const std::string publisher_address = "tcp://python_publisher:5556"; // Or "tcp://localhost:5556" if running locally
    
    // Option 1: Subscribe to a specific ZMQ topic, e.g., "data_batch"
    // The DataReceiver will then expect messages to be "data_batch <binary_payload>"
    // const std::string zmq_subscription_topic = "data_batch";
    // DataReceiver receiver(publisher_address, zmq_subscription_topic);

    // Option 2: Subscribe to ALL ZMQ messages (empty topic string for ZMQ_SUBSCRIBE)
    // AND specify the prefix within the message payload that identifies your data.
    const std::string zmq_subscription_topic = ""; // Subscribe to all ZMQ messages
    const std::string actual_data_prefix = "data_batch"; // Process messages starting with "data_batch "
    DataReceiver receiver(publisher_address, zmq_subscription_topic, actual_data_prefix);

    // Option 3: Subscribe to a specific ZMQ topic, AND the data payload itself does NOT start with the topic string.
    // This assumes the publisher sends *only* the binary data on this specific topic.
    // const std::string zmq_subscription_topic_for_raw_data = "raw_data_stream";
    // const std::string no_internal_prefix = ""; // Process the entire message as data
    // DataReceiver receiver(publisher_address, zmq_subscription_topic_for_raw_data, no_internal_prefix);


    std::cout << "Attempting to start DataReceiver..." << std::endl;
    if (!receiver.start()) {
        std::cerr << "Failed to start DataReceiver. Exiting." << std::endl;
        return 1;
    }

    std::cout << "DataReceiver started. Main loop running. Press Ctrl+C to exit." << std::endl;

    size_t last_data_count = 0;

    while (keep_running_main.load()) {
        // Sleep for a bit to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (!receiver.isRunning() && keep_running_main.load()) {
            std::cerr << "DataReceiver stopped unexpectedly. Exiting main loop." << std::endl;
            break;
        }

        std::vector<Data> current_data_batch = receiver.getCollectedData(); // Gets a copy

        if (current_data_batch.size() > last_data_count) {
            std::cout << "Main loop: Total data structs collected: " << current_data_batch.size()
                      << " (New: " << current_data_batch.size() - last_data_count << ")" << std::endl;
            
            // You can process the new data here. For example, print details of the latest few.
            // Be mindful that current_data_batch can grow very large.
            // For demonstration, let's just acknowledge new data.
            // If you want to process only new items:
            // for (size_t i = last_data_count; i < current_data_batch.size(); ++i) {
            //     const auto& d = current_data_batch[i];
            //     // std::cout << "  New Data ID: " << d.id << ", Duration: " << d.dur << std::endl;
            // }

            last_data_count = current_data_batch.size();
        } else if (current_data_batch.empty() && last_data_count == 0) {
            // std::cout << "Main loop: No data collected yet." << std::endl;
        }
    }

    std::cout << "Main loop terminating. Stopping DataReceiver..." << std::endl;
    receiver.stop();  // Signal the receiver thread to stop
    receiver.join();  // Wait for the receiver thread to finish

    std::cout << "DataReceiver stopped." << std::endl;
    
    // Final check of collected data
    std::vector<Data> final_data = receiver.getCollectedData();
    std::cout << "Total Data structs collected by the end: " << final_data.size() << std::endl;
    if (!final_data.empty()) {
        std::cout << "Example - First collected Data struct ID: " << final_data.front().id << std::endl;
        std::cout << "Example - Last collected Data struct ID: " << final_data.back().id << std::endl;
    }

    std::cout << "Subscriber shutting down normally." << std::endl;
    return 0;
}

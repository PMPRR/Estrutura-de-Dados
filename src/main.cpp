#include <iostream>     // For std::cout, std::cerr, std::endl
#include <string>       // For std::string, std::stoi, std::stoul
#include <vector>       // For std::vector
#include <csignal>      // For signal, SIGINT, SIGTERM
#include <chrono>       // For std::chrono::seconds, milliseconds
#include <thread>       // For std::this_thread::sleep_for
#include <atomic>       // For std::atomic
#include <sstream>      // For std::ostringstream
#include <iomanip>      // For std::fixed, std::setprecision
#include <algorithm>    // For std::min, std::find_if (included, but not used for query by ID in this version)

#include <zmq.hpp>      // For ZeroMQ C++ bindings (zmq::context_t, zmq::socket_t, zmq::message_t, zmq::error_t)
#include <zmq.h>        // For ZMQ_DONTWAIT (C-style ZMQ constants)

#include "network/data_receiver.h" // Your existing DataReceiver class
#include "data.h"                  // The Data struct definition

// Global atomic boolean to signal termination for all loops
std::atomic<bool> keep_running(true);

// Signal handler function
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
        keep_running = false;
    }
}

// Helper function to format a Data struct into a string
// This function takes a const reference to Data.
std::string format_data_for_reply(const Data& data_item) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2); // Set precision for float values
    oss << "ID: " << data_item.id
        << ", Dur: " << data_item.dur << "s"
        << ", SBytes: " << data_item.sbytes
        << ", DBytes: " << data_item.dbytes
        << ", Rate: " << data_item.rate
        << ", AttackCat: " << static_cast<int>(data_item.attack_category)
        << ", Label: " << (data_item.label ? "Attack" : "Normal");
    return oss.str();
}


int main() {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Setup DataReceiver (Subscriber to python_publisher) ---
    const std::string publisher_address = "tcp://python_publisher:5556";
    const std::string zmq_subscription_topic = "data_batch"; // Explicitly subscribe to 'data_batch'
    const std::string actual_data_prefix = "data_batch"; // DataReceiver expects this prefix for processing

    DataReceiver data_collector(publisher_address, zmq_subscription_topic, actual_data_prefix);

    std::cout << "[DataCollector] Attempting to start..." << std::endl;
    if (!data_collector.start()) {
        std::cerr << "[DataCollector] Failed to start. Exiting." << std::endl;
        return 1;
    }
    std::cout << "[DataCollector] Started successfully." << std::endl;

    // --- Setup ZeroMQ REP Server (for Python GUI) ---
    zmq::context_t rep_context(1);
    zmq::socket_t rep_socket(rep_context, ZMQ_REP);
    const std::string rep_server_bind_address = "tcp://*:5558";

    try {
        std::cout << "[REP Server] Binding to " << rep_server_bind_address << "..." << std::endl;
        rep_socket.bind(rep_server_bind_address);
        std::cout << "[REP Server] Successfully bound. Waiting for requests from GUI..." << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "[REP Server] Failed to bind: " << e.what() << std::endl;
        data_collector.stop();
        data_collector.join();
        return 1;
    }

    // --- Master Data Store: std::vector to hold all collected Data objects ---
    std::vector<Data> master_data_store;
    size_t last_processed_data_collector_count = 0; // To track new data from DataReceiver

    int timeout_log_counter = 0;

    while (keep_running.load()) {
        // --- Process newly received data from DataReceiver ---
        std::vector<Data> received_data_from_collector = data_collector.getCollectedData();

        if (received_data_from_collector.size() > last_processed_data_collector_count) {
            std::cout << "[Main] New data received from DataReceiver. Adding to master data store." << std::endl;
            for (size_t i = last_processed_data_collector_count; i < received_data_from_collector.size(); ++i) {
                master_data_store.push_back(received_data_from_collector[i]);
            }
            last_processed_data_collector_count = received_data_from_collector.size();
            std::cout << "[Main] Master data store size: " << master_data_store.size() << " items." << std::endl;
        }

        // --- Handle GUI requests (REP socket) ---
        zmq::message_t request_msg;
        bool received_rep_request = false;
        try {
            if (rep_socket.recv(&request_msg, ZMQ_DONTWAIT)) {
                 received_rep_request = true;
            } else {
                received_rep_request = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM) {
                std::cerr << "[REP Server] Context terminated during recv, exiting loop." << std::endl;
                break;
            } else if (e.num() == EAGAIN) {
                received_rep_request = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                std::cerr << "[REP Server] recv error: " << e.what() << std::endl;
                received_rep_request = false;
            }
        }

        if (received_rep_request && request_msg.size() > 0) {
            timeout_log_counter = 0;
            std::string request_str(static_cast<char*>(request_msg.data()), request_msg.size());
            std::cout << "[REP Server] Received request: \"" << request_str << "\"" << std::endl;

            std::string reply_str;

            if (request_str == "GET_DATA") {
                if (master_data_store.empty()) {
                    reply_str = "No data collected yet.";
                } else {
                    std::ostringstream oss_reply;
                    oss_reply << "Total data items collected: " << master_data_store.size() << "\n";
                    oss_reply << "--- Last 3 Items (or fewer if less than 3) ---\n";
                    
                    size_t start_index = master_data_store.size() > 3 ? master_data_store.size() - 3 : 0;
                    for (size_t i = start_index; i < master_data_store.size(); ++i) {
                        oss_reply << (i - start_index + 1) << ". " << format_data_for_reply(master_data_store[i]) << "\n";
                    }
                    reply_str = oss_reply.str();
                }
            } else if (request_str.rfind("QUERY_ID ", 0) == 0) {
                // Regardless of the ID, respond with NOT IMPLEMENTED
                reply_str = "QUERY_ID functionality NOT IMPLEMENTED yet.";
            } else if (request_str.rfind("REMOVE_DATA ", 0) == 0) {
                reply_str = "REMOVE_DATA functionality NOT IMPLEMENTED yet.";
            } else if (request_str.rfind("GET_STATS ", 0) == 0) {
                reply_str = "GET_STATS functionality NOT IMPLEMENTED yet.";
            } else {
                reply_str = "Unknown command: " + request_str;
            }

            zmq::message_t reply_msg(reply_str.data(), reply_str.size());
            try {
                rep_socket.send(reply_msg, 0);
            } catch(const zmq::error_t& e) {
                 std::cerr << "[REP Server] send error: " << e.what() << std::endl;
            }
            std::cout << "[REP Server] Sent reply for request: \"" << request_str << "\"" << std::endl;
        } else if (!received_rep_request) {
            if (timeout_log_counter < 5 || timeout_log_counter % 60 == 0) {
                 // Debug: std::cout << "[Debug] REP socket: No GUI request. Loop count: " << timeout_log_counter << std::endl;
            }
            timeout_log_counter++;
        }
    }

    std::cout << "[REP Server] Shutting down..." << std::endl;
    std::cout << "[REP Server] Resources will be released." << std::endl;

    std::cout << "[DataCollector] Signaling to stop..." << std::endl;
    data_collector.stop();
    data_collector.join();
    std::cout << "[DataCollector] Stopped." << std::endl;

    std::cout << "Application shut down gracefully." << std::endl;
    return 0;
}

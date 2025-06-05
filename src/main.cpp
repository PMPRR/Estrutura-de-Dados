#include <iostream>
#include <string>
#include <vector>
#include <csignal>  // For signal handling (Ctrl+C)
#include <chrono>   // For std::chrono::seconds
#include <thread>   // For std::this_thread::sleep_for
#include <atomic>
#include <sstream>  // For formatting string replies
#include <iomanip>  // For std::fixed, std::setprecision

#include <zmq.hpp>  // ZeroMQ C++ bindings (cppzmq)
// It's good practice to also include the C header if using C-style constants directly
// and cppzmq doesn't explicitly wrap them all in a way your version expects.
#include <zmq.h>    // For ZMQ_RCVTIMEO, ZMQ_DONTWAIT etc.

#include "network/data_receiver.h" // Your existing DataReceiver class
#include "data.h"                  // The Data struct definition

// Global atomic boolean to signal termination for all loops
std::atomic<bool> keep_running(true); // Ensure this is globally defined

// Signal handler function
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
        keep_running = false;
    }
}

// Helper function to format a Data struct into a string
// Ensure this function is defined or declared before main
std::string format_data_for_reply(const Data& data_item) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2); // Set precision for float values
    oss << "ID: " << data_item.id
        << ", Dur: " << data_item.dur << "s"
        // Add more fields from the Data struct as needed for detailed printing
        // Example:
        // << ", Proto: " << static_cast<int>(data_item.proto) 
        // << ", Service: " << static_cast<int>(data_item.service)
        // << ", State: " << static_cast<int>(data_item.state)
        << ", SrcBytes: " << data_item.sbytes
        << ", DstBytes: " << data_item.dbytes
        << ", Rate: " << data_item.rate
        << ", AttackCat: " << static_cast<int>(data_item.attack_category) // Cast enums to int for printing
        << ", Label: " << (data_item.label ? "Attack" : "Normal");
    return oss.str();
}


int main() {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Setup DataReceiver (Subscriber to python_publisher) ---
    const std::string publisher_address = "tcp://python_publisher:5556";
    const std::string zmq_subscription_topic = ""; 
    const std::string actual_data_prefix = "data_batch"; 
    
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

    size_t last_known_data_count = 0;
    int timeout_log_counter = 0; 

    while (keep_running.load()) {
        zmq::message_t request_msg;
        
        bool received_rep_request = false;
        try {
            // Use ZMQ_DONTWAIT for non-blocking receive.
            // Pass the address of request_msg for the bool zmq::socket_t::recv(zmq::message_t*, int) overload
            if (rep_socket.recv(&request_msg, ZMQ_DONTWAIT)) { 
                 received_rep_request = true;
            } else {
                // No message received immediately with ZMQ_DONTWAIT
                received_rep_request = false;
                // Yield CPU briefly to avoid busy-waiting in the loop when no GUI requests
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            }
        } catch (const zmq::error_t& e) { 
            if (e.num() == ETERM) {
                std::cout << "[REP Server] Context terminated during recv. Exiting loop." << std::endl;
                break; 
            } else if (e.num() == EAGAIN) { 
                // EAGAIN can also mean no message when ZMQ_DONTWAIT is used.
                received_rep_request = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Ensure sleep if EAGAIN means no message
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
                std::vector<Data> current_data_batch = data_collector.getCollectedData();
                if (current_data_batch.empty()) {
                    reply_str = "No data collected yet.";
                } else {
                    std::ostringstream oss_reply;
                    oss_reply << "Total data items collected: " << current_data_batch.size() << "\n";
                    oss_reply << "--- Last 3 Items (or fewer if less than 3) ---\n";
                    
                    size_t start_index = current_data_batch.size() > 3 ? current_data_batch.size() - 3 : 0;
                    for (size_t i = start_index; i < current_data_batch.size(); ++i) {
                        oss_reply << (i - start_index + 1) << ". " << format_data_for_reply(current_data_batch[i]) << "\n";
                    }
                    reply_str = oss_reply.str();
                }
            } else if (request_str == "GET_TOTAL_COUNT") {
                 std::vector<Data> current_data_batch = data_collector.getCollectedData(); 
                 reply_str = "Total data items collected: " + std::to_string(current_data_batch.size());
            }
            else {
                reply_str = "Unknown command: " + request_str;
            }

            zmq::message_t reply_msg(reply_str.data(), reply_str.size());
            try {
                // Use 0 for no flags (equivalent to zmq::send_flags::none)
                rep_socket.send(reply_msg, 0); 
            } catch(const zmq::error_t& e) {
                 std::cerr << "[REP Server] send error: " << e.what() << std::endl;
            }
            std::cout << "[REP Server] Sent reply for request: \"" << request_str << "\"" << std::endl;
        } else if (!received_rep_request) { 
            if (timeout_log_counter < 5 || timeout_log_counter % 60 == 0) { // Log first 5 timeouts, then every 60th
                 std::cout << "[Debug] REP socket: No GUI request. Checking for GAN data. Loop count: " << timeout_log_counter << std::endl;
            }
            timeout_log_counter++;
            
            std::vector<Data> current_data_from_gan = data_collector.getCollectedData();
            
            // UNCOMMENTED DIAGNOSTIC LINE:
//            if (timeout_log_counter % 5 == 0 || current_data_from_gan.size() != last_known_data_count) { 
//                 std::cout << "[Debug] DataCollector Check: Current items from GAN = " << current_data_from_gan.size() 
//                           << ", Last known count = " << last_known_data_count << std::endl;
//            }
//
          //  if (current_data_from_gan.size() > last_known_data_count) {
          //      timeout_log_counter = 0; 
          //      std::cout << "\n*****************************************************" << std::endl;
          //      std::cout << "* [GAN Data Monitor] New data received and processed! *" << std::endl;
          //      std::cout << "*****************************************************" << std::endl;
          //      std::cout << "Previous total items: " << last_known_data_count << std::endl;
          //      std::cout << "Current total items: " << current_data_from_gan.size() << std::endl;
          //      std::cout << "Newly processed items in this batch (" << current_data_from_gan.size() - last_known_data_count << "):" << std::endl;
          //      
          //      for (size_t i = last_known_data_count; i < current_data_from_gan.size(); ++i) {
          //          if (i < current_data_from_gan.size()){ 
          //               std::cout << "  - " << format_data_for_reply(current_data_from_gan[i]) << std::endl;
          //          }
          //      }
          //      last_known_data_count = current_data_from_gan.size(); 
          //      std::cout << "*****************************************************\n" << std::endl;
          //  }
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


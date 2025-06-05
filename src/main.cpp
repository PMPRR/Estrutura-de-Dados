#include <iostream>     // For std::cout, std::cerr, std::endl
#include <string>       // For std::string, std::stoi, std::stoul
#include <vector>       // For std::vector
#include <csignal>      // For signal, SIGINT, SIGTERM
#include <chrono>       // For std::chrono::seconds, milliseconds
#include <thread>       // For std::this_thread::sleep_for
#include <atomic>       // For std::atomic
#include <sstream>      // For std::ostringstream
#include <iomanip>      // For std::fixed, std::setprecision
#include <algorithm>    // For std::min, std::remove_if
#include <cstring>      // For strlen, strncmp
#include <memory>       // For std::unique_ptr, std::make_unique

#include <zmq.hpp>      // For ZeroMQ C++ bindings (zmq::context_t, zmq::socket_t, zmq::message_t, zmq::error_t)
#include <zmq.h>        // For ZMQ_DONTWAIT (C-style ZMQ constants)

#include "network/data_receiver.h" // Your existing DataReceiver class
#include "data.h"                  // The Data struct definition
#include "essential/AVL.h"         // Include for AVL tree
#include "essential/LinkedList.h"  // Include for DoublyLinkedList

// Global atomic boolean to signal termination for all loops
std::atomic<bool> keep_running(true);

// Signal handler function
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
        keep_running = false;
    }
}

// Helper function to format a Data struct into a concise string (for last 3 items display)
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

// New helper function to format a Data struct into a detailed table string
std::string format_data_as_table(const Data& data_item) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4); // Use higher precision for floats

    oss << "--- Data Record Details (ID: " << data_item.id << ") ---\n";
    oss << "ID: " << data_item.id << "\n";
    oss << "Duration (s): " << data_item.dur << "\n";
    oss << "Rate (pkts/s): " << data_item.rate << "\n";
    oss << "Source Load (bytes/s): " << data_item.sload << "\n";
    oss << "Dest Load (bytes/s): " << data_item.dload << "\n";
    oss << "Source Inter-arrival (s): " << data_item.sinpkt << "\n";
    oss << "Dest Inter-arrival (s): " << data_item.dinpkt << "\n";
    oss << "Source Jitter (ms): " << data_item.sjit << "\n";
    oss << "Dest Jitter (ms): " << data_item.djit << "\n";
    oss << "TCP RTT (s): " << data_item.tcprtt << "\n";
    oss << "SYN-ACK Time (s): " << data_item.synack << "\n";
    oss << "ACK/Data Time (s): " << data_item.ackdat << "\n";
    oss << "Source Packets: " << data_item.spkts << "\n";
    oss << "Dest Packets: " << data_item.dpkts << "\n";
    oss << "Source Bytes: " << data_item.sbytes << "\n";
    oss << "Dest Bytes: " << data_item.dbytes << "\n";
    oss << "Source TTL: " << static_cast<int>(data_item.sttl) << "\n";
    oss << "Dest TTL: " << static_cast<int>(data_item.dttl) << "\n";
    oss << "Source Loss: " << data_item.sloss << "\n";
    oss << "Dest Loss:  " << data_item.dloss << "\n"; // Added space for alignment
    oss << "Source Win: " << data_item.swin << "\n";
    oss << "Source TCP Seq Base: " << data_item.stcpb << "\n";
    oss << "Dest TCP Seq Base: " << data_item.dtcpb << "\n";
    oss << "Dest Win: " << data_item.dwin << "\n";
    oss << "Source Mean Packet Size: " << data_item.smean << "\n";
    oss << "Dest Mean Packet Size: " << data_item.dmean << "\n";
    oss << "Transaction Depth: " << data_item.trans_depth << "\n";
    oss << "Response Body Length: " << data_item.response_body_len << "\n";
    oss << "Ct Srv Src: " << data_item.ct_srv_src << "\n";
    oss << "Ct Dst LTM: " << data_item.ct_dst_ltm << "\n";
    oss << "Ct Src Dport LTM: " << data_item.ct_src_dport_ltm << "\n";
    oss << "Ct Dst Sport LTM: " << data_item.ct_dst_sport_ltm << "\n";
    oss << "Ct Dst Src LTM: " << data_item.ct_dst_src_ltm << "\n";
    oss << "Ct FTP Cmd: " << data_item.ct_ftp_cmd << "\n";
    oss << "Ct FLW HTTP Mthd: " << data_item.ct_flw_http_mthd << "\n";
    oss << "Ct Src LTM: " << data_item.ct_src_ltm << "\n";
    oss << "Ct Srv Dst: " << data_item.ct_srv_dst << "\n";
    oss << "Is FTP Login: " << (data_item.is_ftp_login ? "True" : "False") << "\n";
    oss << "Is SM IPs Ports: " << (data_item.is_sm_ips_ports ? "True" : "False") << "\n";
    oss << "Label (Attack): " << (data_item.label ? "True" : "False") << "\n";
    oss << "Protocolo: " << static_cast<int>(data_item.proto) << " (Enum Value)\n";
    oss << "State: " << static_cast<int>(data_item.state) << " (Enum Value)\n";
    oss << "Attack Category: " << static_cast<int>(data_item.attack_category) << " (Enum Value)\n";
    oss << "Service: " << static_cast<int>(data_item.service) << " (Enum Value)\n";

    return oss.str();
}


int main() {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Instantiate Data Structures ---
    AVL avl_tree;
    DoublyLinkedList doubly_linked_list; // Instantiate DoublyLinkedList
    const int AVL_DS_ID = 1;             // Corresponds to "AVL" in gui.py's data_structure_map
    const int LINKED_LIST_DS_ID = 2;     // Corresponds to "LINKED_LIST" in gui.py's data_structure_map


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

    // --- Master Data Store: std::vector of unique_ptrs to hold all collected Data objects ---
    // This vector will own the Data objects, ensuring their memory addresses are stable.
    std::vector<std::unique_ptr<Data>> master_data_store;
    size_t last_processed_data_collector_count = 0; // To track new data from DataReceiver

    int timeout_log_counter = 0;

    while (keep_running.load()) {
        // --- Process newly received data from DataReceiver ---
        // Get a shallow view (pointer and count) of the data from DataReceiver's internal array.
        std::pair<const Data*, size_t> received_data_view = data_collector.getCollectedData();
        const Data* received_data_ptr = received_data_view.first;
        size_t current_received_count = received_data_view.second;

        // Update master_data_store AND AVL tree & LinkedList (only if there's new data)
        if (current_received_count > last_processed_data_collector_count) {
            std::cout << "[Main] New data received from DataReceiver. Adding to master data store, AVL tree, and Linked List." << std::endl;
            for (size_t i = last_processed_data_collector_count; i < current_received_count; ++i) {
                // Deep copy the Data object from DataReceiver's internal array
                // and store it in a unique_ptr within master_data_store.
                master_data_store.push_back(std::make_unique<Data>(received_data_ptr[i]));
                // Get raw pointer to this *stably allocated* Data object.
                Data* data_to_insert = master_data_store.back().get();

                // Insert into AVL tree
                avl_tree.insert(data_to_insert); 
                // Insert into DoublyLinkedList
                doubly_linked_list.append(data_to_insert);

            }
            last_processed_data_collector_count = current_received_count;
            std::cout << "[Main] Master data store size: " << master_data_store.size() << " items." << std::endl;
            std::cout << "[Main] AVL tree and Linked List updated with new data." << std::endl;
        }

        // --- Handle GUI requests (REP socket) ---
        zmq::message_t request_msg;
        bool received_rep_request = false;
        try {
            // Non-blocking receive (ZMQ_DONTWAIT)
            if (rep_socket.recv(&request_msg, ZMQ_DONTWAIT)) {
                 received_rep_request = true;
            } else {
                received_rep_request = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep briefly if no message
            }
        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM) {
                std::cerr << "[REP Server] Context terminated during recv, exiting loop." << std::endl;
                break;
            } else if (e.num() == EAGAIN) { // No message received
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

            if (request_str == "GET_DATA") { // Original "Query Last 3 Data Items"
                if (master_data_store.empty()) {
                    reply_str = "No data collected yet.";
                } else {
                    std::ostringstream oss_reply;
                    oss_reply << "Total data items collected: " << master_data_store.size() << "\n";
                    oss_reply << "--- Last 3 Items (or fewer if less than 3) ---\n";
                    
                    size_t start_index = master_data_store.size() > 3 ? master_data_store.size() - 3 : 0;
                    for (size_t i = start_index; i < master_data_store.size(); ++i) {
                        // Access Data object through unique_ptr
                        oss_reply << (i - start_index + 1) << ". " << format_data_for_reply(*master_data_store[i]) << "\n";
                    }
                    reply_str = oss_reply.str();
                }
            } else if (request_str.rfind("QUERY_DATA_BY_ID ", 0) == 0) { 
                size_t prefix_len = strlen("QUERY_DATA_BY_ID "); 
                std::string remaining_str = request_str.substr(prefix_len); 

                size_t space_pos = remaining_str.find(' ');

                uint32_t id_to_query = 0;
                int ds_id = 0;

                bool parse_success = true;
                if (space_pos == std::string::npos) {
                    reply_str = "Error: QUERY_DATA_BY_ID requires both an ID and a Data Structure ID.";
                    parse_success = false;
                } else {
                    std::string id_str = remaining_str.substr(0, space_pos); 
                    std::string ds_id_str = remaining_str.substr(space_pos + 1); 

                    try {
                        id_to_query = std::stoul(id_str); 
                        ds_id = std::stoi(ds_id_str);     
                    } catch (const std::exception& e) {
                        reply_str = "Error parsing ID or Data Structure ID for QUERY_DATA_BY_ID: " + std::string(e.what());
                        parse_success = false;
                    }
                }

                if (parse_success) {
                    if (ds_id == AVL_DS_ID) { // Check if the requested DS is AVL
                        AVL::Node_AVL* found_node = avl_tree.queryById(id_to_query);
                        if (found_node != nullptr && found_node->data != nullptr) {
                            reply_str = format_data_as_table(*found_node->data);
                        } else {
                            reply_str = "No data with ID " + std::to_string(id_to_query) + " found in AVL tree.";
                        }
                    } else if (ds_id == LINKED_LIST_DS_ID) { // Check if the requested DS is Linked List
                        const Data* found_data = doubly_linked_list.findById(id_to_query);
                        if (found_data != nullptr) {
                            reply_str = format_data_as_table(*found_data);
                        } else {
                            reply_str = "No data with ID " + std::to_string(id_to_query) + " found in Linked List.";
                        }
                    }
                    else {
                        reply_str = "Query by ID for Data Structure ID " + std::to_string(ds_id) + " (other than AVL/LinkedList) NOT IMPLEMENTED yet.";
                    }
                }
            } else if (request_str.rfind("REMOVE_DATA_BY_ID ", 0) == 0) {
                // Expected format: "REMOVE_DATA_BY_ID <ID> <DS_ID>"
                size_t prefix_len = strlen("REMOVE_DATA_BY_ID "); 
                std::string remaining_str = request_str.substr(prefix_len); 

                size_t space_pos = remaining_str.find(' ');

                uint32_t id_to_remove = 0;
                int ds_id = 0;

                bool parse_success = true;
                if (space_pos == std::string::npos) {
                    reply_str = "Error: REMOVE_DATA_BY_ID requires both an ID and a Data Structure ID.";
                    parse_success = false;
                } else {
                    std::string id_str = remaining_str.substr(0, space_pos); 
                    std::string ds_id_str = remaining_str.substr(space_pos + 1); 

                    try {
                        id_to_remove = std::stoul(id_str); 
                        ds_id = std::stoi(ds_id_str);     
                    } catch (const std::exception& e) {
                        reply_str = "Error parsing ID or Data Structure ID for REMOVE_DATA_BY_ID: " + std::string(e.what());
                        parse_success = false;
                    }
                }

                if (parse_success) {
                    bool removed_from_ds = false;
                    if (ds_id == AVL_DS_ID) { // Remove from AVL
                        AVL::Node_AVL* found_node_before_remove = avl_tree.queryById(id_to_remove);
                        if (found_node_before_remove && found_node_before_remove->data) {
                            avl_tree.removeById(id_to_remove);
                            removed_from_ds = true;
                        }
                    } else if (ds_id == LINKED_LIST_DS_ID) { // Remove from Linked List
                        if (doubly_linked_list.findById(id_to_remove) != nullptr) {
                            doubly_linked_list.removeById(id_to_remove);
                            removed_from_ds = true;
                        }
                    } else {
                        reply_str = "Remove by ID for Data Structure ID " + std::to_string(ds_id) + " (other than AVL/LinkedList) NOT IMPLEMENTED yet.";
                    }

                    if (removed_from_ds) {
                        // Find and remove the unique_ptr from master_data_store
                        // This uses std::remove_if with a lambda that captures id_to_remove.
                        // It moves elements to the end and returns an iterator to the new end.
                        // Then erase removes the range.
                        auto it = std::remove_if(master_data_store.begin(), master_data_store.end(),
                            [&](const std::unique_ptr<Data>& data_ptr) {
                                return data_ptr->id == id_to_remove;
                            });
                        if (it != master_data_store.end()) {
                            master_data_store.erase(it, master_data_store.end());
                            reply_str = "Data with ID " + std::to_string(id_to_remove) + " successfully removed from selected data structure and master store.";
                            std::cout << "[Main] Data with ID " << id_to_remove << " removed." << std::endl;
                        } else {
                            // This case implies data was in DS but not in master_data_store, which is an inconsistency.
                            reply_str = "Error: Data with ID " + std::to_string(id_to_remove) + " removed from data structure but not found in master store for deletion.";
                            std::cerr << "[Main] Inconsistency: Data with ID " << id_to_remove << " removed from DS but not in master_data_store." << std::endl;
                        }
                    } else if (reply_str.empty()) { // If not removed and no other error message set
                        reply_str = "No data with ID " + std::to_string(id_to_remove) + " found in selected data structure to remove.";
                    }
                }
            } else if (request_str.rfind("GET_STATS_SLOAD ", 0) == 0 || request_str.rfind("GET_STATS_CATEGORY_BREAKDOWN ", 0) == 0) { 
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
            // Log timeout messages less frequently to avoid excessive console output
            if (timeout_log_counter < 5 || timeout_log_counter % 60 == 0) {
                 // std::cout << "[Debug] REP socket: No GUI request. Loop count: " << timeout_log_counter << std::endl;
            }
            timeout_log_counter++;
        }
    }

    std::cout << "[REP Server] Shutting down..." << std::endl;
    // The zmq objects (rep_context, rep_socket) will be destroyed automatically when main exits.
    std::cout << "[REP Server] Resources will be released." << std::endl;

    std::cout << "[DataCollector] Signaling to stop..." << std::endl;
    data_collector.stop();
    data_collector.join();
    std::cout << "[DataCollector] Stopped." << std::endl;

    std::cout << "Application shut down gracefully." << std::endl;
    return 0;
}


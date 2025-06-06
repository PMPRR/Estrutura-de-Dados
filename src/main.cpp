#include <iostream>     // For std::cout, std::cerr, std::endl
#include <string>       // For std::string, std::stoi, std::stoul
#include <vector>       // For std::vector
#include <csignal>      // For signal, SIGINT, SIGTERM
#include <chrono>       // For std::chrono::seconds, milliseconds
#include <thread>       // For std::this_thread::sleep_for
#include <atomic>       // For std::atomic
#include <sstream>      // For std::ostringstream
#include <iomanip>      // For std::fixed, std::setprecision
#include <algorithm>    // For std::min, std::remove_if, std::sort, std::accumulate, std::min_element, std::max_element
#include <numeric>      // For std::accumulate
#include <cmath>        // For std::sqrt
#include <cstring>      // For strlen, strncmp
#include <memory>       // For std::unique_ptr, std::make_unique

#include <zmq.hpp>      // For ZeroMQ C++ bindings (zmq::context_t, zmq::socket_t, zmq::message_t, zmq::error_t)
#include <zmq.h>        // For ZMQ_DONTWAIT (C-style ZMQ constants)

#include "network/data_receiver.h" // Your existing DataReceiver class
#include "data.h"                  // The Data struct definition
#include "essential/AVL.h"         // Include for AVL tree
#include "essential/LinkedList.h"  // Include for DoublyLinkedList
#include "essential/HashTable.h"   // Include for Chaining HashTable
#include "extra/CuckooHashTable.h" // Include for CuckooHashTable
#include "extra/SegmentTree.h"     // Include for SegmentTree

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
    oss << "SYN-ACK Time (synack) (s): " << data_item.synack << "\n"; 
    oss << "ACK/Data Time (ackdat) (s): " << data_item.ackdat << "\n"; 
    oss << "Source Packets:  " << data_item.spkts << "\n";
    oss << "Dest Packets: " << data_item.dpkts << "\n";
    oss << "Source Bytes: " << data_item.sbytes << "\n";
    oss << "Dest Bytes: " << data_item.dbytes << "\n";
    oss << "Source TTL: " << static_cast<int>(data_item.sttl) << "\n";
    oss << "Dest TTL: " << static_cast<int>(data_item.dttl) << "\n";
    oss << "Source Loss: " << data_item.sloss << "\n";
    oss << "Dest Loss:  " << data_item.dloss << "\n";
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

// Helper function to get a string representation of the data structure name
std::string get_ds_name_by_id(int ds_id) {
    switch (ds_id) {
        case 1: return "AVL Tree";
        case 2: return "Linked List";
        case 3: return "Hash Table";
        case 4: return "Cuckoo Hash Table";
        case 5: return "Segment Tree";
        // Add other cases as needed
        default: return "Unknown Data Structure";
    }
}


// Helper function to calculate and format statistics for a given feature and interval using DoublyLinkedList
std::string get_stats_for_feature_linkedlist(DoublyLinkedList& list, StatisticFeature feature, int interval_count) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    if (list.size() == 0) {
        return "No data in Linked List to calculate statistics.";
    }

    std::string feature_name;
    // Determine feature name for display
    switch (feature) {
        case StatisticFeature::DUR: feature_name = "Duration (dur)"; break;
        case StatisticFeature::RATE: feature_name = "Rate"; break;
        case StatisticFeature::SLOAD: feature_name = "Source Load (sload)"; break;
        case StatisticFeature::DLOAD: feature_name = "Destination Load (dload)"; break;
        case StatisticFeature::SPKTS: feature_name = "Source Packets (spkts)"; break;
        case StatisticFeature::DPKTS: feature_name = "Destination Packets (dpkts)"; break;
        case StatisticFeature::SBYTES: feature_name = "Source Bytes (sbytes)"; break;
        case StatisticFeature::DBYTES: feature_name = "Destination Bytes (dbytes)"; break;
        default: feature_name = "Unknown Feature"; break;
    }

    float avg = list.getAverage(feature, interval_count);
    float stddev = list.getStdDev(feature, interval_count);
    float median = list.getMedian(feature, interval_count);
    float min_val = list.getMin(feature, interval_count);
    float max_val = list.getMax(feature, interval_count);

    oss << "--- Statistics for " << feature_name << " (last " << interval_count << " items) from Linked List ---\n";
    oss << "  Total data points considered: " << std::min(list.size(), interval_count) << "\n"; 
    oss << "  Average: " << avg << "\n";
    oss << "  Standard Deviation: " << stddev << "\n";
    oss << "  Median: " << median << "\n";
    oss << "  Minimum: " << min_val << "\n";
    oss << "  Maximum: " << max_val << "\n";

    return oss.str();
}

// Helper function to calculate and format statistics for a given feature and interval using SegmentTree
std::string get_stats_for_feature_segmenttree(const SegmentTree& tree, StatisticFeature feature, int interval_count) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    // SegmentTree might not have a direct "size" like a list for total items,
    // but its statistical methods should operate on the 'interval_count' of most recent items implicitly.
    // The current SegmentTree implementation collects all pointers and then filters.
    // We rely on the SegmentTree's internal logic for `collectFeatureValuesForInterval`.

    std::string feature_name;
    // Determine feature name for display
    switch (feature) {
        case StatisticFeature::DUR: feature_name = "Duration (dur)"; break;
        case StatisticFeature::RATE: feature_name = "Rate"; break;
        case StatisticFeature::SLOAD: feature_name = "Source Load (sload)"; break;
        case StatisticFeature::DLOAD: feature_name = "Destination Load (dload)"; break;
        case StatisticFeature::SPKTS: feature_name = "Source Packets (spkts)"; break;
        case StatisticFeature::DPKTS: feature_name = "Destination Packets (dpkts)"; break;
        case StatisticFeature::SBYTES: feature_name = "Source Bytes (sbytes)"; break;
        case StatisticFeature::DBYTES: feature_name = "Destination Bytes (dbytes)"; break;
        default: feature_name = "Unknown Feature"; break;
    }

    float avg = tree.getAverage(feature, interval_count);
    float stddev = tree.getStdDev(feature, interval_count);
    float median = tree.getMedian(feature, interval_count);
    float min_val = tree.getMin(feature, interval_count);
    float max_val = tree.getMax(feature, interval_count);
    
    // To display "Total data points considered", we need a way to know how many items
    // the SegmentTree considered for the interval. The current `collectFeatureValuesForInterval`
    // gets all, then sub-selects. We can replicate that logic here for display if needed, or modify SegmentTree.
    std::vector<float> temp_values = tree.collectFeatureValuesForInterval(feature, interval_count);


    oss << "--- Statistics for " << feature_name << " (last " << interval_count << " items) from Segment Tree ---\n";
    oss << "  Total data points considered for interval: " << temp_values.size() << "\n";
    oss << "  Average: " << avg << "\n";
    oss << "  Standard Deviation: " << stddev << "\n";
    oss << "  Median: " << median << "\n";
    oss << "  Minimum: " << min_val << "\n";
    oss << "  Maximum: " << max_val << "\n";

    return oss.str();
}


int main() {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Instantiate Data Structures ---
    AVL avl_tree;
    DoublyLinkedList doubly_linked_list; 
    HashTable hash_table;                
    CuckooHashTable cuckoo_hash_table;   
    SegmentTree segment_tree;            // Instantiate SegmentTree
    
    const int AVL_DS_ID = 1;             
    const int LINKED_LIST_DS_ID = 2;     
    const int HASHTABLE_DS_ID = 3;       
    const int CUCKOO_HASH_DS_ID = 4;     
    const int SEGMENT_TREE_DS_ID = 5;    // Define Segment Tree ID


    // --- Setup DataReceiver (Subscriber to python_publisher) ---
    const std::string publisher_address = "tcp://python_publisher:5556";
    const std::string zmq_subscription_topic = "data_batch"; 
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

    // --- Master Data Store: std::vector of unique_ptrs to hold all collected Data objects ---
    std::vector<std::unique_ptr<Data>> master_data_store;
    size_t last_processed_data_collector_count = 0; 

    int timeout_log_counter = 0;

    while (keep_running.load()) {
        // --- Process newly received data from DataReceiver ---
        std::pair<const Data*, size_t> received_data_view = data_collector.getCollectedData();
        const Data* received_data_ptr = received_data_view.first;
        size_t current_received_count = received_data_view.second;

        if (current_received_count > last_processed_data_collector_count) {
            std::cout << "[Main] New data received. Adding to master store and data structures." << std::endl;
            for (size_t i = last_processed_data_collector_count; i < current_received_count; ++i) {
                master_data_store.push_back(std::make_unique<Data>(received_data_ptr[i]));
                Data* data_to_insert = master_data_store.back().get();

                avl_tree.insert(data_to_insert); 
                doubly_linked_list.append(data_to_insert);
                hash_table.insert(data_to_insert);
                cuckoo_hash_table.insert(data_to_insert);
                segment_tree.insert(data_to_insert); // Insert into Segment Tree

            }
            last_processed_data_collector_count = current_received_count;
            std::cout << "[Main] Master data store size: " << master_data_store.size() << " items." << std::endl;
            std::cout << "[Main] Data structures updated." << std::endl;
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
                    const Data* found_data_ptr = nullptr;
                    if (ds_id == AVL_DS_ID) {
                        AVL::Node_AVL* found_node = avl_tree.queryById(id_to_query);
                        if (found_node) found_data_ptr = found_node->data;
                    } else if (ds_id == LINKED_LIST_DS_ID) { 
                        found_data_ptr = doubly_linked_list.findById(id_to_query);
                    } else if (ds_id == HASHTABLE_DS_ID) { 
                        found_data_ptr = hash_table.find(id_to_query);
                    } else if (ds_id == CUCKOO_HASH_DS_ID) { 
                        found_data_ptr = cuckoo_hash_table.search(id_to_query);
                    } else if (ds_id == SEGMENT_TREE_DS_ID) { // Query Segment Tree
                        found_data_ptr = segment_tree.find(id_to_query);
                    } else {
                        reply_str = "Query by ID for Data Structure ID " + std::to_string(ds_id) + " NOT IMPLEMENTED yet.";
                    }

                    if (found_data_ptr != nullptr) {
                        reply_str = format_data_as_table(*found_data_ptr);
                    } else if (reply_str.empty()) { // Only set if no other error/message
                        reply_str = "No data with ID " + std::to_string(id_to_query) + " found in " + get_ds_name_by_id(ds_id) + ".";
                    }
                }
            } else if (request_str.rfind("REMOVE_DATA_BY_ID ", 0) == 0) {
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
                    if (ds_id == AVL_DS_ID) { 
                        if (avl_tree.queryById(id_to_remove) != nullptr) {
                           avl_tree.removeById(id_to_remove); removed_from_ds = true;
                        }
                    } else if (ds_id == LINKED_LIST_DS_ID) { 
                        if (doubly_linked_list.findById(id_to_remove) != nullptr) {
                           doubly_linked_list.removeById(id_to_remove); removed_from_ds = true;
                        }
                    } else if (ds_id == HASHTABLE_DS_ID) { 
                         if (hash_table.find(id_to_remove) != nullptr) {
                            hash_table.remove(id_to_remove); removed_from_ds = true;
                        }
                    } else if (ds_id == CUCKOO_HASH_DS_ID) { 
                         if (cuckoo_hash_table.search(id_to_remove) != nullptr) {
                            cuckoo_hash_table.remove(id_to_remove); removed_from_ds = true;
                        }
                    } else if (ds_id == SEGMENT_TREE_DS_ID) { // Remove from Segment Tree
                        if (segment_tree.find(id_to_remove) != nullptr) { // Check if exists before removing
                            segment_tree.remove(id_to_remove);
                            removed_from_ds = true;
                        }
                    } else {
                        reply_str = "Remove by ID for Data Structure ID " + std::to_string(ds_id) + " NOT IMPLEMENTED yet.";
                    }

                    if (removed_from_ds) {
                        auto it = std::remove_if(master_data_store.begin(), master_data_store.end(),
                            [&](const std::unique_ptr<Data>& data_ptr) {
                                return data_ptr && data_ptr->id == id_to_remove; // Added null check for data_ptr
                            });
                        if (it != master_data_store.end()) {
                            master_data_store.erase(it, master_data_store.end());
                            reply_str = "Data with ID " + std::to_string(id_to_remove) + " successfully removed from " + get_ds_name_by_id(ds_id) + " and master store.";
                            std::cout << "[Main] Data with ID " << id_to_remove << " removed." << std::endl;
                        } else {
                            reply_str = "Error: Data with ID " + std::to_string(id_to_remove) + " removed from " + get_ds_name_by_id(ds_id) + " but not found in master store for full deletion.";
                            std::cerr << "[Main] Inconsistency: Data with ID " << id_to_remove << " removed from DS but not in master_data_store." << std::endl;
                        }
                    } else if (reply_str.empty()) { 
                        reply_str = "No data with ID " + std::to_string(id_to_remove) + " found in " + get_ds_name_by_id(ds_id) + " to remove.";
                    }
                }
            } else if (request_str.rfind("GET_STATS_SUMMARY ", 0) == 0) {
                size_t prefix_len = strlen("GET_STATS_SUMMARY ");
                std::string remaining_str = request_str.substr(prefix_len);
                size_t first_space = remaining_str.find(' ');
                size_t second_space = remaining_str.find(' ', first_space + 1);
                int feature_enum_val = 0; 
                int interval = 0;
                int ds_id = 0;
                bool parse_success = true;

                if (first_space == std::string::npos || second_space == std::string::npos) {
                    reply_str = "Error: GET_STATS_SUMMARY requires FEATURE_ENUM_VALUE, INTERVAL, and DS_ID.";
                    parse_success = false;
                } else {
                    std::string feature_enum_str = remaining_str.substr(0, first_space);
                    std::string interval_str = remaining_str.substr(first_space + 1, second_space - (first_space + 1));
                    std::string ds_id_str = remaining_str.substr(second_space + 1);
                    try {
                        feature_enum_val = std::stoi(feature_enum_str);
                        interval = std::stoi(interval_str);
                        ds_id = std::stoi(ds_id_str);
                    } catch (const std::exception& e) {
                        reply_str = "Error parsing FEATURE_ENUM_VALUE, INTERVAL, or DS_ID for GET_STATS_SUMMARY: " + std::string(e.what());
                        parse_success = false;
                    }
                }

                if (parse_success) {
                    StatisticFeature selected_feature = static_cast<StatisticFeature>(feature_enum_val);
                    if (ds_id == LINKED_LIST_DS_ID) {
                        reply_str = get_stats_for_feature_linkedlist(doubly_linked_list, selected_feature, interval); 
                    } else if (ds_id == SEGMENT_TREE_DS_ID) { // Statistics from Segment Tree
                         reply_str = get_stats_for_feature_segmenttree(segment_tree, selected_feature, interval);
                    }
                     else {
                        reply_str = "Statistics for " + get_ds_name_by_id(ds_id) + " NOT IMPLEMENTED yet.";
                    }
                }
            } else if (request_str.rfind("GET_STATS_CATEGORY_BREAKDOWN ", 0) == 0) { 
                reply_str = "GET_STATS_CATEGORY_BREAKDOWN functionality NOT IMPLEMENTED yet.";
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
            if (timeout_log_counter < 5 || timeout_log_counter % 600 == 0) { // Log less frequently
                 // std::cout << "[Debug] REP socket: No GUI request. Loop count: " << timeout_log_counter << std::endl;
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


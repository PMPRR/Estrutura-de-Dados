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
#include <map>          // For parsing query parameters
#include <unordered_map> // For the new indices
#include <unordered_set> // For efficiently finding intersections

#include <zmq.hpp>      // For ZeroMQ C++ bindings (zmq::context_t, zmq::socket_t, zmq::message_t, zmq::error_t)
#include <zmq.h>        // For ZMQ_DONTWAIT (C-style ZMQ constants)

#include "network/data_receiver.h" // Your existing DataReceiver class
#include "data.h"                  // The Data struct definition
#include "essential/AVL.h"         // Include for AVL tree
#include "essential/LinkedList.h"  // Include for DoublyLinkedList
#include "essential/HashTable.h"   // Include for Chaining HashTable
#include "extra/CuckooHashTable.h" // Include for CuckooHashTable
#include "extra/SegmentTree.h"     // Include for SegmentTree
#include "essential/RBTree.h"      // Include for Red-Black Tree

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
        << ", Proto: " << static_cast<int>(data_item.proto)
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
    // ... (rest of the formatting fields)
    oss << "Label (Attack): " << (data_item.label ? "True" : "False") << "\n";
    oss << "Protocolo: " << static_cast<int>(data_item.proto) << " (Enum Value)\n";
    // ...
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
        case 6: return "Red-Black Tree";
        default: return "Unknown Data Structure";
    }
}


// Helper function to calculate and format statistics for a given feature and interval using DoublyLinkedList
std::string get_stats_for_feature_linkedlist(DoublyLinkedList& list, StatisticFeature feature, int interval_count) {
    // ... (implementation remains the same)
    return "";
}

// Helper function to calculate and format statistics for a given feature and interval using SegmentTree
std::string get_stats_for_feature_segmenttree(const SegmentTree& tree, StatisticFeature feature, int interval_count) {
    // ... (implementation remains the same)
    return "";
}

// Helper function to parse query strings like "key1=val1 key2=val2" into a map
std::map<std::string, std::string> parse_query_params(const std::string& query) {
    std::map<std::string, std::string> params;
    std::stringstream ss(query);
    std::string item;
    while (ss >> item) {
        size_t pos = item.find('=');
        if (pos != std::string::npos) {
            params[item.substr(0, pos)] = item.substr(pos + 1);
        }
    }
    return params;
}


int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Instantiate Data Structures ---
    AVL avl_tree;
    DoublyLinkedList doubly_linked_list; 
    HashTable hash_table;                
    CuckooHashTable cuckoo_hash_table;   
    SegmentTree segment_tree;            
    RBTree rb_tree;                      
    
    // --- NEW: Instantiate Indexing Data Structures ---
    // Using std::unordered_map for efficient inverted indices.
    std::unordered_map<bool, std::vector<const Data*>> label_index;
    std::unordered_map<int, std::vector<const Data*>> proto_index;
    
    const int AVL_DS_ID = 1;             
    const int LINKED_LIST_DS_ID = 2;     
    const int HASHTABLE_DS_ID = 3;       
    const int CUCKOO_HASH_DS_ID = 4;     
    const int SEGMENT_TREE_DS_ID = 5;    
    const int RED_BLACK_TREE_DS_ID = 6;  


    // --- Setup DataReceiver ---
    // ... (rest of setup is the same)
    DataReceiver data_collector("tcp://python_publisher:5556", "data_batch", "data_batch");
    data_collector.start();

    // --- Setup ZeroMQ REP Server ---
    // ... (rest of setup is the same)
    zmq::context_t rep_context(1);
    zmq::socket_t rep_socket(rep_context, ZMQ_REP);
    rep_socket.bind("tcp://*:5558");


    std::vector<std::unique_ptr<Data>> master_data_store;
    size_t last_processed_data_collector_count = 0; 

    while (keep_running.load()) {
        std::pair<const Data*, size_t> received_data_view = data_collector.getCollectedData();
        const Data* received_data_ptr = received_data_view.first;
        size_t current_received_count = received_data_view.second;

        if (current_received_count > last_processed_data_collector_count) {
            for (size_t i = last_processed_data_collector_count; i < current_received_count; ++i) {
                master_data_store.push_back(std::make_unique<Data>(received_data_ptr[i]));
                Data* data_to_insert = master_data_store.back().get();

                // Insert into main data structures
                avl_tree.insert(data_to_insert); 
                doubly_linked_list.append(data_to_insert);
                hash_table.insert(data_to_insert);
                cuckoo_hash_table.insert(data_to_insert);
                segment_tree.insert(data_to_insert);
                rb_tree.insert(data_to_insert);

                // --- NEW: Populate the indices ---
                label_index[data_to_insert->label].push_back(data_to_insert);
                proto_index[static_cast<int>(data_to_insert->proto)].push_back(data_to_insert);
            }
            last_processed_data_collector_count = current_received_count;
        }

        zmq::message_t request_msg;
        if (rep_socket.recv(&request_msg, ZMQ_DONTWAIT)) {
            std::string request_str(static_cast<char*>(request_msg.data()), request_msg.size());
            std::string reply_str;

            // --- Command Handling ---

            // ... (GET_DATA, QUERY_DATA_BY_ID, REMOVE_DATA_BY_ID, GET_STATS_SUMMARY logic is the same)

            if (request_str.rfind("QUERY_FILTERED_SORTED", 0) == 0) {
                size_t prefix_len = strlen("QUERY_FILTERED_SORTED ");
                std::map<std::string, std::string> params;
                if (request_str.length() > prefix_len) {
                    params = parse_query_params(request_str.substr(prefix_len));
                }

                // 1. OPTIMIZED Filtering
                std::vector<const Data*> candidate_list;
                bool is_first_filter = true;

                // Apply label filter first if present (highly selective)
                if (params.count("label")) {
                    bool required_label = (params["label"] == "true");
                    if (label_index.count(required_label)) {
                        candidate_list = label_index[required_label];
                    }
                    is_first_filter = false;
                }
                
                // Apply protocol filter
                if (params.count("proto")) {
                    try {
                        int required_proto = std::stoi(params["proto"]);
                        if (proto_index.count(required_proto)) {
                            if(is_first_filter) {
                                candidate_list = proto_index[required_proto];
                            } else {
                                // Intersect with existing candidates
                                std::unordered_set<const Data*> current_candidates(candidate_list.begin(), candidate_list.end());
                                std::vector<const Data*> proto_candidates = proto_index[required_proto];
                                std::vector<const Data*> intersection;
                                
                                for(const auto& data_ptr : proto_candidates) {
                                    if(current_candidates.count(data_ptr)) {
                                        intersection.push_back(data_ptr);
                                    }
                                }
                                candidate_list = intersection;
                            }
                        } else { // No data for this proto, result is empty
                            candidate_list.clear();
                        }
                    } catch (const std::exception& e) { /* ignore invalid proto */ }
                    is_first_filter = false;
                }

                // If no categorical filters were used, start with all data pointers
                if(is_first_filter) {
                    candidate_list.reserve(master_data_store.size());
                    for(const auto& ptr : master_data_store) {
                        candidate_list.push_back(ptr.get());
                    }
                }
                
                // (Future: Add more filters here, operating on the already reduced candidate_list)


                // 2. Sorting (now operates on the much smaller candidate_list)
                std::string sort_by = params.count("sort_by") ? params["sort_by"] : "id";
                std::string sort_order = params.count("sort_order") ? params["sort_order"] : "asc";

                std::sort(candidate_list.begin(), candidate_list.end(), 
                    [&](const Data* a, const Data* b) {
                        bool is_asc = (sort_order == "asc");
                        if (sort_by == "dur") return is_asc ? (a->dur < b->dur) : (a->dur > b->dur);
                        if (sort_by == "rate") return is_asc ? (a->rate < b->rate) : (a->rate > b->rate);
                        if (sort_by == "sbytes") return is_asc ? (a->sbytes < b->sbytes) : (a->sbytes > b->sbytes);
                        if (sort_by == "dbytes") return is_asc ? (a->dbytes < b->dbytes) : (a->dbytes > b->dbytes);
                        return is_asc ? (a->id < b->id) : (a->id > b->id);
                    });

                // 3. Formatting the reply
                std::ostringstream oss_reply;
                oss_reply << "Found " << candidate_list.size() << " matching records. Displaying top results:\n";
                oss_reply << "-----------------------------------------------------------------\n";
                
                int limit = params.count("limit") ? std::stoi(params["limit"]) : 20;
                for(int i = 0; i < std::min((int)candidate_list.size(), limit); ++i) {
                    oss_reply << (i + 1) << ". " << format_data_for_reply(*candidate_list[i]) << "\n";
                }
                reply_str = oss_reply.str();

            } else {
                // ... (handling for other commands)
            }

            zmq::message_t reply_msg(reply_str.data(), reply_str.size());
            rep_socket.send(reply_msg, 0);
        }
    }

    // ... (rest of main loop and shutdown)
    return 0;
}


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
#include "extra/SkipList.h"        // NEW: Include for SkipList

// Global atomic boolean to signal termination for all loops
std::atomic<bool> keep_running(true);

// Constants for data cleanup (NEW)
const size_t MASTER_STORE_CAPACITY_THRESHOLD = 30000; 
const size_t CLEANUP_BATCH_SIZE = DATA_RECEIVER_CAPACITY * 0.1; 

// NEW: Constante para simular a redução da frequência do processador (R7)
const std::chrono::microseconds PROCESSING_DELAY_PER_ITEM(50); 

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
    oss << "Destination Load (bytes/s): " << data_item.dload << "\n";
    oss << "Source Packets: " << data_item.spkts << "\n";
    oss << "Dest Packets: " << data_item.dpkts << "\n";
    oss << "Source Bytes: " << data_item.sbytes << "\n";
    oss << "Dest Bytes: " << data_item.dbytes << "\n";
    oss << "Label (Attack): " << (data_item.label ? "True" : "False") << "\n";
    oss << "Protocol: " << static_cast<int>(data_item.proto) << "\n";
    oss << "State: " << static_cast<int>(data_item.state) << "\n";
    oss << "Service: " << static_cast<int>(data_item.service) << "\n";
    oss << "Attack Category: " << static_cast<int>(data_item.attack_category) << "\n";
    oss << "--------------------------------------\n";
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
        case 7: return "SkipList"; 
        default: return "Unknown";
    }
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

// Function to clean up old data from master_data_store and all data structures
void cleanup_old_data(
    std::vector<std::unique_ptr<Data>>& master_data_store,
    AVL& avl_tree,
    DoublyLinkedList& doubly_linked_list,
    HashTable& hash_table,
    CuckooHashTable& cuckoo_hash_table,
    SegmentTree& segment_tree,
    RBTree& rb_tree,
    SkipList& skip_list,
    std::unordered_map<bool, std::vector<const Data*>>& label_index,
    std::unordered_map<int, std::vector<const Data*>>& proto_index,
    size_t num_items_to_remove)
{
    std::cout << "[DEBUG] cleanup_old_data function called." << std::endl;

    if (master_data_store.empty() || num_items_to_remove == 0) {
        std::cout << "[INFO] Nenhuma limpeza necessária: master_data_store vazio ou num_items_to_remove é zero." << std::endl;
        return;
    }

    size_t actual_items_to_remove = std::min(num_items_to_remove, master_data_store.size());
    std::cout << "[INFO] Iniciando limpeza: removendo " << actual_items_to_remove << " itens de dados mais antigos." << std::endl;

    std::vector<uint32_t> ids_to_remove;
    ids_to_remove.reserve(actual_items_to_remove);

    for (size_t i = 0; i < actual_items_to_remove; ++i) {
        if (master_data_store[i]) { 
            ids_to_remove.push_back(master_data_store[i]->id);
        }
    }

    for (uint32_t id : ids_to_remove) {
        avl_tree.removeById(id);
        doubly_linked_list.removeById(id);
        hash_table.remove(id);
        cuckoo_hash_table.remove(id);
        segment_tree.remove(id);
        rb_tree.remove(id);
        skip_list.remove(id);
    }

    master_data_store.erase(master_data_store.begin(), master_data_store.begin() + actual_items_to_remove);
    std::cout << "[INFO] Removido " << actual_items_to_remove << " itens do master_data_store. Novo tamanho: " << master_data_store.size() << std::endl;

    label_index.clear();
    proto_index.clear();
    for (const auto& data_ptr_unique : master_data_store) {
        Data* data_item = data_ptr_unique.get(); 
        label_index[data_item->label].push_back(data_item);
        proto_index[static_cast<int>(data_item->proto)].push_back(data_item);
    }
    std::cout << "[INFO] Índices 'label_index' e 'proto_index' reconstruídos." << std::endl;
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
    SkipList skip_list; 
    
    // --- NEW: Instantiate Indexing Data Structures ---
    std::unordered_map<bool, std::vector<const Data*>> label_index;
    std::unordered_map<int, std::vector<const Data*>> proto_index; 
    
    // --- Setup DataReceiver ---
    DataReceiver data_collector("tcp://python_publisher:5556", "data_batch", "data_batch");
    data_collector.start();

    // --- Setup ZeroMQ REP Server ---
    zmq::context_t rep_context(1);
    zmq::socket_t rep_socket(rep_context, ZMQ_REP);
    rep_socket.bind("tcp://*:5558");

    std::vector<std::unique_ptr<Data>> master_data_store;

    while (keep_running.load()) {
        // NEW: Get a view of currently collected data from DataReceiver
        std::pair<const Data*, size_t> received_data_view = data_collector.getCollectedDataView();
        const Data* received_data_ptr = received_data_view.first;
        size_t num_items_in_view = received_data_view.second; 

        // Process data in chunks as provided by DataReceiver
        size_t processed_count_in_this_cycle = 0;
        if (num_items_in_view > 0) {
            for (size_t i = 0; i < num_items_in_view; ++i) {
                master_data_store.push_back(std::make_unique<Data>(received_data_ptr[i]));
                Data* data_to_insert = master_data_store.back().get();

                avl_tree.insert(data_to_insert);
                doubly_linked_list.append(data_to_insert);
                hash_table.insert(data_to_insert);
                cuckoo_hash_table.insert(data_to_insert);
                segment_tree.insert(data_to_insert);
                rb_tree.insert(data_to_insert);
                skip_list.insert(data_to_insert); 

                label_index[data_to_insert->label].push_back(data_to_insert);
                proto_index[static_cast<int>(data_to_insert->proto)].push_back(data_to_insert);

                std::this_thread::sleep_for(PROCESSING_DELAY_PER_ITEM);
            }
            processed_count_in_this_cycle = num_items_in_view;
            // Mark the processed items as consumed in DataReceiver
            data_collector.markDataAsConsumed(processed_count_in_this_cycle);
        }

        // Check for cleanup after processing any new data
        if (master_data_store.size() >= MASTER_STORE_CAPACITY_THRESHOLD) {
            cleanup_old_data(
                master_data_store,
                avl_tree,
                doubly_linked_list,
                hash_table,
                cuckoo_hash_table,
                segment_tree,
                rb_tree,
                skip_list,
                label_index,
                proto_index,
                CLEANUP_BATCH_SIZE
            );
        }

        // Add a small sleep to prevent busy-waiting if no data/requests are present
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        zmq::message_t request_msg;
        if (rep_socket.recv(&request_msg, ZMQ_DONTWAIT)) {
            std::string request_str(static_cast<char*>(request_msg.data()), request_msg.size());
            std::string reply_str;
            std::cout << "[DEBUG] Received request: '" << request_str << "'" << std::endl;

            // --- Command Handling ---
            std::stringstream ss(request_str);
            std::string command;
            ss >> command;

            if (command == "GET_DATA") {
                std::ostringstream oss_reply;
                int count = 0;
                if (master_data_store.empty()) {
                    oss_reply << "No data collected yet.";
                } else {
                    oss_reply << "Last 3 received data records:\n";
                    for (auto it = master_data_store.rbegin(); it != master_data_store.rend() && count < 3; ++it) {
                        oss_reply << format_data_for_reply(*((*it).get())) << "\n";
                        count++;
                    }
                }
                reply_str = oss_reply.str();
            } else if (command == "QUERY_DATA_BY_ID") {
                uint32_t id;
                int ds_id;
                if (ss >> id >> ds_id) {
                    const Data* found_data = nullptr;
                    switch (ds_id) {
                        case 1: { auto node = avl_tree.queryById(id); if(node) found_data = node->data; break; }
                        case 2: found_data = doubly_linked_list.findById(id); break;
                        case 3: found_data = hash_table.find(id); break; // Corrected from 'found_table'
                        case 4: found_data = cuckoo_hash_table.search(id); break;
                        case 5: found_data = segment_tree.find(id); break;
                        case 6: found_data = rb_tree.find(id); break;
                        case 7: found_data = skip_list.find(id); break; 
                    }
                    if (found_data) {
                        reply_str = "Found data in " + get_ds_name_by_id(ds_id) + ":\n" + format_data_as_table(*found_data);
                    } else {
                        reply_str = "No data with ID " + std::to_string(id) + " found in " + get_ds_name_by_id(ds_id) + ".";
                    }
                } else {
                    reply_str = "Error: Malformed QUERY_DATA_BY_ID command.";
                }
            } else if (command == "REMOVE_DATA_BY_ID") {
                    uint32_t id;
                    int ds_id;
                    if (ss >> id >> ds_id) {
                        bool removed = false;
                        switch(ds_id) {
                            case 1: { avl_tree.removeById(id); removed = true; break; } 
                            case 2: removed = doubly_linked_list.removeById(id); break;
                            case 3: removed = hash_table.remove(id); break;
                            case 4: removed = cuckoo_hash_table.remove(id); break;
                            case 5: removed = segment_tree.remove(id); break;
                            case 6: removed = rb_tree.remove(id); break;
                            case 7: removed = skip_list.remove(id); break; 
                        }
                        if (removed) {
                            reply_str = "Successfully removed reference to ID " + std::to_string(id) + " from " + get_ds_name_by_id(ds_id) + ".";
                        } else {
                            reply_str = "Could not remove data with ID " + std::to_string(id) + " from " + get_ds_name_by_id(ds_id) + " (not found).";
                        }
                    } else {
                        reply_str = "Error: Malformed REMOVE_DATA_BY_ID command.";
                    }
            } else if (command == "PERFORM_STATS") {
                int feature_enum_val, interval, ds_id;
                if (ss >> feature_enum_val >> interval >> ds_id) {
                    StatisticFeature feature = static_cast<StatisticFeature>(feature_enum_val);
                    std::ostringstream oss_stats;
                    oss_stats << std::fixed << std::setprecision(4);
                    oss_stats << "Statistics for " << get_ds_name_by_id(ds_id) << " over last " << interval << " items:\n";
                    
                    if (ds_id == 2) { // DoublyLinkedList
                        oss_stats << "  Average: " << doubly_linked_list.getAverage(feature, interval) << "\n";
                        oss_stats << "  Std Dev: " << doubly_linked_list.getStdDev(feature, interval) << "\n";
                        oss_stats << "  Median:  " << doubly_linked_list.getMedian(feature, interval) << "\n";
                        oss_stats << "  Min:     " << doubly_linked_list.getMin(feature, interval) << "\n";
                        oss_stats << "  Max:     " << doubly_linked_list.getMax(feature, interval) << "\n";
                    } else if (ds_id == 5) { // SegmentTree
                        oss_stats << "  Average: " << segment_tree.getAverage(feature, interval) << "\n";
                        oss_stats << "  Std Dev: " << segment_tree.getStdDev(feature, interval) << "\n";
                        oss_stats << "  Median:  " << segment_tree.getMedian(feature, interval) << "\n";
                        oss_stats << "  Min:     " << segment_tree.getMin(feature, interval) << "\n";
                        oss_stats << "  Max:     " << segment_tree.getMax(feature, interval) << "\n";
                    } else {
                        oss_stats << "  Statistics are not implemented for this data structure.";
                    }
                    reply_str = oss_stats.str();
                } else {
                    reply_str = "Error: Malformed PERFORM_STATS command.";
                }
            }
            else if (command == "QUERY_FILTERED_SORTED") {
                size_t prefix_len = command.length() + 1;
                std::map<std::string, std::string> params;
                if (request_str.length() > prefix_len) {
                    params = parse_query_params(request_str.substr(prefix_len));
                }

                // Filtering
                std::vector<const Data*> candidate_list;
                bool is_first_filter = true;

                if (params.count("label")) {
                    bool required_label = (params["label"] == "true");
                    if (label_index.count(required_label)) {
                        candidate_list = label_index[required_label];
                    }
                    is_first_filter = false;
                }
                
                if (params.count("proto")) {
                    try {
                        int required_proto = std::stoi(params["proto"]);
                        if (proto_index.count(required_proto)) {
                            if(is_first_filter) {
                                candidate_list = proto_index[required_proto];
                            } else {
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
                        } else {
                            candidate_list.clear();
                        }
                    } catch (const std::exception& e) { /* ignore invalid proto */ }
                    is_first_filter = false;
                }

                if(is_first_filter) {
                    candidate_list.reserve(master_data_store.size());
                    for(const auto& ptr : master_data_store) {
                        candidate_list.push_back(ptr.get());
                    }
                }
                
                // Sorting
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

                // Formatting reply
                std::ostringstream oss_reply;
                oss_reply << "Found " << candidate_list.size() << " matching records. Displaying top results:\n";
                oss_reply << "-----------------------------------------------------------------\n";
                
                int limit = params.count("limit") ? std::stoi(params["limit"]) : 20;
                for(int i = 0; i < std::min((int)candidate_list.size(), limit); ++i) {
                    oss_reply << (i + 1) << ". " << format_data_for_reply(*candidate_list[i]) << "\n";
                }
                reply_str = oss_reply.str();

            } else {
                reply_str = "Error: Unknown command '" + command + "' or invalid format.";
            }

            std::cout << "[DEBUG] Sending reply: '" << reply_str.substr(0, 200) << (reply_str.length() > 200 ? "..." : "") << "'" << std::endl;
            zmq::message_t reply_msg(reply_str.data(), reply_str.size());
            rep_socket.send(reply_msg, 0);
        }

        // Add a small sleep to prevent busy-waiting if no data/requests are present
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "Main loop terminated. Shutting down server." << std::endl;
    data_collector.stop();
    data_collector.join();
    rep_socket.close();
    rep_context.close();
    std::cout << "Server shutdown complete." << std::endl;

    return 0;
}


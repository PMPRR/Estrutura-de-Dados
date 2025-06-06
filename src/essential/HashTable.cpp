#include "essential/HashTable.h" // Correct header for THIS HashTable.cpp
#include <iostream> // For potential debug/error output
#include <functional> // For std::hash

HashTable::HashTable(size_t capacidade)
    : table(capacidade), itemCount(0) {}

HashTable::~HashTable() {
    clear();
}

// Simple hash function for uint32_t using std::hash
size_t HashTable::hash(uint32_t key) const {
    return std::hash<uint32_t>{}(key) % table.size();
}

void HashTable::insert(const Data* data) {
    if (!data) {
        std::cerr << "Error: Attempted to insert a nullptr Data into HashTable." << std::endl;
        return;
    }

    size_t idx = hash(data->id);
    // Check if key (data->id) already exists and update value (data pointer)
    for (auto &node : table[idx]) {
        if (node.data && node.data->id == data->id) { // Check for null data pointer too
            node.data = data; // Update the pointer to the new Data object
            return;
        }
    }
    // If key does not exist, add new node
    table[idx].emplace_back(data);
    ++itemCount;
}

bool HashTable::remove(uint32_t id) {
    size_t idx = hash(id);
    for (auto it = table[idx].begin(); it != table[idx].end(); ++it) {
        if (it->data && it->data->id == id) { // Check for null data pointer too
            table[idx].erase(it);
            --itemCount;
            return true;
        }
    }
    return false;
}

const Data* HashTable::find(uint32_t id) const {
    size_t idx = hash(id);
    for (const auto &node : table[idx]) {
        if (node.data && node.data->id == id) { // Check for null data pointer too
            return node.data;
        }
    }
    return nullptr;
}

void HashTable::clear() {
    for (auto &bucket : table) {
        bucket.clear(); // Clears all Node objects in the bucket
    }
    itemCount = 0;
}

size_t HashTable::size() const {
    return itemCount;
}


#include "extra/CuckooHashTable.h" // Correct header for CuckooHashTable implementation
#include <cmath>
#include <vector>
#include <utility>   // For std::swap
#include <stdexcept> // For std::runtime_error if needed, though replaced with cerr
#include <iostream>  // For std::cerr, std::cout

CuckooHashTable::CuckooHashTable(size_t initial_capacity)
    : capacity(initial_capacity), size(0) {
    // Ensure capacity is not zero or too small
    if (capacity == 0) capacity = 1;
    // Cuckoo hashing often benefits from prime capacities, or capacities not powers of 2.
    // If you need specific prime logic, you can add it here.
    table1.assign(capacity, Entry()); // Initialize with empty entries (nullptr)
    table2.assign(capacity, Entry()); // Initialize with empty entries (nullptr)
    max_loop = std::log2(capacity) * 2 + 1; // A common heuristic for max kicks
    if (max_loop < 10) max_loop = 10; // Ensure a reasonable minimum
}

CuckooUsageInfo CuckooHashTable::getUsageInfo() const {
    CuckooUsageInfo info = {0};
    info.capacity_per_table = capacity;
    info.total_capacity = capacity * 2;
    info.current_size = size;
    info.total_memory_bytes = info.total_capacity * sizeof(Entry);

    size_t table1_count = 0;
    for(const auto& entry : table1) {
        if (entry.data != nullptr) table1_count++;
    }

    size_t table2_count = 0;
    for(const auto& entry : table2) {
        if (entry.data != nullptr) table2_count++;
    }

    if (capacity > 0) {
        info.table1_usage_percent = (static_cast<float>(table1_count) / capacity) * 100.0f;
        info.table2_usage_percent = (static_cast<float>(table2_count) / capacity) * 100.0f;
    }
    if (info.total_capacity > 0) {
        info.overall_load_factor_percent = (static_cast<float>(size) / info.total_capacity) * 100.0f;
    }
    return info;
}

// Simple hash function for uint32_t
size_t CuckooHashTable::hash1(uint32_t key) const {
    return key % capacity;
}

// A different hash function is critical for Cuckoo Hashing.
// Using a simple division and modulo, or a different prime, or bitwise operations.
// For now, a simple arithmetic one to ensure it's distinct from hash1.
size_t CuckooHashTable::hash2(uint32_t key) const {
    // A simple multiplicative hash part can help improve distribution,
    // even before the modulo. This constant is a common choice.
    uint32_t c = 2654435769; // A large prime-like number
    return (key * c) % capacity;
}

void CuckooHashTable::rehash() {
    size_t old_capacity = capacity;
    capacity = capacity * 2 + 1; // Double and add 1 to aim for an odd/prime-like new capacity
    max_loop = std::log2(capacity) * 2 + 1; // Update max kicks for new capacity
    if (max_loop < 10) max_loop = 10;

    std::vector<Entry> old_table1; // Use temporary vectors to hold old entries
    old_table1.reserve(old_capacity);
    for(const auto& entry : table1) {
        if(entry.data != nullptr) old_table1.push_back(entry);
    }

    std::vector<Entry> old_table2;
    old_table2.reserve(old_capacity);
    for(const auto& entry : table2) {
        if(entry.data != nullptr) old_table2.push_back(entry);
    }
    
    table1.assign(capacity, Entry()); // Resize and clear with new capacity
    table2.assign(capacity, Entry());
    size = 0; // Reset size, will be incremented by insert

    // Re-insert all elements from old tables into the new, larger tables
    for (const auto& entry : old_table1) {
        insert(entry.data); // This insert will handle potential kicks/cycles in new tables
    }
    for (const auto& entry : old_table2) {
        insert(entry.data); // This insert will handle potential kicks/cycles in new tables
    }
    //std::cout << "[CuckooHashTable] Rehashing complete. Old capacity: " << old_capacity << ", New capacity: " << capacity << ", max_loop: " << max_loop << std::endl;
}

bool CuckooHashTable::insert(const Data* data) {
    if (data == nullptr) {
        std::cerr << "Error: Attempted to insert a nullptr Data into CuckooHashTable." << std::endl;
        return false;
    }

    // Check if key (data->id) already exists and update its pointer
    // This avoids adding duplicates and ensures the latest pointer is used.
    size_t pos1_check = hash1(data->id);
    if (table1[pos1_check].data != nullptr && table1[pos1_check].data->id == data->id) {
        table1[pos1_check].data = data; // Update pointer
        return true;
    }
    size_t pos2_check = hash2(data->id);
    if (table2[pos2_check].data != nullptr && table2[pos2_check].data->id == data->id) {
        table2[pos2_check].data = data; // Update pointer
        return true;
    }

    const Data* current_data_to_place = data; // The item we are trying to insert (or kick)
    size_t loop_count = 0;

    for (; loop_count < max_loop; ++loop_count) {
        // Try to place in table 1
        size_t pos1 = hash1(current_data_to_place->id);
        if (table1[pos1].data == nullptr) { // Slot is empty
            table1[pos1].data = current_data_to_place;
            ++size;
            return true;
        }
        std::swap(current_data_to_place, table1[pos1].data); // Kick out existing, try to insert current_data_to_place

        // Try to place in table 2
        size_t pos2 = hash2(current_data_to_place->id);
        if (table2[pos2].data == nullptr) { // Slot is empty
            table2[pos2].data = current_data_to_place;
            ++size;
            return true;
        }
        std::swap(current_data_to_place, table2[pos2].data); // Kick out existing, try to insert current_data_to_place
    }

    // If loop_count reaches max_loop, a cycle was detected or max kicks were exceeded.
    // Rehash and try inserting the problematic item again (this recursive call will succeed).
    std::cerr << "[CuckooHashTable] Max kicks reached for ID " << current_data_to_place->id << ". Initiating rehash." << std::endl;
    rehash();
    return insert(current_data_to_place); // Re-attempt insertion of the item that caused the cycle
}

bool CuckooHashTable::remove(uint32_t id) {
    size_t pos1 = hash1(id);
    if (table1[pos1].data != nullptr && table1[pos1].data->id == id) {
        table1[pos1].data = nullptr; // Set to nullptr to indicate empty slot
        --size;
        return true;
    }

    size_t pos2 = hash2(id);
    if (table2[pos2].data != nullptr && table2[pos2].data->id == id) {
        table2[pos2].data = nullptr; // Set to nullptr to indicate empty slot
        --size;
        return true;
    }

    return false; // Item not found
}

const Data* CuckooHashTable::search(uint32_t id) {
    size_t pos1 = hash1(id);
    if (table1[pos1].data != nullptr && table1[pos1].data->id == id) {
        return table1[pos1].data;
    }

    size_t pos2 = hash2(id);
    if (table2[pos2].data != nullptr && table2[pos2].data->id == id) {
        return table2[pos2].data;
    }

    return nullptr; // Item not found
}

bool CuckooHashTable::contains(uint32_t id) const {
    size_t pos1 = hash1(id);
    if (table1[pos1].data != nullptr && table1[pos1].data->id == id) return true;

    size_t pos2 = hash2(id);
    if (table2[pos2].data != nullptr && table2[pos2].data->id == id) return true;

    return false;
}

size_t CuckooHashTable::getSize() const {
    return size;
}

size_t CuckooHashTable::getCapacity() const {
    return capacity;
}


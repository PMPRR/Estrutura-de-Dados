#ifndef CUCKOOHASHTABLE_H_
#define CUCKOOHASHTABLE_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "data.h"

// NEW: Struct to hold usage information for the Cuckoo Hash Table
struct CuckooUsageInfo {
    size_t capacity_per_table;
    size_t total_capacity;
    size_t current_size;
    float table1_usage_percent;
    float table2_usage_percent;
    float overall_load_factor_percent;
    size_t total_memory_bytes;
};


class CuckooHashTable {
public:
    explicit CuckooHashTable(size_t initial_capacity = 101);
    
    bool insert(const Data* data);
    bool remove(uint32_t id);
    const Data* search(uint32_t id);
    bool contains(uint32_t id) const;
    size_t getSize() const;
    size_t getCapacity() const;

    // NEW: Method to get usage and load factor stats
   CuckooUsageInfo getUsageInfo() const;

private:
    struct Entry {
        const Data* data;
        Entry(const Data* d) : data(d) {}
        Entry() : data(nullptr) {}
    };

    std::vector<Entry> table1;
    std::vector<Entry> table2;
    size_t capacity;
    size_t size;       
    size_t max_loop;   

    size_t hash1(uint32_t key) const;
    size_t hash2(uint32_t key) const;
    void rehash();
};

#endif // CUCKOOHASHTABLE_H_


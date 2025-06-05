#ifndef CUCKOOHASHTABLE_H_
#define CUCKOOHASHTABLE_H_

#include <cstddef>
#include <cstdint>
#include "data.h"
#include <vector>

class CuckooHashTable {
public:
    explicit CuckooHashTable(size_t initial_capacity = 101);
    bool insert(const Data& data);
    bool remove(uint32_t id);
    Data* search(uint32_t id);
    bool contains(uint32_t id) const;
    size_t getSize() const;
    size_t getCapacity() const;

private:
    struct Entry {
        Data data;
        bool occupied;

        Entry();
        Entry(const Data& d);
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

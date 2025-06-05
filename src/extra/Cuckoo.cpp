#include "CuckooHashTable.h"
#include <cmath>
#include <vector>
#include <utility>
#include <stdexcept>

CuckooHashTable::Entry::Entry() : occupied(false) {}
CuckooHashTable::Entry::Entry(const Data& d) : data(d), occupied(true) {}

CuckooHashTable::CuckooHashTable(size_t initial_capacity)
    : capacity(initial_capacity), size(0) {
    table1.resize(capacity);
    table2.resize(capacity);
    max_loop = std::log2(capacity) + 1;
}

size_t CuckooHashTable::hash1(uint32_t key) const {
    return key % capacity;
}

size_t CuckooHashTable::hash2(uint32_t key) const {
    return (key / capacity) % capacity;
}

void CuckooHashTable::rehash() {
    size_t old_capacity = capacity;
    capacity *= 2;
    max_loop = std::log2(capacity) + 1;

    std::vector<Entry> old_table1 = table1;
    std::vector<Entry> old_table2 = table2;

    table1.assign(capacity, Entry());
    table2.assign(capacity, Entry());
    size = 0;

    for (const auto& entry : old_table1) {
        if (entry.occupied) insert(entry.data);
    }
    for (const auto& entry : old_table2) {
        if (entry.occupied) insert(entry.data);
    }
}

bool CuckooHashTable::insert(const Data& data) {
    if (contains(data.id)) return false;

    Data cur = data;
    size_t loop_count = 0;

    for (; loop_count < max_loop; ++loop_count) {
        size_t pos1 = hash1(cur.id);
        if (!table1[pos1].occupied) {
            table1[pos1] = Entry(cur);
            ++size;
            return true;
        }
        std::swap(cur, table1[pos1].data);

        size_t pos2 = hash2(cur.id);
        if (!table2[pos2].occupied) {
            table2[pos2] = Entry(cur);
            ++size;
            return true;
        }
        std::swap(cur, table2[pos2].data);
    }

    rehash();
    return insert(cur);
}

bool CuckooHashTable::remove(uint32_t id) {
    size_t pos1 = hash1(id);
    if (table1[pos1].occupied && table1[pos1].data.id == id) {
        table1[pos1].occupied = false;
        --size;
        return true;
    }

    size_t pos2 = hash2(id);
    if (table2[pos2].occupied && table2[pos2].data.id == id) {
        table2[pos2].occupied = false;
        --size;
        return true;
    }

    return false;
}

Data* CuckooHashTable::search(uint32_t id) {
    size_t pos1 = hash1(id);
    if (table1[pos1].occupied && table1[pos1].data.id == id) {
        return &table1[pos1].data;
    }

    size_t pos2 = hash2(id);
    if (table2[pos2].occupied && table2[pos2].data.id == id) {
        return &table2[pos2].data;
    }

    return nullptr;
}

bool CuckooHashTable::contains(uint32_t id) const {
    size_t pos1 = hash1(id);
    if (table1[pos1].occupied && table1[pos1].data.id == id) return true;

    size_t pos2 = hash2(id);
    if (table2[pos2].occupied && table2[pos2].data.id == id) return true;

    return false;
}

size_t CuckooHashTable::getSize() const {
    return size;
}

size_t CuckooHashTable::getCapacity() const {
    return capacity;
}

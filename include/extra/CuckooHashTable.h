#ifndef CUCKOOHASHTABLE_H_
#define CUCKOOHASHTABLE_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "data.h" // Include Data struct definition

class CuckooHashTable {
public:
    explicit CuckooHashTable(size_t initial_capacity = 101);
    // Insert a Data* into the hash table using its ID as the key.
    // Returns true if insertion (or update) was successful.
    bool insert(const Data* data);

    // Remove a Data* by its ID.
    // Returns true if removal was successful.
    bool remove(uint32_t id);

    // Search for a Data* by its ID.
    // Returns a const Data* if found, nullptr otherwise.
    const Data* search(uint32_t id);

    // Check if an ID exists in the hash table.
    bool contains(uint32_t id) const;

    // Get the number of elements currently stored.
    size_t getSize() const;

    // Get the current capacity of the hash tables.
    size_t getCapacity() const;

private:
    // Entry struct now stores a pointer to Data.
    // nullptr will indicate an unoccupied slot.
    struct Entry {
        const Data* data;

        // Constructor for an occupied entry
        Entry(const Data* d) : data(d) {}
        // Default constructor for an empty entry (nullptr data)
        Entry() : data(nullptr) {}
    };

    std::vector<Entry> table1;
    std::vector<Entry> table2;
    size_t capacity;
    size_t size;       // Current number of elements
    size_t max_loop;   // Max kicks before rehash

    // Hash functions
    size_t hash1(uint32_t key) const;
    size_t hash2(uint32_t key) const;

    // Rehash the tables (doubles capacity)
    void rehash();
};

#endif // CUCKOOHASHTABLE_H_


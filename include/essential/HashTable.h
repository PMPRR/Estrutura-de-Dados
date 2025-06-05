#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <vector>
#include <optional>
#include <functional>
#include <iostream>

namespace essential {

template<typename K, typename V>
class HashTable {
private:
    struct Entry {
        K key;
        V value;
        bool occupied = false;
        bool deleted = false;
    };

    std::vector<Entry> table;
    size_t current_size;
    size_t capacity;
    const float load_factor_threshold = 0.7;

    size_t hash(const K& key) const;
    void rehash();

public:
    HashTable(size_t init_capacity = 8);
    void insert(const K& key, const V& value);
    std::optional<V> get(const K& key) const;
    bool erase(const K& key);
    void display() const;
};

} // namespace essential

// Importa a implementação
#include "../../src/HashTable.tpp"

#endif // HASHTABLE_H

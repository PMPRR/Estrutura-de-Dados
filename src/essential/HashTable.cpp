#include "essential/HashTable.h"

namespace essential {

HashTable::HashTable(size_t capacidade)
    : table(capacidade), itemCount(0) {}

HashTable::~HashTable() {
    clear();
}

size_t HashTable::hash(const std::string &key) const {
    // Hash DJB2 simples
    size_t hash = 5381;
    for (char c : key) {
        hash = ((hash << 5) + hash) + static_cast<size_t>(c); // hash * 33 + c
    }
    return hash % table.size();
}

void HashTable::insert(const std::string &key, const std::string &value) {
    size_t idx = hash(key);
    for (auto &node : table[idx]) {
        if (node.key == key) {
            node.value = value; // Atualiza valor se a chave já existir
            return;
        }
    }
    table[idx].emplace_back(key, value);
    ++itemCount;
}

bool HashTable::remove(const std::string &key) {
    size_t idx = hash(key);
    for (auto it = table[idx].begin(); it != table[idx].end(); ++it) {
        if (it->key == key) {
            table[idx].erase(it);
            --itemCount;
            return true;
        }
    }
    return false;
}

const std::string* HashTable::find(const std::string &key) const {
    size_t idx = hash(key);
    for (const auto &node : table[idx]) {
        if (node.key == key)
            return &node.value;
    }
    return nullptr;
}

void HashTable::clear() {
    for (auto &bucket : table) {
        bucket.clear();
    }
    itemCount = 0;
}

size_t HashTable::size() const {
    return itemCount;
}

} // namespace essential

#include <map>
#include <iostream>
#include <vector>
#include <optional>
#include <cmath>
#include "data.h"

#define TABLE_SIZE 101

// Conversão de Attack_cat para string
inline std::string attackCatToString(Attack_cat cat) {
    switch (cat) {
        case Attack_cat::NORMAL: return "Normal";
        case Attack_cat::GENERIC: return "Generic";
        case Attack_cat::EXPLOITS: return "Exploits";
        case Attack_cat::FUZZERS: return "Fuzzers";
        case Attack_cat::DOS: return "DoS";
        case Attack_cat::ANALYSIS: return "Analysis";
        case Attack_cat::RECONNAISSANCE: return "Reconnaissance";
        case Attack_cat::BACKDOOR: return "Backdoor";
        case Attack_cat::SHELLCODE: return "Shellcode";
        case Attack_cat::WORMS: return "Worms";
        default: return "Unknown";
    }
}


// Sobrecarga do operador <<
inline std::ostream& operator<<(std::ostream& os, Attack_cat cat) {
    return os << attackCatToString(cat);
}

class CuckooHashTable {
private:
    std::vector<std::optional<Data>> table1;
    std::vector<std::optional<Data>> table2;
    int maxKickCount;

    int hash1(uint32_t key) const {
        return key % TABLE_SIZE;
    }

    int hash2(uint32_t key) const {
        return (key / TABLE_SIZE) % TABLE_SIZE;
    }

    void rehash() {
        std::vector<std::optional<Data>> oldTable1 = table1;
        std::vector<std::optional<Data>> oldTable2 = table2;

        table1.assign(TABLE_SIZE, std::nullopt);
        table2.assign(TABLE_SIZE, std::nullopt);

        for (const auto& item : oldTable1) {
            if (item.has_value()) {
                insert(item->id, item.value());
            }
        }
        for (const auto& item : oldTable2) {
            if (item.has_value()) {
                insert(item->id, item.value());
            }
        }
    }

public:
    CuckooHashTable() : table1(TABLE_SIZE), table2(TABLE_SIZE), maxKickCount(32) {}

    void insert(uint32_t id, const Data& value) {
        Data toInsert = value;
        int kickCount = 0;

        while (kickCount < maxKickCount) {
            int h1 = hash1(toInsert.id);
            if (!table1[h1].has_value()) {
                table1[h1] = toInsert;
                return;
            }

            std::swap(toInsert, table1[h1].value());

            int h2 = hash2(toInsert.id);
            if (!table2[h2].has_value()) {
                table2[h2] = toInsert;
                return;
            }

            std::swap(toInsert, table2[h2].value());
            kickCount++;
        }

        std::cerr << "Rehashing needed for ID " << id << std::endl;
        rehash();
        insert(id, value); // reinserir após rehash
    }

    std::optional<Data> search(uint32_t id) const {
        int h1 = hash1(id);
        if (table1[h1].has_value() && table1[h1]->id == id) {
            return table1[h1];
        }

        int h2 = hash2(id);
        if (table2[h2].has_value() && table2[h2]->id == id) {
            return table2[h2];
        }

        return std::nullopt;
    }

    bool remove(uint32_t id) {
        int h1 = hash1(id);
        if (table1[h1].has_value() && table1[h1]->id == id) {
            table1[h1] = std::nullopt;
            return true;
        }

        int h2 = hash2(id);
        if (table2[h2].has_value() && table2[h2]->id == id) {
            table2[h2] = std::nullopt;
            return true;
        }

        return false;
    }

    void printStatsByAttackCategory() const {
        std::map<Attack_cat, std::vector<Data>> groups;

        auto addToGroup = [&](const std::optional<Data>& entry) {
            if (entry.has_value()) {
                groups[entry->attack_category].push_back(entry.value());
            }
        };

        for (const auto& entry : table1) addToGroup(entry);
        for (const auto& entry : table2) addToGroup(entry);

        for (const auto& [category, entries] : groups) {
            int count = entries.size();
            double sum = 0, sum_sq = 0;

            for (const auto& d : entries) {
                sum += d.sload;
                sum_sq += d.sload * d.sload;
            }

            double mean = sum / count;
            double variance = (sum_sq / count) - (mean * mean);
            double std_dev = std::sqrt(variance);

            std::cout << "Categoria: " << category << "\n";
            std::cout << " - Quantidade: " << count << "\n";
            std::cout << " - Média sload: " << mean << "\n";
            std::cout << " - Desvio padrão sload: " << std_dev << "\n\n";
        }
    }
};

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <memory>
#include <algorithm>
#include <set>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <cmath> // Required for std::log2

// Include all the data structures to be benchmarked
#include "essential/AVL.h"
#include "essential/RBTree.h"
#include "extra/SkipList.h"
#include "essential/HashTable.h"
#include "extra/CuckooHashTable.h" 
#include "essential/LinkedList.h"
#include "data.h"

// --- Global Test Data Pool ---
std::vector<std::unique_ptr<Data>> test_data_pool;
std::vector<uint32_t> existing_keys;
std::unordered_map<uint32_t, const Data*> data_map;
std::mt19937 rng;

void GenerateGlobalData(size_t n) {
    if (test_data_pool.size() >= n) return;

    test_data_pool.clear();
    existing_keys.clear();
    data_map.clear();
    
    std::uniform_int_distribution<uint32_t> dist(1, 20000000);
    rng.seed(std::random_device{}());
    std::set<uint32_t> unique_ids;

    // To ensure we have enough unique data for all tests
    size_t required_size = n + 100;
    while (unique_ids.size() < required_size) {
        unique_ids.insert(dist(rng));
    }

    test_data_pool.reserve(required_size);
    
    for (uint32_t id : unique_ids) {
        test_data_pool.push_back(std::make_unique<Data>(id, 1.0f, 10.0f, 100.0f, 0.0f, 0.1f, 0.1f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1, 1, 10, 10, 64, 64, 0, 0, 1, 1, 1, 1, 1, 1, 1, 100, 1, 1, 1, 1, 1, 0, 0, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
        const Data* ptr = test_data_pool.back().get();
        existing_keys.push_back(id);
        data_map[id] = ptr;
    }
}

// --- Base Fixture for setup ---
class BaseFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        int n = state.range(0);
        GenerateGlobalData(n);
        std::shuffle(existing_keys.begin(), existing_keys.begin() + n, rng);
    }
};

// --- Tree Fixtures ---
class AVLFixture : public BaseFixture {
public:
    std::unique_ptr<AVL> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<AVL>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};

class RBTreeFixture : public BaseFixture {
public:
    std::unique_ptr<RBTree> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<RBTree>();
        for (int i = 0; i < state.range(0); ++i) {
             structure->insert(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};

class SkipListFixture : public BaseFixture {
public:
    std::unique_ptr<SkipList> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<SkipList>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};

// --- Hash Table Fixtures ---
class HashTableFixture : public BaseFixture {
public:
    std::unique_ptr<HashTable> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<HashTable>(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};

class CuckooHashTableFixture : public BaseFixture {
public:
    std::unique_ptr<CuckooHashTable> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<CuckooHashTable>(state.range(0) * 2); // Cuckoo needs more space
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};

// --- LinkedList Fixture ---
class LinkedListFixture : public BaseFixture {
public:
    std::unique_ptr<DoublyLinkedList> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<DoublyLinkedList>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->append(data_map.at(existing_keys[i]));
        }
    }
    void TearDown(const benchmark::State& state) override { structure.reset(); }
};


// --- PERFORMANCE BENCHMARK DEFINITIONS ---
// AVL
BENCHMARK_DEFINE_F(AVLFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->queryById(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(AVLFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_add = data_map.at(key);
        structure->removeById(key);
        state.ResumeTiming();
        structure->insert(data_to_add);
        i++;
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(AVLFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map.at(key);
        state.ResumeTiming();
        structure->removeById(key);
        state.PauseTiming();
        structure->insert(data_to_readd);
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// RBTree
BENCHMARK_DEFINE_F(RBTreeFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->find(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(RBTreeFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_add = data_map.at(key);
        structure->remove(key);
        state.ResumeTiming();
        structure->insert(data_to_add);
        i++;
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(RBTreeFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map.at(key);
        state.ResumeTiming();
        structure->remove(key);
        state.PauseTiming();
        structure->insert(data_to_readd);
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// SkipList
BENCHMARK_DEFINE_F(SkipListFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->find(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(SkipListFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_add = data_map.at(key);
        structure->remove(key);
        state.ResumeTiming();
        structure->insert(data_to_add);
        i++;
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(SkipListFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map.at(key);
        state.ResumeTiming();
        structure->remove(key);
        state.PauseTiming();
        structure->insert(data_to_readd);
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// HashTable
BENCHMARK_DEFINE_F(HashTableFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->find(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(HashTableFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key_to_find = existing_keys[i % state.range(0)];
        const Data* data_to_add = data_map.at(key_to_find);
        state.ResumeTiming();
        structure->insert(data_to_add);
        i++;
    }
    state.SetComplexityN(state.range(0));
}


// CuckooHashTable
BENCHMARK_DEFINE_F(CuckooHashTableFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->search(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(CuckooHashTableFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key_to_find = existing_keys[i % state.range(0)];
        const Data* data_to_add = data_map.at(key_to_find);
        structure->remove(key_to_find);
        state.ResumeTiming();
        structure->insert(data_to_add);
        i++;
    }
    state.SetComplexityN(state.range(0));
}


// LinkedList
BENCHMARK_DEFINE_F(LinkedListFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->findById(existing_keys[state.iterations() % state.range(0)]));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(LinkedListFixture, Append)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const Data* data_to_add = test_data_pool[state.range(0) + (i % 100)].get();
        state.ResumeTiming();
        structure->append(data_to_add);
        state.PauseTiming();
        structure->removeById(data_to_add->id);
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_DEFINE_F(LinkedListFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map.at(key);
        state.ResumeTiming();
        structure->removeById(key);
        state.PauseTiming();
        structure->append(data_to_readd);
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// --- DIAGNOSTIC BENCHMARKS ---
// Memory Usage
BENCHMARK_DEFINE_F(AVLFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t node_size = sizeof(Data*) + 2 * sizeof(void*) + sizeof(int);
    state.counters["Memory_Bytes"] = state.range(0) * node_size;
}
BENCHMARK_DEFINE_F(RBTreeFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t node_size = sizeof(Data*) + 3 * sizeof(void*) + sizeof(int);
    state.counters["Memory_Bytes"] = state.range(0) * node_size;
}
BENCHMARK_DEFINE_F(SkipListFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t node_size = sizeof(Data*) + 4 * sizeof(void*); // Simplified estimate
    state.counters["Memory_Bytes"] = state.range(0) * node_size;
}
BENCHMARK_DEFINE_F(HashTableFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t entry_size = sizeof(Data*) + sizeof(uint32_t);
    state.counters["Memory_Bytes"] = state.range(0) * entry_size;
}
BENCHMARK_DEFINE_F(CuckooHashTableFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t entry_size = sizeof(Data*);
    state.counters["Memory_Bytes"] = state.range(0) * entry_size * 2; // Account for the larger table size
}
BENCHMARK_DEFINE_F(LinkedListFixture, MemoryUsage)(benchmark::State& state) {
    for(auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    size_t node_size = sizeof(Data*) + 2 * sizeof(void*);
    state.counters["Memory_Bytes"] = state.range(0) * node_size;
}

// Collision Info
BENCHMARK_DEFINE_F(HashTableFixture, CollisionInfo)(benchmark::State& state) {
    for (auto _ : state) { benchmark::DoNotOptimize(structure.get()); }
    CollisionInfo info = structure->getCollisionInfo();
    state.counters["CollidingBuckets"] = info.colliding_buckets;
    state.counters["CollisionRate(%)"] = info.collision_rate_percent;
}

// --- Registering Benchmarks ---
const int min_range = 1 << 10;
const int max_range = 1 << 16; 

// AVL
BENCHMARK_REGISTER_F(AVLFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(AVLFixture, Insert)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(AVLFixture, Remove)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(AVLFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);

// RBTree
BENCHMARK_REGISTER_F(RBTreeFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(RBTreeFixture, Insert)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(RBTreeFixture, Remove)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(RBTreeFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);

// SkipList
BENCHMARK_REGISTER_F(SkipListFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(SkipListFixture, Insert)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(SkipListFixture, Remove)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oLogN);
BENCHMARK_REGISTER_F(SkipListFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);

// HashTables
BENCHMARK_REGISTER_F(HashTableFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::o1);
BENCHMARK_REGISTER_F(HashTableFixture, Insert)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::o1);
BENCHMARK_REGISTER_F(HashTableFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);
BENCHMARK_REGISTER_F(HashTableFixture, CollisionInfo)->RangeMultiplier(2)->Range(min_range, max_range);

BENCHMARK_REGISTER_F(CuckooHashTableFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::o1);
BENCHMARK_REGISTER_F(CuckooHashTableFixture, Insert)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::o1);
BENCHMARK_REGISTER_F(CuckooHashTableFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);
// Note: The CollisionInfo test for Cuckoo has been removed as it does not apply directly.

// LinkedList
BENCHMARK_REGISTER_F(LinkedListFixture, Find)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oN);
BENCHMARK_REGISTER_F(LinkedListFixture, Append)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::o1);
BENCHMARK_REGISTER_F(LinkedListFixture, Remove)->RangeMultiplier(2)->Range(min_range, max_range)->Complexity(benchmark::oN);
BENCHMARK_REGISTER_F(LinkedListFixture, MemoryUsage)->RangeMultiplier(2)->Range(min_range, max_range);

BENCHMARK_MAIN();


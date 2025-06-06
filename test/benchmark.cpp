#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <memory>
#include <algorithm>
#include <set>
#include <iostream>
#include <iomanip>
#include <unordered_map>

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

    while (unique_ids.size() < n) {
        unique_ids.insert(dist(rng));
    }

    test_data_pool.reserve(n);
    existing_keys.reserve(n);
    
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

// --- Fixtures ---
class AVLFixture : public BaseFixture {
public:
    std::unique_ptr<AVL> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<AVL>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(test_data_pool[i].get());
        }
    }
    void TearDown(const benchmark::State& state) override {
        structure.reset();
    }
};

class RBTreeFixture : public BaseFixture {
public:
    std::unique_ptr<RBTree> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<RBTree>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(test_data_pool[i].get());
        }
    }
    void TearDown(const benchmark::State& state) override {
        structure.reset();
    }
};

class SkipListFixture : public BaseFixture {
public:
    std::unique_ptr<SkipList> structure;
    void SetUp(const benchmark::State& state) override {
        BaseFixture::SetUp(state);
        structure = std::make_unique<SkipList>();
        for (int i = 0; i < state.range(0); ++i) {
            structure->insert(test_data_pool[i].get());
        }
    }
    void TearDown(const benchmark::State& state) override {
        structure.reset();
    }
};

// --- PERFORMANCE BENCHMARK DEFINITIONS ---

// --- AVL Benchmarks ---
BENCHMARK_DEFINE_F(AVLFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->queryById(existing_keys[0]));
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(AVLFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        structure->removeById(key);
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();
        
        structure->insert(data_to_readd);
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(AVLFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();

        structure->removeById(key);

        state.PauseTiming();
        structure->insert(data_to_readd); // Re-insert to maintain size
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(AVLFixture, MixedLatency)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key_to_find = existing_keys[i % state.range(0)];
        uint32_t key_to_remove_add = existing_keys[(i + 1) % state.range(0)];
        const Data* data_to_readd = data_map[key_to_remove_add];
        state.ResumeTiming();

        benchmark::DoNotOptimize(structure->queryById(key_to_find));
        structure->removeById(key_to_remove_add);
        structure->insert(data_to_readd);
        
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// --- RBTree Benchmarks ---
BENCHMARK_DEFINE_F(RBTreeFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->find(existing_keys[0]));
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(RBTreeFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        structure->remove(key);
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();
        
        structure->insert(data_to_readd);
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(RBTreeFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();

        structure->remove(key);

        state.PauseTiming();
        structure->insert(data_to_readd); // Re-insert to maintain size
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(RBTreeFixture, MixedLatency)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key_to_find = existing_keys[i % state.range(0)];
        uint32_t key_to_remove_add = existing_keys[(i + 1) % state.range(0)];
        const Data* data_to_readd = data_map[key_to_remove_add];
        state.ResumeTiming();

        benchmark::DoNotOptimize(structure->find(key_to_find));
        structure->remove(key_to_remove_add);
        structure->insert(data_to_readd);
        
        i++;
    }
    state.SetComplexityN(state.range(0));
}

// --- SkipList Benchmarks ---
BENCHMARK_DEFINE_F(SkipListFixture, Find)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure->find(existing_keys[0]));
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(SkipListFixture, Insert)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        structure->remove(key);
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();
        
        structure->insert(data_to_readd);
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(SkipListFixture, Remove)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key = existing_keys[i % state.range(0)];
        const Data* data_to_readd = data_map[key];
        state.ResumeTiming();

        structure->remove(key);

        state.PauseTiming();
        structure->insert(data_to_readd); // Re-insert to maintain size
        state.ResumeTiming();
        i++;
    }
    state.SetComplexityN(state.range(0));
}

BENCHMARK_DEFINE_F(SkipListFixture, MixedLatency)(benchmark::State& state) {
    long i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        uint32_t key_to_find = existing_keys[i % state.range(0)];
        uint32_t key_to_remove_add = existing_keys[(i + 1) % state.range(0)];
        const Data* data_to_readd = data_map[key_to_remove_add];
        state.ResumeTiming();

        benchmark::DoNotOptimize(structure->find(key_to_find));
        structure->remove(key_to_remove_add);
        structure->insert(data_to_readd);
        
        i++;
    }
    state.SetComplexityN(state.range(0));
}


// --- NEW: MEMORY USAGE BENCHMARKS ---

BENCHMARK_DEFINE_F(AVLFixture, MemoryUsage)(benchmark::State& state) {
    // Estimativa do tamanho de um nó da árvore AVL:
    // 1 ponteiro para os dados + 2 ponteiros para os filhos + 1 inteiro para a altura.
    size_t node_size = sizeof(Data*) + 2 * sizeof(void*) + sizeof(int);
    size_t total_memory = state.range(0) * node_size;
    state.counters["Memory_Bytes"] = benchmark::Counter(total_memory, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(structure.get());
    }
}

BENCHMARK_DEFINE_F(RBTreeFixture, MemoryUsage)(benchmark::State& state) {
    // Estimativa do tamanho de um nó da árvore Red-Black:
    // 1 ponteiro para os dados + 2 ponteiros para os filhos + 1 ponteiro para o pai + 1 inteiro para a cor.
    size_t node_size = sizeof(Data*) + 3 * sizeof(void*) + sizeof(int);
    size_t total_memory = state.range(0) * node_size;
    state.counters["Memory_Bytes"] = benchmark::Counter(total_memory, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);

    for (auto _ : state) {
        benchmark::DoNotOptimize(structure.get());
    }
}

BENCHMARK_DEFINE_F(SkipListFixture, MemoryUsage)(benchmark::State& state) {
    // Estimativa do tamanho de um nó da Skip List é mais complexa devido ao array de ponteiros.
    // Usaremos uma estimativa simplificada: 1 ponteiro para os dados + uma média de 4 ponteiros "forward".
    size_t node_size = sizeof(Data*) + 4 * sizeof(void*);
    size_t total_memory = state.range(0) * node_size;
    state.counters["Memory_Bytes"] = benchmark::Counter(total_memory, benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);

    for (auto _ : state) {
        benchmark::DoNotOptimize(structure.get());
    }
}


// --- Análise dos Resultados ---
//
// 1. Complexidade Assintótica Real vs. Teórica:
//    Dentro de cada teste de DESEMPENHO (Find, Insert, etc.), a linha:
//    `state.SetComplexityN(state.range(0));`
//    informa à biblioteca o 'N' (tamanho da entrada). Ao rodar os testes, a
//    biblioteca medirá o tempo para diferentes 'N's e calculará a complexidade
//    Big-O observada.
//
//    COMO LER: Na saída, procure as colunas "Time" e "Big-O". Para as árvores
//    e Skip List, a complexidade teórica é O(logN). O benchmark mostrará quão
//    próximo o resultado prático está disso. Um valor baixo na coluna "RMS"
//    (erro) indica um bom ajuste entre a teoria e a prática.
//
// 2. Uso de Memória:
//    Os novos testes `MemoryUsage` não medem tempo, mas sim reportam o uso de
//    memória estimado. Eles fazem isso usando `state.counters`.
//
//    COMO LER: Na saída, procure pelas linhas `.../MemoryUsage`. Elas terão uma
//    coluna extra chamada "Memory_Bytes", mostrando o total de bytes estimados
//    para a estrutura com 'N' elementos. Isso permite comparar o custo de
//    memória entre as diferentes estruturas de dados.


// --- Registering Benchmarks ---
const int min_range = 1 << 10;
const int max_range = 1 << 16; 

#define REGISTER_PERFORMANCE_BENCHMARKS(Fixture) \
    BENCHMARK_REGISTER_F(Fixture, Find)->RangeMultiplier(4)->Range(min_range, max_range); \
    BENCHMARK_REGISTER_F(Fixture, Insert)->RangeMultiplier(4)->Range(min_range, max_range); \
    BENCHMARK_REGISTER_F(Fixture, Remove)->RangeMultiplier(4)->Range(min_range, max_range); \
    BENCHMARK_REGISTER_F(Fixture, MixedLatency)->RangeMultiplier(4)->Range(min_range, max_range);

#define REGISTER_MEMORY_BENCHMARK(Fixture) \
    BENCHMARK_REGISTER_F(Fixture, MemoryUsage)->RangeMultiplier(4)->Range(min_range, max_range);

// Registrando todos os benchmarks
REGISTER_PERFORMANCE_BENCHMARKS(AVLFixture);
REGISTER_MEMORY_BENCHMARK(AVLFixture);

REGISTER_PERFORMANCE_BENCHMARKS(RBTreeFixture);
REGISTER_MEMORY_BENCHMARK(RBTreeFixture);

REGISTER_PERFORMANCE_BENCHMARKS(SkipListFixture);
REGISTER_MEMORY_BENCHMARK(SkipListFixture);

BENCHMARK_MAIN();


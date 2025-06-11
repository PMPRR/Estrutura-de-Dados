// Criado por Gemini AI, baseado no código de Luiz Henrique 

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <string> 
#include <vector>
#include <list>
#include <cstdint> 
#include "data.h"  

// NEW: Struct to hold collision and load factor information
struct CollisionInfo {
    size_t total_buckets;
    size_t used_buckets;
    size_t colliding_buckets;
    size_t max_chain_length;
    float load_factor;
    float collision_rate_percent;

    size_t total_memory_bytes;
};


class HashTable {
public:

    // Constructor
    explicit HashTable(size_t capacidade = 101);

    // Insere um Data* na tabela usando seu ID como chave
    void insert(const Data* data);

    // Remove um Data* pela chave (ID)
    bool remove(uint32_t id);

    // Busca Data* pelo ID, retorna nullptr se não encontrar
    const Data* find(uint32_t id) const;

    // Limpa toda a tabela (remove apenas os ponteiros e Nodes, não Data* em si)
    void clear();

    // Retorna o número de elementos armazenados
    size_t size() const;

    // NEW: Method to get collision and other stats
    CollisionInfo getCollisionInfo() const;

    // Destrutor (limpa apenas os Nodes, não os Data* apontados)
    ~HashTable();

private:
    // Estrutura para os Nodes da tabela hash
    struct Node {
        const Data* data; // Agora armazena um ponteiro para Data
        Node(const Data* d) : data(d) {}
    };

    std::vector< std::list<Node> > table;
    size_t itemCount;

    // Função hash para uint32_t
    size_t hash(uint32_t key) const;
};

#endif


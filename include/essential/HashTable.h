#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <string> // Still needed for std::string if hash function uses it, but not for Node directly
#include <vector>
#include <list>
#include <cstdint> // For uint32_t
#include "data.h"  // Include Data struct definition

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


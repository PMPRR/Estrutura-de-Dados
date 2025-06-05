#pragma once
#include <string>
#include <vector>
#include <list>

namespace essential {

class HashTable {
public:
    // Construtor
    explicit HashTable(size_t capacidade = 101);

    // Insere um par chave-valor na tabela
    void insert(const std::string &key, const std::string &value);

    // Remove uma chave
    bool remove(const std::string &key);

    // Busca valor pela chave, retorna nullptr se não encontrar
    const std::string* find(const std::string &key) const;

    // Limpa toda a tabela
    void clear();

    // Retorna o número de elementos armazenados
    size_t size() const;

    // Destrutor
    ~HashTable();

private:
    // Estrutura para os pares chave-valor
    struct Node {
        std::string key;
        std::string value;
        Node(const std::string &k, const std::string &v) : key(k), value(v) {}
    };

    std::vector< std::list<Node> > table;
    size_t itemCount;

    // Função hash simples para string
    size_t hash(const std::string &key) const;
};

} // namespace essential

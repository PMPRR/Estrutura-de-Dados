#ifndef RBTREE_H
#define RBTREE_H

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <stdexcept>

namespace essential {

// Implementação simples de optional para compatibilidade com GCC 6.3.0
template<typename T>
class Optional {
private:
    bool has_value_;
    T value_;
    
public:
    Optional() : has_value_(false) {}
    Optional(const T& value) : has_value_(true), value_(value) {}
    
    bool has_value() const { return has_value_; }
    const T& value() const { 
        if (!has_value_) throw std::runtime_error("Accessing empty Optional");
        return value_; 
    }
    T& value() { 
        if (!has_value_) throw std::runtime_error("Accessing empty Optional");
        return value_; 
    }
};

template <typename Key, typename Value>
class RBTree {
public:
    enum class Color { RED, BLACK };

    struct Node {
        Key key;
        Value value;
        Color color;
        Node* parent;
        Node* left;
        Node* right;

        Node(const Key& k, const Value& v, Color c = Color::RED)
            : key(k), value(v), color(c), parent(nullptr), left(nullptr), right(nullptr) {}
    };

    // Construtor e Destrutor
    RBTree();
    ~RBTree();

    // Operações básicas
    void insert(const Key& key, const Value& value);
    bool remove(const Key& key);
    Optional<Value> find(const Key& key) const;
    bool contains(const Key& key) const;
    void clear();
    bool empty() const;
    size_t size() const;

    // Iteração
    void inorder(const std::function<void(const Key&, const Value&)>& callback) const;
    void preorder(const std::function<void(const Key&, const Value&)>& callback) const;
    void postorder(const std::function<void(const Key&, const Value&)>& callback) const;

    // Métodos para depuração
    void printTree() const;
    bool verifyProperties() const;

private:
    Node* root_;
    Node* nil_;  // Sentinela para representar folhas NULL
    size_t size_;

    // Métodos auxiliares para operações de árvore
    Node* search(const Key& key) const;
    void leftRotate(Node* x);
    void rightRotate(Node* y);
    void insertFixup(Node* z);
    void transplant(Node* u, Node* v);
    Node* minimum(Node* x) const;
    void deleteFixup(Node* x);
    void deleteNode(Node* z);

    // Métodos auxiliares para percorrer a árvore
    void inorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const;
    void preorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const;
    void postorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const;

    // Métodos auxiliares para depuração
    void printNode(Node* node, int indent = 0) const;
    bool verifyProperty1(Node* node) const;  // Cada nó é vermelho ou preto
    bool verifyProperty2() const;            // A raiz é preta
    bool verifyProperty3() const;            // Todas as folhas (NIL) são pretas
    bool verifyProperty4(Node* node) const;  // Se um nó é vermelho, seus filhos são pretos
    int verifyProperty5(Node* node) const;   // Todos os caminhos de um nó para suas folhas têm o mesmo número de nós pretos

    // Método auxiliar para liberar memória
    void destroyTree(Node* node);
};

} // namespace essential

#include "../../src/essential/RBTree.cpp"  // Incluindo a implementação para templates

#endif // RBTREE_H

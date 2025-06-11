// Criado por Gemini 2.5 pro AI, baseado no código de Luiz Henrique

#ifndef RBTREE_H
#define RBTREE_H

#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <cassert>
#include "data.h" // For Data struct definition

class RBTree {
public:
    enum class Color { RED, BLACK };

    struct Node {
        uint32_t key;
        const Data* value;
        Color color;
        Node* parent;
        Node* left;
        Node* right;

        Node(uint32_t k, const Data* v, Color c = Color::RED)
            : key(k), value(v), color(c), parent(nullptr), left(nullptr), right(nullptr) {}
    };

    // Constructor and Destructor
    RBTree();
    ~RBTree();

    // --- Primary Operations ---
    void insert(const Data* data);
    bool remove(uint32_t key);
    const Data* find(uint32_t key) const;
    bool contains(uint32_t key) const;

    // --- Utility Operations ---
    void clear();
    bool empty() const;
    size_t size() const;

    // --- Debugging ---
    void printTree() const;
    bool verifyProperties() const;

    size_t getMemoryUsage() const;
private:

    size_t getMemoryUsageRecursive(Node* node) const;
    Node* root_;
    Node* nil_;  // Sentinel node
    size_t size_;

    // --- Private Helper Methods ---
    Node* search(uint32_t key) const;
    void leftRotate(Node* x);
    void rightRotate(Node* y);
    void insertFixup(Node* z);
    void transplant(Node* u, Node* v);
    Node* minimum(Node* x) const;
    void deleteFixup(Node* x);
    void deleteNode(Node* z);
    void destroyTree(Node* node);

    void printNode(Node* node, int indent = 0) const;

    bool verifyProperty1(Node* node) const;
    bool verifyProperty2() const;
    bool verifyProperty3() const;
    bool verifyProperty4(Node* node) const;
    int verifyProperty5(Node* node) const;
};

#endif

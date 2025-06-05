#ifndef AVL_H_
#define AVL_H_

#include "data.h"

//struct Node_AVL {
//public:
//    const Data* data;
//    Node_AVL* left;
//    Node_AVL* right;
//    uint16_t height;
//public:
//    Node_AVL(const Data* data) : data(data), left(nullptr), right(nullptr), height(1) {} 
//};

class AVL {
public:
    struct Node_AVL {
    public:
        const Data* data;
        Node_AVL* left;
        Node_AVL* right;
        uint16_t height;
    
    public:
        Node_AVL(const Data* data) : data(data), left(nullptr), right(nullptr), height(1) {} 
    }; 
private:
    Node_AVL* _root;
public:
    AVL() : _root(nullptr) {};
    void insert(const Data* data);
    void printAsciiTree(int indentUnit = 4);
    void printPreOrderHierarchical(int indentUnit = 4);

    Node_AVL* queryById(uint32_t id);

    void removeById(uint32_t id);
private:
    int16_t height(Node_AVL* node);
    int16_t getBalance(Node_AVL* node);

    Node_AVL* leftRotation(Node_AVL* node);
    Node_AVL* rightRotation(Node_AVL* node);

    Node_AVL* insertUtil(const Data* data, Node_AVL* node);
    Node_AVL* removeUtil(Node_AVL* node, uint32_t id);

    void printAsciiRecursive(Node_AVL* node, int currentIndentLevel, int indentUnit);
    void printPreOrderHierarchicalRecursive(Node_AVL* node, int depth, int indentUnit);
};

#endif // !AVL_H_



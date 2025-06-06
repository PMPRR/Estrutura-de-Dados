#ifndef RBTREE_CPP
#define RBTREE_CPP

#include "../../include/essential/RBTree.h"
#include <cassert>
#include <iomanip>

namespace essential {

template <typename Key, typename Value>
RBTree<Key, Value>::RBTree() : size_(0) {
    nil_ = new Node(Key(), Value(), Color::BLACK);
    nil_->left = nil_;
    nil_->right = nil_;
    nil_->parent = nil_;
    root_ = nil_;
}

template <typename Key, typename Value>
RBTree<Key, Value>::~RBTree() {
    clear();
    delete nil_;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::clear() {
    if (root_ != nil_) {
        destroyTree(root_);
        root_ = nil_;
        size_ = 0;
    }
}

template <typename Key, typename Value>
void RBTree<Key, Value>::destroyTree(Node* node) {
    if (node != nil_) {
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::empty() const {
    return size_ == 0;
}

template <typename Key, typename Value>
size_t RBTree<Key, Value>::size() const {
    return size_;
}

template <typename Key, typename Value>
typename RBTree<Key, Value>::Node* RBTree<Key, Value>::search(const Key& key) const {
    Node* current = root_;
    while (current != nil_) {
        if (key == current->key) {
            return current;
        } else if (key < current->key) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return nil_;
}

template <typename Key, typename Value>
Optional<Value> RBTree<Key, Value>::find(const Key& key) const {
    Node* node = search(key);
    if (node != nil_) {
        return Optional<Value>(node->value);
    }
    return Optional<Value>();
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::contains(const Key& key) const {
    return search(key) != nil_;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::leftRotate(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    
    if (y->left != nil_) {
        y->left->parent = x;
    }
    
    y->parent = x->parent;
    
    if (x->parent == nil_) {
        root_ = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    
    y->left = x;
    x->parent = y;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::rightRotate(Node* y) {
    Node* x = y->left;
    y->left = x->right;
    
    if (x->right != nil_) {
        x->right->parent = y;
    }
    
    x->parent = y->parent;
    
    if (y->parent == nil_) {
        root_ = x;
    } else if (y == y->parent->left) {
        y->parent->left = x;
    } else {
        y->parent->right = x;
    }
    
    x->right = y;
    y->parent = x;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::insert(const Key& key, const Value& value) {
    // Verificar se a chave já existe
    Node* existing = search(key);
    if (existing != nil_) {
        // Atualizar o valor se a chave já existir
        existing->value = value;
        return;
    }

    Node* z = new Node(key, value);
    Node* y = nil_;
    Node* x = root_;
    
    while (x != nil_) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else {
            x = x->right;
        }
    }
    
    z->parent = y;
    
    if (y == nil_) {
        root_ = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }
    
    z->left = nil_;
    z->right = nil_;
    z->color = Color::RED;
    
    insertFixup(z);
    size_++;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::insertFixup(Node* z) {
    while (z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right;
            
            if (y->color == Color::RED) {
                // Caso 1: O tio é vermelho
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    // Caso 2: O tio é preto e z é filho direito
                    z = z->parent;
                    leftRotate(z);
                }
                // Caso 3: O tio é preto e z é filho esquerdo
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                rightRotate(z->parent->parent);
            }
        } else {
            // Simétrico ao caso acima, com "esquerda" e "direita" trocados
            Node* y = z->parent->parent->left;
            
            if (y->color == Color::RED) {
                // Caso 1: O tio é vermelho
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    // Caso 2: O tio é preto e z é filho esquerdo
                    z = z->parent;
                    rightRotate(z);
                }
                // Caso 3: O tio é preto e z é filho direito
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                leftRotate(z->parent->parent);
            }
        }
        
        if (z == root_) {
            break;
        }
    }
    
    root_->color = Color::BLACK;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::transplant(Node* u, Node* v) {
    if (u->parent == nil_) {
        root_ = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

template <typename Key, typename Value>
typename RBTree<Key, Value>::Node* RBTree<Key, Value>::minimum(Node* x) const {
    while (x->left != nil_) {
        x = x->left;
    }
    return x;
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::remove(const Key& key) {
    Node* z = search(key);
    if (z == nil_) {
        return false;
    }
    
    deleteNode(z);
    size_--;
    return true;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::deleteNode(Node* z) {
    Node* y = z;
    Node* x;
    Color y_original_color = y->color;
    
    if (z->left == nil_) {
        x = z->right;
        transplant(z, z->right);
    } else if (z->right == nil_) {
        x = z->left;
        transplant(z, z->left);
    } else {
        y = minimum(z->right);
        y_original_color = y->color;
        x = y->right;
        
        if (y->parent == z) {
            x->parent = y;
        } else {
            transplant(y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        
        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    
    delete z;
    
    if (y_original_color == Color::BLACK) {
        deleteFixup(x);
    }
}

template <typename Key, typename Value>
void RBTree<Key, Value>::deleteFixup(Node* x) {
    while (x != root_ && x->color == Color::BLACK) {
        if (x == x->parent->left) {
            Node* w = x->parent->right;
            
            if (w->color == Color::RED) {
                // Caso 1: O irmão de x é vermelho
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                leftRotate(x->parent);
                w = x->parent->right;
            }
            
            if (w->left->color == Color::BLACK && w->right->color == Color::BLACK) {
                // Caso 2: O irmão de x é preto e ambos os filhos do irmão são pretos
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->right->color == Color::BLACK) {
                    // Caso 3: O irmão de x é preto, o filho esquerdo do irmão é vermelho e o direito é preto
                    w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    rightRotate(w);
                    w = x->parent->right;
                }
                
                // Caso 4: O irmão de x é preto e o filho direito do irmão é vermelho
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->right->color = Color::BLACK;
                leftRotate(x->parent);
                x = root_;
            }
        } else {
            // Simétrico ao caso acima, com "esquerda" e "direita" trocados
            Node* w = x->parent->left;
            
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                rightRotate(x->parent);
                w = x->parent->left;
            }
            
            if (w->right->color == Color::BLACK && w->left->color == Color::BLACK) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->left->color == Color::BLACK) {
                    w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    leftRotate(w);
                    w = x->parent->left;
                }
                
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->left->color = Color::BLACK;
                rightRotate(x->parent);
                x = root_;
            }
        }
    }
    
    x->color = Color::BLACK;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::inorder(const std::function<void(const Key&, const Value&)>& callback) const {
    inorderTraversal(root_, callback);
}

template <typename Key, typename Value>
void RBTree<Key, Value>::inorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const {
    if (node != nil_) {
        inorderTraversal(node->left, callback);
        callback(node->key, node->value);
        inorderTraversal(node->right, callback);
    }
}

template <typename Key, typename Value>
void RBTree<Key, Value>::preorder(const std::function<void(const Key&, const Value&)>& callback) const {
    preorderTraversal(root_, callback);
}

template <typename Key, typename Value>
void RBTree<Key, Value>::preorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const {
    if (node != nil_) {
        callback(node->key, node->value);
        preorderTraversal(node->left, callback);
        preorderTraversal(node->right, callback);
    }
}

template <typename Key, typename Value>
void RBTree<Key, Value>::postorder(const std::function<void(const Key&, const Value&)>& callback) const {
    postorderTraversal(root_, callback);
}

template <typename Key, typename Value>
void RBTree<Key, Value>::postorderTraversal(Node* node, const std::function<void(const Key&, const Value&)>& callback) const {
    if (node != nil_) {
        postorderTraversal(node->left, callback);
        postorderTraversal(node->right, callback);
        callback(node->key, node->value);
    }
}

template <typename Key, typename Value>
void RBTree<Key, Value>::printTree() const {
    if (root_ == nil_) {
        std::cout << "Árvore vazia" << std::endl;
        return;
    }
    
    printNode(root_);
    std::cout << std::endl;
}

template <typename Key, typename Value>
void RBTree<Key, Value>::printNode(Node* node, int indent) const {
    if (node == nil_) {
        return;
    }
    
    // Aumentar o recuo para o filho direito
    printNode(node->right, indent + 4);
    
    // Imprimir o nó atual
    std::cout << std::setw(indent) << " ";
    std::string color = (node->color == Color::RED) ? "R" : "B";
    std::cout << node->key << "(" << color << ")" << std::endl;
    
    // Imprimir o filho esquerdo
    printNode(node->left, indent + 4);
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::verifyProperties() const {
    if (!verifyProperty1(root_)) {
        std::cerr << "Propriedade 1 violada: Nem todos os nós são vermelhos ou pretos." << std::endl;
        return false;
    }
    
    if (!verifyProperty2()) {
        std::cerr << "Propriedade 2 violada: A raiz não é preta." << std::endl;
        return false;
    }
    
    if (!verifyProperty3()) {
        std::cerr << "Propriedade 3 violada: Nem todas as folhas (NIL) são pretas." << std::endl;
        return false;
    }
    
    if (!verifyProperty4(root_)) {
        std::cerr << "Propriedade 4 violada: Um nó vermelho tem um filho vermelho." << std::endl;
        return false;
    }
    
    if (verifyProperty5(root_) == -1) {
        std::cerr << "Propriedade 5 violada: Nem todos os caminhos da raiz às folhas têm o mesmo número de nós pretos." << std::endl;
        return false;
    }
    
    return true;
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::verifyProperty1(Node* node) const {
    if (node == nil_) {
        return true;
    }
    
    if (node->color != Color::RED && node->color != Color::BLACK) {
        return false;
    }
    
    return verifyProperty1(node->left) && verifyProperty1(node->right);
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::verifyProperty2() const {
    return root_->color == Color::BLACK;
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::verifyProperty3() const {
    return nil_->color == Color::BLACK;
}

template <typename Key, typename Value>
bool RBTree<Key, Value>::verifyProperty4(Node* node) const {
    if (node == nil_) {
        return true;
    }
    
    if (node->color == Color::RED) {
        if (node->left->color == Color::RED || node->right->color == Color::RED) {
            return false;
        }
    }
    
    return verifyProperty4(node->left) && verifyProperty4(node->right);
}

template <typename Key, typename Value>
int RBTree<Key, Value>::verifyProperty5(Node* node) const {
    if (node == nil_) {
        return 1;  // Altura preta de uma folha NIL é 1
    }
    
    int leftBlackHeight = verifyProperty5(node->left);
    int rightBlackHeight = verifyProperty5(node->right);
    
    if (leftBlackHeight == -1 || rightBlackHeight == -1 || leftBlackHeight != rightBlackHeight) {
        return -1;  // Propriedade violada
    }
    
    // Adicionar 1 à altura preta se o nó atual for preto
    return (node->color == Color::BLACK) ? leftBlackHeight + 1 : leftBlackHeight;
}

} // namespace essential

#endif // RBTREE_CPP

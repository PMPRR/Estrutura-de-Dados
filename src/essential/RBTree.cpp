// Criado por Gemini AI 2.5 pro, baeado no código de Luiz Henrique

#include "essential/RBTree.h"

size_t RBTree::getMemoryUsage() const {
    if (root_ == nil_) return sizeof(*nil_);
    return getMemoryUsageRecursive(root_) + sizeof(*nil_);
}
size_t RBTree::getMemoryUsageRecursive(Node* node) const {
    if (node == nil_) return 0;
    return sizeof(Node) + getMemoryUsageRecursive(node->left) + getMemoryUsageRecursive(node->right);
}

RBTree::RBTree() : size_(0) {
    // nil_ sentinel node setup
    nil_ = new Node(0, nullptr, Color::BLACK);
    nil_->left = nil_;
    nil_->right = nil_;
    nil_->parent = nil_;
    root_ = nil_;
}

RBTree::~RBTree() {
    clear();
    delete nil_;
}

void RBTree::clear() {
    if (root_ != nil_) {
        destroyTree(root_);
        root_ = nil_;
        size_ = 0;
    }
}

void RBTree::destroyTree(Node* node) {
    if (node != nil_) {
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }
}

bool RBTree::empty() const {
    return size_ == 0;
}

size_t RBTree::size() const {
    return size_;
}

typename RBTree::Node* RBTree::search(uint32_t key) const {
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

const Data* RBTree::find(uint32_t key) const {
    Node* node = search(key);
    if (node != nil_) {
        return node->value; // Return the raw pointer
    }
    return nullptr; // Return nullptr if not found
}

bool RBTree::contains(uint32_t key) const {
    return search(key) != nil_;
}

void RBTree::leftRotate(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    if (y->left != nil_) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == nil_) root_ = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void RBTree::rightRotate(Node* y) {
    Node* x = y->left;
    y->left = x->right;
    if (x->right != nil_) x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == nil_) root_ = x;
    else if (y == y->parent->left) y->parent->left = x;
    else y->parent->right = x;
    x->right = y;
    y->parent = x;
}

void RBTree::insert(const Data* data) {
    if (!data) return; 

    uint32_t key = data->id;
    const Data* value = data;

    Node* existing = search(key);
    if (existing != nil_) {
        existing->value = value;
        return;
    }

    Node* z = new Node(key, value);
    Node* y = nil_;
    Node* x = root_;
    
    while (x != nil_) {
        y = x;
        if (z->key < x->key) x = x->left;
        else x = x->right;
    }
    
    z->parent = y;
    
    if (y == nil_) root_ = z;
    else if (z->key < y->key) y->left = z;
    else y->right = z;
    
    z->left = nil_;
    z->right = nil_;
    z->color = Color::RED;
    
    insertFixup(z);
    size_++;
}

void RBTree::insertFixup(Node* z) {
    while (z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right; // Uncle
            if (y->color == Color::RED) { // Case 1: Uncle is RED
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) { // Case 2: Uncle is BLACK, z is a right child
                    z = z->parent;
                    leftRotate(z);
                }
                // Case 3: Uncle is BLACK, z is a left child
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                rightRotate(z->parent->parent);
            }
        } else { // Symmetric case
            Node* y = z->parent->parent->left; // Uncle
            if (y->color == Color::RED) { // Case 1
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) { // Case 2
                    z = z->parent;
                    rightRotate(z);
                }
                // Case 3
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                leftRotate(z->parent->parent);
            }
        }
        if (z == root_) break;
    }
    root_->color = Color::BLACK;
}

void RBTree::transplant(Node* u, Node* v) {
    if (u->parent == nil_) root_ = v;
    else if (u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;
    v->parent = u->parent;
}

typename RBTree::Node* RBTree::minimum(Node* x) const {
    while (x->left != nil_) x = x->left;
    return x;
}

bool RBTree::remove(uint32_t key) {
    Node* z = search(key);
    if (z == nil_) return false;
    deleteNode(z);
    size_--;
    return true;
}

void RBTree::deleteNode(Node* z) {
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

void RBTree::deleteFixup(Node* x) {
    while (x != root_ && x->color == Color::BLACK) {
        if (x == x->parent->left) {
            Node* w = x->parent->right; // Sibling
            if (w->color == Color::RED) { // Case 1
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                leftRotate(x->parent);
                w = x->parent->right;
            }
            if (w->left->color == Color::BLACK && w->right->color == Color::BLACK) { // Case 2
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->right->color == Color::BLACK) { // Case 3
                    w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    rightRotate(w);
                    w = x->parent->right;
                }
                // Case 4
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->right->color = Color::BLACK;
                leftRotate(x->parent);
                x = root_;
            }
        } else { // Symmetric case
            Node* w = x->parent->left; // Sibling
            if (w->color == Color::RED) { // Case 1
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                rightRotate(x->parent);
                w = x->parent->left;
            }
            if (w->right->color == Color::BLACK && w->left->color == Color::BLACK) { // Case 2
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->left->color == Color::BLACK) { // Case 3
                    w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    leftRotate(w);
                    w = x->parent->left;
                }
                // Case 4
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

void RBTree::printTree() const {
    if (root_ == nil_) {
        std::cout << "Tree is empty" << std::endl;
        return;
    }
    printNode(root_);
    std::cout << std::endl;
}

void RBTree::printNode(Node* node, int indent) const {
    if (node == nil_) return;
    printNode(node->right, indent + 4);
    std::cout << std::setw(indent) << " ";
    std::string color = (node->color == Color::RED) ? "R" : "B";
    std::cout << node->key << "(" << color << ")" << std::endl;
    printNode(node->left, indent + 4);
}

// Verification implementations
bool RBTree::verifyProperties() const {
    if (!verifyProperty1(root_)) return false;
    if (!verifyProperty2()) return false;
    if (!verifyProperty3()) return false;
    if (!verifyProperty4(root_)) return false;
    if (verifyProperty5(root_) == -1) return false;
    return true;
}

bool RBTree::verifyProperty1(Node* node) const {
    if (node == nil_) return true;
    if (node->color != Color::RED && node->color != Color::BLACK) return false;
    return verifyProperty1(node->left) && verifyProperty1(node->right);
}

bool RBTree::verifyProperty2() const {
    return root_->color == Color::BLACK;
}

bool RBTree::verifyProperty3() const {
    return nil_->color == Color::BLACK;
}

bool RBTree::verifyProperty4(Node* node) const {
    if (node == nil_) return true;
    if (node->color == Color::RED) {
        if (node->left->color == Color::RED || node->right->color == Color::RED) return false;
    }
    return verifyProperty4(node->left) && verifyProperty4(node->right);
}

int RBTree::verifyProperty5(Node* node) const {
    if (node == nil_) return 1;
    int leftBlackHeight = verifyProperty5(node->left);
    int rightBlackHeight = verifyProperty5(node->right);
    if (leftBlackHeight == -1 || rightBlackHeight == -1 || leftBlackHeight != rightBlackHeight) return -1;
    return (node->color == Color::BLACK) ? leftBlackHeight + 1 : leftBlackHeight;
}

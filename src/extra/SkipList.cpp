#include "extra/SkipList.h"
#include <iostream>
#include <cstdlib> // For malloc, free, rand
#include <ctime>   // For srand
#include <new>     // For placement new
#include <vector>
#include <iomanip> // For std::setw

// Constructor
SkipList::SkipList(int max_level, float p)
    : MAX_LEVEL_(max_level), P_(p), current_level_(0), size_(0) {
    // Seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create the header node. Its key and value don't matter.
    // It will have the maximum number of levels to serve as the entry point.
    head_ = createNode(MAX_LEVEL_, 0, nullptr);
    for (int i = 0; i < MAX_LEVEL_; ++i) {
        head_->forward[i] = nullptr; // Initialize all forward pointers to null
    }
}

// Destructor
SkipList::~SkipList() {
    clear();
    free(head_); // Free the header node's memory
}

// Private helper to create a node using the flexible array member trick
SkipList::Node* SkipList::createNode(int level, uint32_t key, const Data* value) {
    // Allocate memory for the node struct plus additional forward pointers
    size_t node_size = sizeof(Node) + (level - 1) * sizeof(Node*);
    void* memory = malloc(node_size);
    if (!memory) {
        throw std::bad_alloc(); // Failed to allocate memory
    }
    
    // Use placement new to construct the Node object in the allocated memory
    Node* node = new (memory) Node{key, value, level};
    return node;
}

// Private helper to generate a random level for a new node
int SkipList::randomLevel() {
    int level = 1;
    // Keep increasing the level based on the probability P_
    // This loop continues as long as a random float is less than P
    // and we haven't reached the maximum allowed level.
    while ((static_cast<float>(rand()) / RAND_MAX) < P_ && level < MAX_LEVEL_) {
        level++;
    }
    return level;
}

// Insert a new data element
void SkipList::insert(const Data* data) {
    if (!data) return; // Do not insert null data pointers
    uint32_t key = data->id;

    // `update` will store the nodes that need their `forward` pointers updated.
    std::vector<Node*> update(MAX_LEVEL_ + 1);
    Node* current = head_;

    // Start from the highest level and find the insertion point at each level
    for (int i = current_level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // Move to the level 0 insertion point
    current = current->forward[0];

    // If a node with the same key already exists, update its value
    if (current != nullptr && current->key == key) {
        current->value = data;
        return;
    }

    // If key does not exist, generate a random level for the new node
    int new_level = randomLevel();
    
    // If the new level is higher than the list's current level,
    // update the list's level and the `update` vector for the new levels.
    if (new_level > current_level_) {
        for (int i = current_level_ + 1; i < new_level; i++) {
            update[i] = head_;
        }
        current_level_ = new_level -1; // Adjust current_level_ to be 0-indexed
    }
    
    // Create the new node
    Node* new_node = createNode(new_level, key, data);
    
    // Splice the new node into the list at all its levels
    for (int i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    size_++;
}

// Find a data element by its key
const Data* SkipList::find(uint32_t key) const {
    Node* current = head_;

    // Start from the highest level and traverse down
    for (int i = current_level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }
    
    // Move to level 0 to find the exact node
    current = current->forward[0];
    
    // Check if the node was found
    if (current != nullptr && current->key == key) {
        return current->value;
    }
    
    return nullptr; // Not found
}

// Remove a data element by its key
bool SkipList::remove(uint32_t key) {
    std::vector<Node*> update(MAX_LEVEL_ + 1);
    Node* current = head_;

    // Find the node to be removed, tracking the update path
    for (int i = current_level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];

    // If the node exists
    if (current != nullptr && current->key == key) {
        // Unlink the node at all its levels
        for (int i = 0; i < current->level; i++) {
            if (update[i]->forward[i] != current) {
                break; // Should not happen in a consistent list
            }
            update[i]->forward[i] = current->forward[i];
        }

        // Deallocate the node's memory
        // First, explicitly call destructor because of placement new
        current->~Node();
        // Then, free the raw memory
        free(current);
        
        // Update the list's current level if necessary
        while (current_level_ > 0 && head_->forward[current_level_] == nullptr) {
            current_level_--;
        }

        size_--;
        return true;
    }

    return false; // Node not found
}

// Check if empty
bool SkipList::empty() const {
    return size_ == 0;
}

// Get size
size_t SkipList::size() const {
    return size_;
}

// Private helper to clear the list
void SkipList::clear() {
    Node* current = head_->forward[0];
    while (current != nullptr) {
        Node* next = current->forward[0];
        current->~Node(); // Explicitly call destructor
        free(current);    // Free raw memory
        current = next;
    }

    // Reset all header pointers
    for (int i = 0; i <= MAX_LEVEL_; ++i) {
        if(i < MAX_LEVEL_) head_->forward[i] = nullptr;
    }
    current_level_ = 0;
    size_ = 0;
}

// Print the skip list for debugging
void SkipList::printList() const {
    std::cout << "\n--- Skip List ---" << std::endl;
    for (int i = current_level_; i >= 0; i--) {
        Node* node = head_->forward[i];
        std::cout << "Level " << std::setw(2) << i << ": ";
        while (node != nullptr) {
            std::cout << node->key << " ";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
    std::cout << "-----------------" << std::endl;
}


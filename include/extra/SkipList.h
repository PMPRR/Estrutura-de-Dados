#ifndef SKIPLIST_H
#define SKIPLIST_H

#include "data.h"
#include <vector>
#include <cstdint>

class SkipList {
public:
    // Constructor: Initializes an empty skip list.
    // max_level: The theoretical maximum number of levels this list can have.
    // p: The probability (from 0.0 to 1.0) for a node to have an additional level.
    explicit SkipList(int max_level = 16, float p = 0.5f);

    // Destructor: Frees all nodes in the skip list.
    ~SkipList();

    // Inserts a pointer to a Data object into the skip list.
    // The key for sorting is the Data object's 'id'.
    // The skip list does not take ownership of the Data pointer.
    void insert(const Data* data);

    // Removes a Data object by its ID.
    // Returns true if the element was found and removed, false otherwise.
    bool remove(uint32_t key);

    // Finds a Data object by its ID.
    // Returns a const pointer to the Data object if found, nullptr otherwise.
    const Data* find(uint32_t key) const;

    // Checks if the skip list is empty.
    bool empty() const;

    // Returns the number of elements in the skip list.
    size_t size() const;
    
    // Prints a visual representation of the skip list to the console for debugging.
    void printList() const;

private:
    struct Node {
        uint32_t key;
        const Data* value;
        int level; // Store the actual level of this node

        // Flexible array member trick. Must be the LAST member.
        Node* forward[1];
    };

    // Private helper to create a new node with a specific level (and thus size).
    Node* createNode(int level, uint32_t key, const Data* value);

    // Generates a random level for a new node based on probability P.
    int randomLevel();

    // Frees all nodes and resets the skip list.
    void clear();

    const int MAX_LEVEL_; // Maximum possible level for this list
    const float P_;       // Probability for level generation

    int current_level_;   // The highest level currently in use in the list
    size_t size_;         // Number of elements in the list
    Node* head_;          // Pointer to the header node (acts as entry point)
};

#endif // SKIPLIST_H


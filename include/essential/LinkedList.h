#ifndef DOUBLYLINKEDLIST_H
#define DOUBLYLINKEDLIST_H

#include "data.h"
#include <iostream>
#include <vector>
#include <cstdint> // For uint32_t

class DoublyLinkedList {
private:
    struct Node {
        const Data* data; // Stores a pointer to the Data object
        Node* prev;
        Node* next;
        Node(const Data* d); // Constructor takes a const Data pointer
    };

    Node* head;
    Node* tail;
    int count;

public:
    DoublyLinkedList();
    ~DoublyLinkedList(); // Destructor will only delete Node objects, not Data

    void append(const Data* d); // Takes a const Data pointer
    void insertAt(int index, const Data* d); // Takes a const Data pointer
    const Data* findById(uint32_t id); // Returns const Data pointer
    bool removeById(uint32_t id); // Removes node, does NOT delete Data
    int size() const;

    // Example: statistics on Data::dur (duration)
    float average_dur();
    float stddev_dur();
    float median_dur();
    float min_dur();
    float max_dur();
    void histogram_dur(int bins);

    // You can add similar methods for other fields, e.g. average_rate(), etc.
    void print() const;
};

#endif // DOUBLYLINKEDLIST_H


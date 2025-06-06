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

    // Helper to get feature value from a Data object based on StatisticFeature enum
    float getFeatureValue(const Data* data, StatisticFeature feature);

    // DECLARATION ADDED: Helper to collect values for a given interval
    std::vector<float> collectIntervalValues(StatisticFeature feature, int interval_count);


public:
    DoublyLinkedList();
    ~DoublyLinkedList(); // Destructor will only delete Node objects, not Data

    void append(const Data* d); // Takes a const Data pointer
    void insertAt(int index, const Data* d); // Takes a const Data pointer
    const Data* findById(uint32_t id); // Returns const Data pointer
    bool removeById(uint32_t id); // Removes node, does NOT delete Data
    int size() const;

    // Accessor for the head of the list (useful for external iteration if needed)
    const Node* getHead() const { return head; }
    // Accessor for the tail of the list (useful for backward iteration)
    const Node* getTail() const { return tail; }

    // Generic statistical methods that take a StatisticFeature enum and interval_count
    float getAverage(StatisticFeature feature, int interval_count);
    float getStdDev(StatisticFeature feature, int interval_count);
    float getMedian(StatisticFeature feature, int interval_count);
    float getMin(StatisticFeature feature, int interval_count);
    float getMax(StatisticFeature feature, int interval_count);

    void print() const;
    size_t getMemoryUsage() const;
};

#endif // DOUBLYLINKEDLIST_H


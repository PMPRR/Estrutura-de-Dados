#ifndef SEGMENTTREE_H
#define SEGMENTTREE_H

#include <iostream>
#include <vector>
#include <map>
#include <memory>    // For std::unique_ptr
#include <algorithm> // For std::min_element, std::max_element etc.
#include "data.h"    // For Data struct definition

// SegmentTree with methods: insert, remove, find (using id), and getTotalRate
class SegmentTree {
private:
    // Node struct for the Segment Tree
    struct Node {
        int left, right;                // Range covered by this node
        float sumRate;                  // Aggregate sum of rates in this node's range
        std::unique_ptr<Node> leftChild;    // Pointer to left child
        std::unique_ptr<Node> rightChild;   // Pointer to right child
        std::vector<const Data*> values;    // Stores pointers to Data objects if leaf node

        // Constructor for Node
        Node(int l, int r) : left(l), right(r), sumRate(0.0f) {}
    };

    std::unique_ptr<Node> root;             // Root of the Segment Tree
    std::map<uint32_t, int> idToIndex;      // Maps Data ID to its index in the implicit array
    int nextIndex = 0;                      // Next available index for new Data items

    // Private helper for recursive insertion
    void insert(Node* node, int idx, const Data* data); // Takes const Data*

    // Private helper for recursive removal
    bool remove(Node* node, int idx, uint32_t id);

    // Private helper to get sum of rates (handles null nodes)
    float getSum(Node* node) const;

    // Private helper for recursive find
    // Returns const Data* to the object (owned externally).
    const Data* find(Node* node, int idx, uint32_t id); // Returns const Data*

    // Helper to get feature value from a Data object based on StatisticFeature enum
    float getFeatureValue(const Data* data, StatisticFeature feature) const;

    // Helper to recursively collect all data pointers from the tree
    void collectAllDataPointersRecursive(Node* node, std::vector<const Data*>& collected_pointers) const;

public: // Moved collectFeatureValuesForInterval to public
    // Helper to collect feature values for a given interval
    std::vector<float> collectFeatureValuesForInterval(StatisticFeature feature, int interval_count) const;

    // Constructor for SegmentTree
    SegmentTree();

    // Inserts a Data object (via pointer) into the Segment Tree.
    // The SegmentTree stores a pointer to the Data object; it does not own the Data object itself.
    void insert(const Data* data); // Takes const Data*

    // Removes a Data object by its ID from the Segment Tree.
    // Returns true if successfully removed, false otherwise.
    bool remove(uint32_t id);

    // Finds a Data object by its ID in the Segment Tree.
    // Returns a const pointer to the Data object if found, nullptr otherwise.
    // Note: The returned pointer points to external data.
    const Data* find(uint32_t id); // Returns const Data*

    // Gets the total sum of rates across all data in the tree.
    float getTotalRate() const;

    // Generic statistical methods that take a StatisticFeature enum and interval_count
    float getAverage(StatisticFeature feature, int interval_count) const;
    float getStdDev(StatisticFeature feature, int interval_count) const;
    float getMedian(StatisticFeature feature, int interval_count) const;
    float getMin(StatisticFeature feature, int interval_count) const;
    float getMax(StatisticFeature feature, int interval_count) const;
};

#endif // SEGMENTTREE_H


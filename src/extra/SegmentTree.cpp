// Criado pelo Gemini 2.5: 
// PROMPT:  Change the benchmark so it can analize segment tree, make sure to include the memory

#include "extra/SegmentTree.h" // Include the corresponding header file
#include <iostream>            // For debug output (if any)
#include <algorithm>           // For std::find_if, std::min, std::max, std::sort, std::reverse
#include <vector>              // Already included via SegmentTree.h, but good practice to list dependencies
#include <numeric>             // For std::accumulate
#include <cmath>               // For std::sqrt

// Private helper for recursive insertion
void SegmentTree::insert(Node* node, int idx, const Data* data) { // Changed to const Data*
    // Base case: If it's a leaf node (range contains a single index)
    if (node->left == node->right) {
        // Here we store the pointer to the Data object
        node->values.push_back(data);
        // Access rate via pointer
        if (data) node->sumRate += data->rate; // Update sum of rates for this leaf
        return;
    }

    // Determine which child to recurse into
    int mid = (node->left + node->right) / 2;
    if (idx <= mid) {
        // Create left child if it doesn't exist
        if (!node->leftChild)
            node->leftChild = std::make_unique<Node>(node->left, mid);
        insert(node->leftChild.get(), idx, data); // Recurse into left child
    } else {
        // Create right child if it doesn't exist
        if (!node->rightChild)
            node->rightChild = std::make_unique<Node>(mid + 1, node->right);
        insert(node->rightChild.get(), idx, data); // Recurse into right child
    }

    // Update sum of rates for current (non-leaf) node based on its children
    node->sumRate = getSum(node->leftChild.get()) + getSum(node->rightChild.get());
}

// Private helper for recursive removal
bool SegmentTree::remove(Node* node, int idx, uint32_t id) {
    if (!node) return false; // Node does not exist

    // Base case: If it's a leaf node
    if (node->left == node->right) {
        auto& vec = node->values; // Get reference to vector of Data pointers
        // Find the Data pointer by ID
        auto it = std::find_if(vec.begin(), vec.end(), [id](const Data* d) {
            return d && d->id == id; // Check for null pointer before dereferencing
        });
        if (it != vec.end()) {
            if (*it) node->sumRate -= (*it)->rate; // Update sum of rates, access rate via pointer
            vec.erase(it);             // Remove the Data pointer from the vector
            return true;
        }
        return false; // Item not found in this leaf node
    }

    // Determine which child to recurse into for removal
    int mid = (node->left + node->right) / 2;
    bool removed = false;
    if (idx <= mid)
        removed = remove(node->leftChild.get(), idx, id); // Try removing from left child
    else
        removed = remove(node->rightChild.get(), idx, id); // Try removing from right child

    // If an item was removed from a child, update this node's sumRate
    if (removed)
        node->sumRate = getSum(node->leftChild.get()) + getSum(node->rightChild.get());

    return removed;
}

// Private helper to get sum of rates (handles null nodes by returning 0)
float SegmentTree::getSum(Node* node) const {
    return node ? node->sumRate : 0.0f;
}

// Private helper for recursive find
const Data* SegmentTree::find(Node* node, int idx, uint32_t id) { // Changed return type to const Data*
    if (!node) return nullptr; // Node does not exist

    // Base case: If it's a leaf node
    if (node->left == node->right) {
        // Search directly in the leaf's values vector of pointers
        for (const Data* d_ptr : node->values) // Iterate through pointers
            if (d_ptr && d_ptr->id == id) // Check for null pointer before dereferencing
                return d_ptr; // Return the const Data* pointer to the found Data object
        return nullptr; // Item not found in this leaf
    }

    // Determine which child to recurse into for finding
    int mid = (node->left + node->right) / 2;
    if (idx <= mid)
        return find(node->leftChild.get(), idx, id); // Search in left child
    else
        return find(node->rightChild.get(), idx, id); // Search in right child
}

// Helper to get feature value from a Data object based on StatisticFeature enum (copied from LinkedList.cpp)
float SegmentTree::getFeatureValue(const Data* data, StatisticFeature feature) const {
    if (!data) return 0.0f; // Handle null data pointer gracefully for statistics

    switch (feature) {
        case StatisticFeature::DUR: return data->dur;
        case StatisticFeature::RATE: return data->rate;
        case StatisticFeature::SLOAD: return data->sload;
        case StatisticFeature::DLOAD: return data->dload;
        case StatisticFeature::SPKTS: return static_cast<float>(data->spkts);
        case StatisticFeature::DPKTS: return static_cast<float>(data->dpkts);
        case StatisticFeature::SBYTES: return static_cast<float>(data->sbytes);
        case StatisticFeature::DBYTES: return static_cast<float>(data->dbytes);
        default: return 0.0f; // Should not happen if enum and implementation are in sync
    }
}

// Helper to recursively collect all Data* pointers from the tree
void SegmentTree::collectAllDataPointersRecursive(Node* node, std::vector<const Data*>& collected_pointers) const {
    if (!node) return;
    if (node->left == node->right) { // Leaf node
        for (const Data* d_ptr : node->values) {
            collected_pointers.push_back(d_ptr);
        }
        return;
    }
    collectAllDataPointersRecursive(node->leftChild.get(), collected_pointers);
    collectAllDataPointersRecursive(node->rightChild.get(), collected_pointers);
}

// Helper to collect feature values for a given interval
std::vector<float> SegmentTree::collectFeatureValuesForInterval(StatisticFeature feature, int interval_count) const {
    std::vector<const Data*> all_data_pointers;
    // Collect all pointers by traversing the tree
    collectAllDataPointersRecursive(root.get(), all_data_pointers);

    std::vector<float> values;
    if (all_data_pointers.empty()) return values;

    // Determine the actual subset of values to process (last 'interval_count' items)
    size_t start_index = 0;
    if (all_data_pointers.size() > interval_count) {
        start_index = all_data_pointers.size() - interval_count;
    }

    // Populate values with the relevant feature values from the subset
    // Note: The order of insertion into the SegmentTree determines the order of data in leaf nodes.
    // Assuming `nextIndex` increments sequentially for new data, the order of `Data*` in `all_data_pointers`
    // will generally be the order of insertion.
    for (size_t i = start_index; i < all_data_pointers.size(); ++i) {
        if (all_data_pointers[i]) {
            values.push_back(getFeatureValue(all_data_pointers[i], feature));
        }
    }
    return values;
}

// Public constructor for SegmentTree
SegmentTree::SegmentTree() {
    // Initial range for the Segment Tree. Adjust as needed for your data's ID distribution.
    // 0 to 1,000,000 is a large range, assuming IDs fall within it.
    root = std::make_unique<Node>(0, 1000000);
}

// Public insert method
void SegmentTree::insert(const Data* data) { // Changed to const Data*
    if (!data) {
        std::cerr << "Error: Attempted to insert a nullptr Data into SegmentTree." << std::endl;
        return;
    }
    // Assign a unique index to each Data ID for SegmentTree's internal mapping.
    // This assumes new IDs are continuously increasing or that the SegmentTree
    // implicitly covers a range large enough for all potential IDs.
    // This `idToIndex` map is essential because the SegmentTree is built on an index range,
    // not directly on Data IDs.
    int idx = nextIndex++;
    idToIndex[data->id] = idx; // Map Data ID to its allocated index
    insert(root.get(), idx, data); // Call recursive helper
}

// Public remove method
bool SegmentTree::remove(uint32_t id) {
    // Find the internal index corresponding to the Data ID
    auto it = idToIndex.find(id);
    if (it == idToIndex.end()) return false; // ID not found in the tree

    // Call recursive helper to remove
    bool success = remove(root.get(), it->second, id);
    if (success)
        idToIndex.erase(it); // Remove mapping if successfully removed from tree
    return success;
}

// Public find method
const Data* SegmentTree::find(uint32_t id) { // Changed return type to const Data*
    // Find the internal index corresponding to the Data ID
    auto it = idToIndex.find(id);
    if (it == idToIndex.end()) return nullptr; // ID not found in the tree

    // Call recursive helper to find
    return find(root.get(), it->second, id);
}

// Public method to get total rate
float SegmentTree::getTotalRate() const {
    return getSum(root.get());
}

// NEW: Recursive helper function for calculating memory usage
size_t SegmentTree::getMemoryUsageRecursive(const Node* node) const {
    if (!node) {
        return 0;
    }

    // Memory of the current node object itself
    size_t current_node_size = sizeof(Node);

    // Memory consumed by the vector of pointers in the leaf node
    if (!node->values.empty()) {
        current_node_size += node->values.capacity() * sizeof(const Data*);
    }

    // Recursively add memory from children
    size_t left_child_size = getMemoryUsageRecursive(node->leftChild.get());
    size_t right_child_size = getMemoryUsageRecursive(node->rightChild.get());

    return current_node_size + left_child_size + right_child_size;
}

// NEW: Public method to get total memory usage of the tree
size_t SegmentTree::getMemoryUsage() const {
    if (!root) {
        return 0;
    }
    // Start recursion from the root
    return getMemoryUsageRecursive(root.get());
}


// Generic statistical methods that take a StatisticFeature enum and interval_count
float SegmentTree::getAverage(StatisticFeature feature, int interval_count) const {
    std::vector<float> values = collectFeatureValuesForInterval(feature, interval_count);
    if (values.empty()) return 0.0f;
    return std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
}

float SegmentTree::getStdDev(StatisticFeature feature, int interval_count) const {
    std::vector<float> values = collectFeatureValuesForInterval(feature, interval_count);
    if (values.empty()) return 0.0f;
    float avg = getAverage(feature, interval_count); // Use the same average calculated on the interval
    float sum_sq_diff = 0.0f;
    for (float val : values) {
        sum_sq_diff += (val - avg) * (val - avg);
    }
    return (values.size() > 0) ? std::sqrt(sum_sq_diff / values.size()) : 0.0f;
}

float SegmentTree::getMedian(StatisticFeature feature, int interval_count) const {
    std::vector<float> values = collectFeatureValuesForInterval(feature, interval_count);
    if (values.empty()) return 0.0f;
    std::sort(values.begin(), values.end());
    int mid = values.size() / 2;
    return values.size() % 2 == 0 ? (values[mid - 1] + values[mid]) / 2.0f : values[mid];
}

float SegmentTree::getMin(StatisticFeature feature, int interval_count) const {
    std::vector<float> values = collectFeatureValuesForInterval(feature, interval_count);
    if (values.empty()) return 0.0f; // Or std::numeric_limits<float>::max();
    return *std::min_element(values.begin(), values.end());
}

float SegmentTree::getMax(StatisticFeature feature, int interval_count) const {
    std::vector<float> values = collectFeatureValuesForInterval(feature, interval_count);
    if (values.empty()) return 0.0f; // Or std::numeric_limits<float>::lowest();
    return *std::max_element(values.begin(), values.end());
}

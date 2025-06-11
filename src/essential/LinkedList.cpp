// Criado por Gemini AI 2.5 pro, baseado no código de Fernando Nishio

#include "essential/LinkedList.h"
#include <cmath>
#include <algorithm>
#include <iostream> // For print and potential debug output
#include <vector>   // For median, and temporary storage for interval calculations
#include <numeric>  // For std::accumulate

DoublyLinkedList::Node::Node(const Data* d) : data(d), prev(nullptr), next(nullptr) {}

DoublyLinkedList::DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

size_t DoublyLinkedList::getMemoryUsage() const {
    // Memory for each node is a const Data* pointer + 2 node pointers (prev, next)
    return size() * (sizeof(Node)); 
}

DoublyLinkedList::~DoublyLinkedList() {
    Node* current = head;
    while (current) {
        Node* next = current->next;
        // IMPORTANT: Do NOT delete current->data;
        // The Data objects are owned by master_data_store (via unique_ptr) in main.cpp.
        // This destructor should only deallocate the Node objects themselves.
        delete current;
        current = next;
    }
    head = nullptr;
    tail = nullptr;
    count = 0;
}

void DoublyLinkedList::append(const Data* d) {
    Node* newNode = new Node(d);
    if (!head) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        newNode->prev = tail;
        tail = newNode;
    }
    count++;
}

void DoublyLinkedList::insertAt(int index, const Data* d) {
    if (index < 0 || index > count) {
        std::cerr << "Error: Index out of bounds for insertAt(" << index << ")." << std::endl;
        return;
    }

    Node* newNode = new Node(d);
    if (index == 0) {
        newNode->next = head;
        if (head) head->prev = newNode;
        head = newNode;
        if (!tail) tail = newNode; // If list was empty, head becomes tail
    } else if (index == count) {
        append(d); // Uses the existing append logic
        return; // append already increments count
    } else {
        Node* current = head;
        // Traverse to the node *before* the insertion point
        for (int i = 0; i < index; ++i)
            current = current->next;

        // Insert newNode before 'current'
        newNode->prev = current->prev;
        newNode->next = current;
        current->prev->next = newNode;
        current->prev = newNode;
    }
    count++;
}

const Data* DoublyLinkedList::findById(uint32_t id) {
    Node* current = head;
    while (current) {
        if (current->data && current->data->id == id) // Check for null data pointer too
            return current->data;
        current = current->next;
    }
    return nullptr;
}

bool DoublyLinkedList::removeById(uint32_t id) {
    Node* current = head;
    while (current) {
        if (current->data && current->data->id == id) { // Check for null data pointer too
            if (current == head) {
                head = current->next;
                if (head) head->prev = nullptr;
                else tail = nullptr; // List became empty
            } else if (current == tail) {
                tail = current->prev;
                tail->next = nullptr;
            } else {
                current->prev->next = current->next;
                current->next->prev = current->prev;
            }
            // IMPORTANT: Do NOT delete current->data;
            // The Data object is owned by master_data_store (via unique_ptr) in main.cpp.
            // This function should only deallocate the Node object.
            delete current;
            count--;
            return true;
        }
        current = current->next;
    }
    return false; // Not found
}

int DoublyLinkedList::size() const {
    return count;
}

// Helper to get feature value from a Data object based on StatisticFeature enum
float DoublyLinkedList::getFeatureValue(const Data* data, StatisticFeature feature) {
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
        // Add cases for other features as they are included in StatisticFeature enum
        default: return 0.0f; // Should not happen if enum and implementation are in sync
    }
}

// Helper to collect values for a given interval
std::vector<float> DoublyLinkedList::collectIntervalValues(StatisticFeature feature, int interval_count) {
    std::vector<float> values;
    if (count == 0) return values;

    int actual_count = std::min(count, interval_count);
    values.reserve(actual_count);

    Node* current = tail; // Start from the tail to get the most recent items
    for (int i = 0; i < actual_count; ++i) {
        if (current && current->data) {
            values.push_back(getFeatureValue(current->data, feature));
        }
        if (current) current = current->prev;
    }
    // The values are collected in reverse order (most recent first).
    // For statistics like average, min/max, it doesn't matter.
    // For median, sorting will handle it.
    return values;
}


// Generic statistical methods that take a StatisticFeature enum and interval_count
float DoublyLinkedList::getAverage(StatisticFeature feature, int interval_count) {
    std::vector<float> values = collectIntervalValues(feature, interval_count);
    if (values.empty()) return 0.0f;
    return std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
}

float DoublyLinkedList::getStdDev(StatisticFeature feature, int interval_count) {
    std::vector<float> values = collectIntervalValues(feature, interval_count);
    if (values.empty()) return 0.0f;
    float avg = getAverage(feature, interval_count); // Use the same average calculated on the interval
    float sum_sq_diff = 0.0f;
    for (float val : values) {
        sum_sq_diff += (val - avg) * (val - avg);
    }
    return (values.size() > 0) ? std::sqrt(sum_sq_diff / values.size()) : 0.0f;
}

float DoublyLinkedList::getMedian(StatisticFeature feature, int interval_count) {
    std::vector<float> values = collectIntervalValues(feature, interval_count);
    if (values.empty()) return 0.0f;
    std::sort(values.begin(), values.end());
    int mid = values.size() / 2;
    return values.size() % 2 == 0 ? (values[mid - 1] + values[mid]) / 2.0f : values[mid];
}

float DoublyLinkedList::getMin(StatisticFeature feature, int interval_count) {
    std::vector<float> values = collectIntervalValues(feature, interval_count);
    if (values.empty()) return 0.0f; // Or std::numeric_limits<float>::max();
    return *std::min_element(values.begin(), values.end());
}

float DoublyLinkedList::getMax(StatisticFeature feature, int interval_count) {
    std::vector<float> values = collectIntervalValues(feature, interval_count);
    if (values.empty()) return 0.0f; // Or std::numeric_limits<float>::lowest();
    return *std::max_element(values.begin(), values.end());
}


void DoublyLinkedList::print() const {
    Node* current = head;
    int idx = 0;
    std::cout << "DoublyLinkedList contents (" << count << " elements):\n";
    while (current) {
        std::cout << "[" << idx << "] ";
        if (current->data) {
            std::cout << "id=" << current->data->id << ", dur=" << current->data->dur;
        } else {
            std::cout << "(null data)";
        }
        std::cout << std::endl;
        current = current->next;
        ++idx;
    }
}


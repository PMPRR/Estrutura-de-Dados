#include "essential/LinkedList.h"
#include <cmath>
#include <algorithm>
#include <iostream> // For print and potential debug output

DoublyLinkedList::Node::Node(const Data* d) : data(d), prev(nullptr), next(nullptr) {}

DoublyLinkedList::DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

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

// ---- Statistics for Data::dur ----

float DoublyLinkedList::average_dur() {
    if (count == 0) return 0.0f;
    float sum = 0.0f;
    Node* current = head;
    while (current) {
        if (current->data) sum += current->data->dur;
        current = current->next;
    }
    return sum / count;
}

float DoublyLinkedList::stddev_dur() {
    if (count == 0) return 0.0f;
    float avg = average_dur();
    float sum = 0.0f;
    Node* current = head;
    while (current) {
        if (current->data) {
            float val = current->data->dur;
            sum += (val - avg) * (val - avg);
        }
        current = current->next;
    }
    return std::sqrt(sum / count);
}

float DoublyLinkedList::median_dur() {
    if (count == 0) return 0.0f;
    std::vector<float> values;
    values.reserve(count); // Pre-allocate memory for efficiency
    Node* current = head;
    while (current) {
        if (current->data) values.push_back(current->data->dur);
        current = current->next;
    }
    if (values.empty()) return 0.0f; // In case all data pointers were null
    std::sort(values.begin(), values.end());
    int mid = values.size() / 2;
    return values.size() % 2 == 0 ? (values[mid - 1] + values[mid]) / 2.0f : values[mid];
}

float DoublyLinkedList::min_dur() {
    if (!head || !head->data) return 0.0f; // Handle empty list or null head data
    float minVal = head->data->dur;
    Node* current = head->next;
    while (current) {
        if (current->data) minVal = std::min(minVal, current->data->dur);
        current = current->next;
    }
    return minVal;
}

float DoublyLinkedList::max_dur() {
    if (!head || !head->data) return 0.0f; // Handle empty list or null head data
    float maxVal = head->data->dur;
    Node* current = head->next;
    while (current) {
        if (current->data) maxVal = std::max(maxVal, current->data->dur);
        current = current->next;
    }
    return maxVal;
}

void DoublyLinkedList::histogram_dur(int bins) {
    if (count == 0) {
        std::cout << "  No data to generate histogram.\n";
        return;
    }
    float minVal = min_dur();
    float maxVal = max_dur();
    float range = maxVal - minVal;
    if (range == 0) {
        std::cout << "  Todos os valores são iguais: " << minVal << "\n";
        return;
    }

    std::vector<int> binCounts(bins, 0);
    Node* current = head;
    while (current) {
        if (current->data) {
            float val = current->data->dur;
            int bin = std::min(int((val - minVal) / range * bins), bins - 1);
            binCounts[bin]++;
        }
        current = current->next;
    }

    std::cout << "  Histogram for 'dur' (" << bins << " bins):\n";
    for (int i = 0; i < bins; ++i) {
        float low = minVal + i * (range / bins);
        float high = minVal + (i + 1) * (range / bins);
        std::cout << "    [" << low << ", " << high << "): " << binCounts[i] << "\n";
    }
}

void DoublyLinkedList::print() const {
    Node* current = head;
    int idx = 0;
    std::cout << "DoublyLinkedList contents (" << count << " elements):\n";
    while (current) {
        std::cout << "[" << idx << "] ";
        if (current->data) {
            std::cout << "id=" << current->data->id << ", dur=" << current->data->dur;
            // You can add more fields as needed, e.g.:
            // std::cout << ", rate=" << current->data->rate;
        } else {
            std::cout << "(null data)";
        }
        std::cout << std::endl;
        current = current->next;
        ++idx;
    }
}


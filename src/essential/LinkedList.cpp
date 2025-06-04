#include "data.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>

struct Node {
    Data* data;
    Node* prev;
    Node* next;
    Node(Data* d) : data(d), prev(nullptr), next(nullptr) {}
};

class DoublyLinkedList {
private:
    Node* head;
    Node* tail;
    int count;

public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

    ~DoublyLinkedList() {
        Node* current = head;
        while (current) {
            Node* next = current->next;
            delete current->data;
            delete current;
            current = next;
        }
    }

    void append(Data* d) {
        Node* newNode = new Node(d);
        if (!head) head = tail = newNode;
        else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        count++;
    }

    void insertAt(int index, Data* d) {
        if (index < 0 || index > count) return;

        Node* newNode = new Node(d);
        if (index == 0) {
            newNode->next = head;
            if (head) head->prev = newNode;
            head = newNode;
            if (!tail) tail = newNode;
        } else if (index == count) {
            append(d);
            return;
        } else {
            Node* current = head;
            for (int i = 0; i < index; ++i)
                current = current->next;

            newNode->prev = current->prev;
            newNode->next = current;
            current->prev->next = newNode;
            current->prev = newNode;
        }
        count++;
    }

    Data* findById(uint32_t id) {
        Node* current = head;
        while (current) {
            if (current->data->id == id)
                return current->data;
            current = current->next;
        }
        return nullptr;
    }

    bool removeById(uint32_t id) {
        Node* current = head;
        while (current) {
            if (current->data->id == id) {
                if (current == head) {
                    head = current->next;
                    if (head) head->prev = nullptr;
                    else tail = nullptr;  // Lista ficou vazia
                } else if (current == tail) {
                    tail = current->prev;
                    tail->next = nullptr;
                } else {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }
                delete current->data;
                delete current;
                count--;
                return true;
            }
            current = current->next;
        }
        return false; // Não encontrado
    }

    int size() const {
        return count;
    }

    float average(std::function<float(Data*)> accessor) {
        if (count == 0) return 0.0;
        float sum = 0.0;
        Node* current = head;
        while (current) {
            sum += accessor(current->data);
            current = current->next;
        }
        return sum / count;
    }

    float stddev(std::function<float(Data*)> accessor) {
        if (count == 0) return 0.0;
        float avg = average(accessor);
        float sum = 0.0;
        Node* current = head;
        while (current) {
            float val = accessor(current->data);
            sum += (val - avg) * (val - avg);
            current = current->next;
        }
        return std::sqrt(sum / count);
    }

    float median(std::function<float(Data*)> accessor) {
        if (count == 0) return 0.0;
        std::vector<float> values;
        Node* current = head;
        while (current) {
            values.push_back(accessor(current->data));
            current = current->next;
        }
        std::sort(values.begin(), values.end());
        int mid = values.size() / 2;
        return values.size() % 2 == 0 ? (values[mid - 1] + values[mid]) / 2.0 : values[mid];
    }

    float min(std::function<float(Data*)> accessor) {
        if (!head) return 0.0;
        float minVal = accessor(head->data);
        Node* current = head->next;
        while (current) {
            minVal = std::min(minVal, accessor(current->data));
            current = current->next;
        }
        return minVal;
    }

    float max(std::function<float(Data*)> accessor) {
        if (!head) return 0.0;
        float maxVal = accessor(head->data);
        Node* current = head->next;
        while (current) {
            maxVal = std::max(maxVal, accessor(current->data));
            current = current->next;
        }
        return maxVal;
    }

    void histogram(std::function<float(Data*)> accessor, int bins) {
        if (count == 0) return;
        float minVal = min(accessor);
        float maxVal = max(accessor);
        float range = maxVal - minVal;
        if (range == 0) {
            std::cout << "  Todos os valores são iguais: " << minVal << "\n";
            return;
        }

        std::vector<int> binCounts(bins, 0);
        Node* current = head;
        while (current) {
            float val = accessor(current->data);
            int bin = std::min(int((val - minVal) / range * bins), bins - 1);
            binCounts[bin]++;
            current = current->next;
        }

        for (int i = 0; i < bins; ++i) {
            float low = minVal + i * (range / bins);
            float high = minVal + (i + 1) * (range / bins);
            std::cout << "  [" << low << ", " << high << "): " << binCounts[i] << "\n";
        }
    }
};

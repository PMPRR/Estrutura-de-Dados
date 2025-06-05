#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include "data.h"
// SegmentTree com métodos: insert, remove, find (usando id), e getTotalRate
class SegmentTree {
private:
    struct Node {
        int left, right;
        float sumRate;
        std::unique_ptr<Node> leftChild, rightChild;
        std::vector<Data> values;

        Node(int l, int r) : left(l), right(r), sumRate(0.0f) {}
    };

    std::unique_ptr<Node> root;
    std::map<uint32_t, int> idToIndex;
    int nextIndex = 0;

    void insert(Node* node, int idx, const Data& data) {
        if (node->left == node->right) {
            node->values.push_back(data);
            node->sumRate += data.rate;
            return;
        }

        int mid = (node->left + node->right) / 2;
        if (idx <= mid) {
            if (!node->leftChild)
                node->leftChild = std::make_unique<Node>(node->left, mid);
            insert(node->leftChild.get(), idx, data);
        } else {
            if (!node->rightChild)
                node->rightChild = std::make_unique<Node>(mid + 1, node->right);
            insert(node->rightChild.get(), idx, data);
        }

        node->sumRate = getSum(node->leftChild.get()) + getSum(node->rightChild.get());
    }

    bool remove(Node* node, int idx, uint32_t id) {
        if (!node) return false;

        if (node->left == node->right) {
            auto& vec = node->values;
            auto it = std::find_if(vec.begin(), vec.end(), [id](const Data& d) { return d.id == id; });
            if (it != vec.end()) {
                node->sumRate -= it->rate;
                vec.erase(it);
                return true;
            }
            return false;
        }

        int mid = (node->left + node->right) / 2;
        bool removed = false;
        if (idx <= mid)
            removed = remove(node->leftChild.get(), idx, id);
        else
            removed = remove(node->rightChild.get(), idx, id);

        if (removed)
            node->sumRate = getSum(node->leftChild.get()) + getSum(node->rightChild.get());

        return removed;
    }

    float getSum(Node* node) const {
        return node ? node->sumRate : 0.0f;
    }

    Data* find(Node* node, int idx, uint32_t id) {
        if (!node) return nullptr;

        if (node->left == node->right) {
            for (auto& d : node->values)
                if (d.id == id)
                    return &d;
            return nullptr;
        }

        int mid = (node->left + node->right) / 2;
        if (idx <= mid)
            return find(node->leftChild.get(), idx, id);
        else
            return find(node->rightChild.get(), idx, id);
    }

public:
    SegmentTree() {
        root = std::make_unique<Node>(0, 1000000); // intervalo grande o suficiente
    }

    void insert(const Data& data) {
        int idx = nextIndex++;
        idToIndex[data.id] = idx;
        insert(root.get(), idx, data);
    }

    bool remove(uint32_t id) {
        auto it = idToIndex.find(id);
        if (it == idToIndex.end()) return false;

        bool success = remove(root.get(), it->second, id);
        if (success)
            idToIndex.erase(it);
        return success;
    }

    Data* find(uint32_t id) {
        auto it = idToIndex.find(id);
        if (it == idToIndex.end()) return nullptr;

        return find(root.get(), it->second, id);
    }

    float getTotalRate() const {
        return getSum(root.get());
    }
};

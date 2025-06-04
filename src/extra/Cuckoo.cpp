#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

const int MAX_REHASHES = 10;
class CuckooHash {
private:
    vector<int> table1, table2;
    int size;
    int numElements;

    int hash1(int key) {
        return key % size;
    }

    int hash2(int key) {
        return (key / size) % size;
    }

    void rehash() {
        cout << "Fazendo rehash ,tamanho limite atingido, expandindo tabela\n";
        size *= 2;
        vector<int> old1 = table1;
        vector<int> old2 = table2;

        table1.assign(size, -1);
        table2.assign(size, -1);
        numElements = 0;

        for (int key : old1) {
            if (key != -1)
                insert(key);
        }
        for (int key : old2) {
            if (key != -1)
                insert(key);
        }
    }

public:
    CuckooHash(int initialSize = 11) {
        size = initialSize;
        table1.assign(size, -1);
        table2.assign(size, -1);
        numElements = 0;
        srand(time(0));
    }

    bool insert(int key) {
        if (search(key)) return false;

        int loopCount = 0;
        int pos1 = hash1(key);
        for (int i = 0; i < MAX_REHASHES; ++i) {
            if (table1[pos1] == -1) {
                table1[pos1] = key;
                numElements++;
                return true;
            }
            swap(key, table1[pos1]);
            int pos2 = hash2(key);
            if (table2[pos2] == -1) {
                table2[pos2] = key;
                numElements++;
                return true;
            }
            swap(key, table2[pos2]);
            pos1 = hash1(key);
        }

        rehash();
        return insert(key); // tenta de novo após rehash
    }

    bool search(int key) {
        int pos1 = hash1(key);
        int pos2 = hash2(key);
        return table1[pos1] == key || table2[pos2] == key;
    }

    bool remove(int key) {
        int pos1 = hash1(key);
        if (table1[pos1] == key) {
            table1[pos1] = -1;
            numElements--;
            return true;
        }

        int pos2 = hash2(key);
        if (table2[pos2] == key) {
            table2[pos2] = -1;
            numElements--;
            return true;
        }

        return false;
    }

    void display() {
        cout << "Table 1: ";
        for (int i = 0; i < size; i++) {
            if (table1[i] != -1)
                cout << table1[i] << " ";
            else
                cout << "_ ";
        }
        cout << "\nTable 2: ";
        for (int i = 0; i < size; i++) {
            if (table2[i] != -1)
                cout << table2[i] << " ";
            else
                cout << "_ ";
        }
        cout << "\n";
    }
};

#include "essential/HashTable.h" // Corrected include path
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

void testManyElements() {
    std::cout << "--- Test: Many Elements (HashTable) ---\n";
    const int N = 1000;
    HashTable ht(101); // Use HashTable directly, no namespace

    for (int i = 0; i < N; ++i) {
        ht.insert(new Data(i + 1, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, (float)i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP)); // Simplified Data creation for test
    }
    assert(ht.size() == N);
    std::cout << "Inserted " << N << " elements. Size is correct.\n";

    for (int i = 0; i < N; ++i) {
        const Data* found = ht.find(i + 1);
        assert(found != nullptr);
        assert(found->id == (uint32_t)(i + 1));
    }
    std::cout << "All elements found successfully.\n";

    for (int i = 0; i < N / 2; ++i) {
        bool removed = ht.remove(i + 1);
        assert(removed);
    }
    assert(ht.size() == N / 2);
    std::cout << "Removed " << N / 2 << " elements. Size is correct.\n";

    for (int i = 0; i < N / 2; ++i) {
        const Data* found = ht.find(i + 1);
        assert(found == nullptr); // Should not be found
    }
    std::cout << "First half of elements confirmed removed.\n";

    for (int i = N / 2; i < N; ++i) {
        const Data* found = ht.find(i + 1);
        assert(found != nullptr); // Should still be found
    }
    std::cout << "Second half of elements confirmed present.\n";

    ht.clear();
    assert(ht.size() == 0);
    std::cout << "Cleared hash table. Size is correct.\n";
    std::cout << "--- Test: Many Elements PASSED ---\n\n";
}

void testBasic() {
    std::cout << "--- Test: Basic HashTable Operations ---\n";
    HashTable ht(13); // Use HashTable directly, no namespace

    ht.insert(new Data(10, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
    ht.insert(new Data(20, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
    
    assert(ht.size() == 2);
    std::cout << "Inserted 2 elements. Size is correct.\n";

    const Data* found10 = ht.find(10);
    assert(found10 != nullptr && found10->id == 10);
    std::cout << "Found ID 10.\n";

    const Data* found20 = ht.find(20);
    assert(found20 != nullptr && found20->id == 20);
    std::cout << "Found ID 20.\n";

    bool removed = ht.remove(10);
    assert(removed);
    assert(ht.size() == 1);
    std::cout << "Removed ID 10. Size is correct.\n";

    const Data* notFound10 = ht.find(10);
    assert(notFound10 == nullptr);
    std::cout << "Confirmed ID 10 not found.\n";

    const Data* stillFound20 = ht.find(20);
    assert(stillFound20 != nullptr && stillFound20->id == 20);
    std::cout << "Confirmed ID 20 still found.\n";

    ht.clear();
    assert(ht.size() == 0);
    std::cout << "Cleared hash table. Size is correct.\n";
    std::cout << "--- Test: Basic HashTable Operations PASSED ---\n\n";
}

void testCollisions() {
    std::cout << "--- Test: HashTable Collisions (simple modulo hash) ---\n";
    HashTable ht(5); // Small table size to force collisions
    // IDs that will likely collide with modulo 5: 1, 6, 11, ... or 2, 7, 12 ...
    ht.insert(new Data(1, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
    ht.insert(new Data(6, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
    ht.insert(new Data(11, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));
    ht.insert(new Data(2, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP));

    assert(ht.size() == 4);
    std::cout << "Inserted 4 elements with potential collisions. Size is correct.\n";

    assert(ht.find(1) != nullptr);
    assert(ht.find(6) != nullptr);
    assert(ht.find(11) != nullptr);
    assert(ht.find(2) != nullptr);
    std::cout << "All elements retrieved successfully.\n";

    assert(ht.remove(6));
    assert(ht.size() == 3);
    assert(ht.find(6) == nullptr);
    std::cout << "Removed 6. Size and find correct.\n";

    std::cout << "--- Test: HashTable Collisions PASSED ---\n\n";
}

int main() {
    std::cout << "Running HashTable tests...\n\n";
    testBasic();
    testManyElements();
    testCollisions();
    std::cout << "All HashTable tests passed!\n";
    return 0;
}


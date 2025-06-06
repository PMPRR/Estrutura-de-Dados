#include "essential/RBTree.h" // Include the header for the RBTree
#include <iostream>
#include <vector>
#include <cassert>
#include <memory> // For std::unique_ptr

// A simple utility to create sample data for testing purposes.
// This is similar to the helpers in your other test files.
std::vector<std::unique_ptr<Data>> create_sample_data_for_rbtree() {
    std::vector<std::unique_ptr<Data>> samples;

    // Use a loop to create a variety of data points.
    // We'll use unique_ptr to manage memory automatically.
    for (int i = 1; i <= 15; ++i) {
        samples.push_back(std::make_unique<Data>(
            i,                          // id
            1.0f * i,                   // dur
            10.0f * i,                  // rate
            100.0f, 0.0f, 0.1f, 0.1f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            (uint16_t)i, (uint16_t)i, (uint32_t)i*10, (uint32_t)i*10,
            64, 64, 0, 0, 
            1, 1, 1, 1,
            (uint16_t)i*2, (uint16_t)i*2, 1, 100,
            1, 1, 1, 1, 1, 0, 0, 1, 1,
            false, false, (i % 3 == 0), // Label some as attacks
            Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP
        ));
    }
    return samples;
}


int main() {
    std::cout << "--- Running Red-Black Tree Tests ---" << std::endl;

    // --- 1. Setup ---
    std::cout << "\n[Step 1] Creating RBTree and sample data..." << std::endl;
    RBTree rb_tree;
    auto test_data = create_sample_data_for_rbtree();
    assert(rb_tree.empty() == true);
    assert(rb_tree.size() == 0);
    std::cout << "Initial tree is empty. Size: 0. OK." << std::endl;

    // --- 2. Insertion Test ---
    std::cout << "\n[Step 2] Inserting " << test_data.size() << " elements..." << std::endl;
    for (const auto& data_ptr : test_data) {
        rb_tree.insert(data_ptr.get());
    }
    assert(rb_tree.size() == test_data.size());
    std::cout << "Insertion complete. Tree size: " << rb_tree.size() << ". OK." << std::endl;
    
    // --- 3. Property Verification After Insertion ---
    std::cout << "\n[Step 3] Verifying Red-Black Tree properties after insertion..." << std::endl;
    bool properties_ok_after_insert = rb_tree.verifyProperties();
    assert(properties_ok_after_insert == true);
    std::cout << "All Red-Black Tree properties are valid. OK." << std::endl;
    // rb_tree.printTree(); // Uncomment for visual debugging of the tree structure

    // --- 4. Find Test ---
    std::cout << "\n[Step 4] Finding existing and non-existing elements..." << std::endl;
    // Test finding an element that should exist (e.g., ID 5)
    const Data* found_data_5 = rb_tree.find(5);
    assert(found_data_5 != nullptr);
    assert(found_data_5->id == 5);
    std::cout << "Found element with ID 5. OK." << std::endl;

    // Test finding an element that should not exist (e.g., ID 99)
    const Data* not_found_data = rb_tree.find(99);
    assert(not_found_data == nullptr);
    std::cout << "Did not find element with non-existent ID 99. OK." << std::endl;

    // --- 5. Removal Test ---
    std::cout << "\n[Step 5] Removing elements..." << std::endl;
    // Remove an existing element (e.g., ID 7)
    bool removed_7 = rb_tree.remove(7);
    assert(removed_7 == true);
    assert(rb_tree.size() == test_data.size() - 1);
    assert(rb_tree.find(7) == nullptr); // Verify it's gone
    std::cout << "Successfully removed element with ID 7. Tree size: " << rb_tree.size() << ". OK." << std::endl;

    // Attempt to remove a non-existent element
    bool removed_99 = rb_tree.remove(99);
    assert(removed_99 == false);
    assert(rb_tree.size() == test_data.size() - 1);
    std::cout << "Attempt to remove non-existent ID 99 failed as expected. OK." << std::endl;

    // Remove the root (ID 8 might be the root after insertions) or another node to test rebalancing
    // Let's remove ID 4, which could be in various positions
    rb_tree.remove(4);
    assert(rb_tree.find(4) == nullptr);
    std::cout << "Successfully removed element with ID 4. Tree size: " << rb_tree.size() << ". OK." << std::endl;


    // --- 6. Property Verification After Removal ---
    std::cout << "\n[Step 6] Verifying Red-Black Tree properties after removal..." << std::endl;
    bool properties_ok_after_remove = rb_tree.verifyProperties();
    assert(properties_ok_after_remove == true);
    std::cout << "All Red-Black Tree properties are still valid. OK." << std::endl;
    // rb_tree.printTree(); // Uncomment for visual debugging

    // --- 7. Clear Test ---
    std::cout << "\n[Step 7] Clearing the tree..." << std::endl;
    rb_tree.clear();
    assert(rb_tree.empty() == true);
    assert(rb_tree.size() == 0);
    std::cout << "Tree cleared successfully. Size is 0. OK." << std::endl;

    std::cout << "\n--- All Red-Black Tree Tests Passed! ---" << std::endl;

    return 0;
}


#include "essential/AVL.h"
#include "data.h"
#include <algorithm>
#include <iostream>

void AVL::insert(const Data *data) { 
    _root = insertUtil(data, _root); 
}

AVL::Node_AVL* AVL::insertUtil(const Data* data, Node_AVL* node) {
    // 1. Perform standard BST insertion
    if (node == nullptr) {
        return new Node_AVL(data); // Create new node if tree/subtree is empty
    }

    if (data->id < node->data->id) {
        node->left = insertUtil(data, node->left);
    } else if (data->id > node->data->id) {
        node->right = insertUtil(data, node->right);
    } else {
        // Duplicate keys are not inserted.
        // Depending on requirements, you might update the existing node's data
        // or throw an exception. Here, we simply return the node.
        return node;
    }

    // 2. Update height of this ancestor node
    node->height = 1 + std::max(height(node->left), height(node->right));

    // 3. Get the balance factor of this ancestor node to check if it became unbalanced
    int16_t balance = getBalance(node);

    // 4. If the node becomes unbalanced, then there are 4 cases:
    // Using the alternative balancing logic based on child's balance factor.

    // Left Heavy
    if (balance > 1) { // Current node is left-heavy
        // If the left child is left-heavy or balanced (BF >= 0), it's an LL Case.
        // If the left child is right-heavy (BF < 0), it's an LR Case.
        if (getBalance(node->left) >= 0) { // LL Case (or L- case if balance is 0, still effectively LL for rotation)
            return rightRotation(node);
        } else { // LR Case (node->left is right-heavy)
            node->left = leftRotation(node->left); // First, left-rotate the left child
            return rightRotation(node);           // Then, right-rotate the current node
        }
    }

    // Right Heavy
    if (balance < -1) { // Current node is right-heavy
        // If the right child is right-heavy or balanced (BF <= 0), it's an RR Case.
        // If the right child is left-heavy (BF > 0), it's an RL Case.
        if (getBalance(node->right) <= 0) { // RR Case (or R- case if balance is 0, still effectively RR for rotation)
            return leftRotation(node);
        } else { // RL Case (node->right is left-heavy)
            node->right = rightRotation(node->right); // First, right-rotate the right child
            return leftRotation(node);               // Then, left-rotate the current node
        }
    }

    /*
    // Original balancing logic (now commented out):
    // Left Left Case (LL)
    if (balance > 1 && data->id < node->left->data->id) {
        // performs a right rotation on node (which is y in the rotation diagrams)
        return rightRotation(node);
    }

    // Right Right Case (RR)
    if (balance < -1 && data->id > node->right->data->id) {
        // performs a left rotation on node (which is x in the rotation diagrams)
        return leftRotation(node);
    }

    // Left Right Case (LR)
    if (balance > 1 && data->id > node->left->data->id) {
        node->left = leftRotation(node->left); // Left rotate the left child
        return rightRotation(node);           // Then right rotate the current node
    }

    // Right Left Case (RL)
    if (balance < -1 && data->id < node->right->data->id) {
        node->right = rightRotation(node->right); // Right rotate the right child
        return leftRotation(node);               // Then left rotate the current node
    }
    */

    // Return the (possibly updated) node pointer
    return node;
}

int16_t AVL::getBalance(Node_AVL *node) {
  if (node == nullptr)
    return 0;
  return height(node->left) - height(node->right);
}

int16_t AVL::height(Node_AVL *node) {
  if (node == nullptr)
    return 0;
  return node->height;
}

AVL::Node_AVL* AVL::rightRotation(Node_AVL* y) {
    // y is the root of the subtree to be rotated
    Node_AVL* x = y->left;    // x becomes the new root
    Node_AVL* T2 = x->right;  // T2 is the right child of x (or nullptr)

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights (order matters: children first, then parent)
    // Height of y (which is now a child of x)
    y->height = 1 + std::max(height(y->left), height(y->right));
    // Height of x (the new root of this subtree)
    x->height = 1 + std::max(height(x->left), height(x->right));

    // Return the new root of the rotated subtree
    return x;
}

// Private utility function for left rotation
AVL::Node_AVL* AVL::leftRotation(Node_AVL* x) {
    // x is the root of the subtree to be rotated
    Node_AVL* y = x->right;   // y becomes the new root
    Node_AVL* T2 = y->left;   // T2 is the left child of y (or nullptr)

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update heights (order matters: children first, then parent)
    // Height of x (which is now a child of y)
    x->height = 1 + std::max(height(x->left), height(x->right));
    // Height of y (the new root of this subtree)
    y->height = 1 + std::max(height(y->left), height(y->right));

    // Return the new root of the rotated subtree
    return y;
}

void AVL::printAsciiRecursive(Node_AVL *node, int currentIndentLevel,
                              int indentUnit) {
  if (node == nullptr) {
    return;
  }

  // Process the right child first (it will appear on top)
  printAsciiRecursive(node->right, currentIndentLevel + 1, indentUnit);

  // Print the current node after appropriate spacing
  for (int i = 0; i < currentIndentLevel * indentUnit; i++) {
    std::cout << " ";
  }

  if (node->data != nullptr) {
    std::cout << node->data->id << "(H:" << node->height
              << ",BF:" << getBalance(node) << ")";
  } else {
    std::cout << "[null_data_ptr]";
  }
  std::cout << std::endl;

  // Process the left child (it will appear on the bottom)
  printAsciiRecursive(node->left, currentIndentLevel + 1, indentUnit);
}

void AVL::printAsciiTree(int indentUnit) {
  if (_root == nullptr) {
    std::cout << "<empty tree>" << std::endl;
    return;
  }
  std::cout << "ASCII Art Tree (rotated 90 degrees counter-clockwise):"
            << std::endl;
  printAsciiRecursive(_root, 0, indentUnit);
}

void AVL::printPreOrderHierarchical(int indentUnit) {
        if (_root == nullptr) {
            std::cout << "<empty tree for Pre-Order Hierarchical print>" << std::endl;
            return;
        }
        std::cout << "Pre-Order Hierarchical Print:" << std::endl;
        printPreOrderHierarchicalRecursive(_root, 0, indentUnit);
}

void AVL::printPreOrderHierarchicalRecursive(Node_AVL* node, int depth, int indentUnit) {
        if (node == nullptr) {
            return;
        }
        // Print indentation for the current depth
        for (int i = 0; i < depth * indentUnit; ++i) {
            std::cout << " ";
        }
        // Print the current node's data
        if (node->data != nullptr) {
            std::cout << node->data->id
                      << "(H:" << node->height
                      << ",BF:" << getBalance(node) << ")" << std::endl;
        } else {
            std::cout << "[null_data_ptr]" << std::endl; // Should not happen in a valid tree with valid pointers
        }

        // Recursively print the left child
        printPreOrderHierarchicalRecursive(node->left, depth + 1, indentUnit);
        // Recursively print the right child
        printPreOrderHierarchicalRecursive(node->right, depth + 1, indentUnit);
}



AVL::Node_AVL* AVL::queryById(uint32_t id){
    Node_AVL* temp = _root;
    while(temp != nullptr && temp->data->id != id){
        if(temp->data->id > id){
            temp = temp->left;
        }
        else{
            temp = temp->right;
        }
    }

    return temp; // Case ID not found
}

void AVL::removeById(uint32_t id){
     _root = removeUtil(_root, id);
}

AVL::Node_AVL* AVL::removeUtil(Node_AVL* node, uint32_t id) {
    // 1. Standard BST delete

    // Base case: If the tree (or subtree) is empty, key not found
    if (node == nullptr) {
        return nullptr;
    }

    // If the ID to be deleted is smaller than the node's ID, then it lies in left subtree
    if (id < node->data->id) {
        node->left = removeUtil(node->left, id);
    }
    // If the ID to be deleted is greater than the node's ID, then it lies in right subtree
    else if (id > node->data->id) {
        node->right = removeUtil(node->right, id);
    }
    // If ID is same as node's ID, then this is the node to be deleted
    else {
        // Node with only one child or no child
        if (node->left == nullptr) {
            Node_AVL* temp = node->right;
            delete node; // Deallocate the node
            node = nullptr; // Avoid dangling pointer issues before returning
            return temp;    // Return the right child (or nullptr if leaf)
        } else if (node->right == nullptr) {
            Node_AVL* temp = node->left;
            delete node; // Deallocate the node
            node = nullptr;
            return temp;    // Return the left child
        }

        // Node with two children: Get the in-order successor (smallest in the right subtree)
        // Inlined logic for findMinNode:
        Node_AVL* successor = node->right; // Start with the root of the right subtree
        while (successor != nullptr && successor->left != nullptr) {
            successor = successor->left; // Go to the leftmost node
        }
        // 'successor' now points to the in-order successor.

        // Copy the in-order successor's data (pointer) to this node
        node->data = successor->data; // We are only copying the pointer to Data

        // Delete the in-order successor from the right subtree
        node->right = removeUtil(node->right, successor->data->id);
    }

    // If the tree had only one node then return (node would be nullptr now if it was deleted)
    // This check is implicitly handled: if node became nullptr, balancing won't apply
    // But, if the deletion resulted in node becoming nullptr (e.g. single node tree),
    // the original 'node' pointer from the previous recursive call is what we need.
    // The 'node' here, if it was the one deleted and replaced by temp, is no longer the same.
    // The structure means 'node' parameter at this point is the parent of the operation.
    // So if after the recursive calls, 'node' (the current subtree root) is null, return it.
    if (node == nullptr) { // This can happen if the node itself was a leaf and deleted
      return nullptr;
    }


    // 2. Update height of the current node
    node->height = 1 + std::max(height(node->left), height(node->right));

    // 3. Get the balance factor of this node
    int16_t balance = getBalance(node);

    // 4. Rebalance if necessary (similar to insertUtil)

    // Left Heavy cases
    if (balance > 1) {
        if (getBalance(node->left) >= 0) { // LL Case
            return rightRotation(node);
        } else { // LR Case
            node->left = leftRotation(node->left);
            return rightRotation(node);
        }
    }

    // Right Heavy cases
    if (balance < -1) {
        if (getBalance(node->right) <= 0) { // RR Case
            return leftRotation(node);
        } else { // RL Case
            node->right = rightRotation(node->right);
            return leftRotation(node);
        }
    }

    return node; // Return the (possibly updated and rebalanced) node pointer
}

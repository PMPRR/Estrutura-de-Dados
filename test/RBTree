#include "../include/essential/RBTree.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <string>

// Função auxiliar para imprimir cabeçalhos de teste
void printTestHeader(const std::string& testName) {
    std::cout << "\n=== " << testName << " ===\n";
}

// Teste básico de inserção e busca
void testBasicInsertAndFind() {
    printTestHeader("Teste Básico de Inserção e Busca");
    
    essential::RBTree<int, std::string> tree;
    
    // Inserir alguns pares chave-valor
    tree.insert(10, "dez");
    tree.insert(20, "vinte");
    tree.insert(5, "cinco");
    tree.insert(15, "quinze");
    tree.insert(30, "trinta");
    
    // Verificar se os valores podem ser encontrados
    assert(tree.find(10).value() == "dez");
    assert(tree.find(20).value() == "vinte");
    assert(tree.find(5).value() == "cinco");
    assert(tree.find(15).value() == "quinze");
    assert(tree.find(30).value() == "trinta");
    
    // Verificar se chaves inexistentes retornam nullopt
    assert(!tree.find(100).has_value());
    
    std::cout << "Árvore após inserções:\n";
    tree.printTree();
    
    std::cout << "Teste concluído com sucesso!\n";
}

// Teste de atualização de valores
void testUpdateValues() {
    printTestHeader("Teste de Atualização de Valores");
    
    essential::RBTree<int, std::string> tree;
    
    // Inserir alguns pares chave-valor
    tree.insert(10, "dez");
    tree.insert(20, "vinte");
    
    // Verificar valores iniciais
    assert(tree.find(10).value() == "dez");
    assert(tree.find(20).value() == "vinte");
    
    // Atualizar valores
    tree.insert(10, "TEN");
    tree.insert(20, "TWENTY");
    
    // Verificar valores atualizados
    assert(tree.find(10).value() == "TEN");
    assert(tree.find(20).value() == "TWENTY");
    
    std::cout << "Teste concluído com sucesso!\n";
}

// Teste de remoção
void testRemoval() {
    printTestHeader("Teste de Remoção");
    
    essential::RBTree<int, std::string> tree;
    
    // Inserir alguns pares chave-valor
    tree.insert(10, "dez");
    tree.insert(20, "vinte");
    tree.insert(5, "cinco");
    tree.insert(15, "quinze");
    tree.insert(30, "trinta");
    
    std::cout << "Árvore antes da remoção:\n";
    tree.printTree();
    
    // Remover um nó folha
    assert(tree.remove(5) == true);
    assert(tree.find(5).has_value() == false);
    
    std::cout << "Árvore após remover 5 (nó folha):\n";
    tree.printTree();
    
    // Remover um nó com um filho
    assert(tree.remove(30) == true);
    assert(tree.find(30).has_value() == false);
    
    std::cout << "Árvore após remover 30 (nó com um filho):\n";
    tree.printTree();
    
    // Remover um nó com dois filhos
    assert(tree.remove(10) == true);
    assert(tree.find(10).has_value() == false);
    
    std::cout << "Árvore após remover 10 (nó com dois filhos):\n";
    tree.printTree();
    
    // Tentar remover um nó inexistente
    assert(tree.remove(100) == false);
    
    std::cout << "Teste concluído com sucesso!\n";
}

// Teste de percurso
void testTraversal() {
    printTestHeader("Teste de Percurso");
    
    essential::RBTree<int, std::string> tree;
    
    // Inserir alguns pares chave-valor
    tree.insert(10, "dez");
    tree.insert(20, "vinte");
    tree.insert(5, "cinco");
    tree.insert(15, "quinze");
    tree.insert(30, "trinta");
    
    std::cout << "Árvore:\n";
    tree.printTree();
    
    // Testar percurso em ordem
    std::cout << "Percurso em ordem (inorder):\n";
    std::vector<int> inorderKeys;
    tree.inorder([&inorderKeys](const int& key, const std::string& value) {
        std::cout << key << ": " << value << std::endl;
        inorderKeys.push_back(key);
    });
    
    // Verificar se as chaves estão em ordem crescente
    std::vector<int> sortedKeys = inorderKeys;
    std::sort(sortedKeys.begin(), sortedKeys.end());
    assert(inorderKeys == sortedKeys);
    
    // Testar percurso em pré-ordem
    std::cout << "\nPercurso em pré-ordem (preorder):\n";
    tree.preorder([](const int& key, const std::string& value) {
        std::cout << key << ": " << value << std::endl;
    });
    
    // Testar percurso em pós-ordem
    std::cout << "\nPercurso em pós-ordem (postorder):\n";
    tree.postorder([](const int& key, const std::string& value) {
        std::cout << key << ": " << value << std::endl;
    });
    
    std::cout << "Teste concluído com sucesso!\n";
}

// Teste de verificação de propriedades da árvore rubro-negra
void testRBProperties() {
    printTestHeader("Teste de Propriedades da Árvore Rubro-Negra");
    
    essential::RBTree<int, std::string> tree;
    
    // Inserir vários valores para testar as propriedades
    for (int i = 0; i < 100; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }
    
    // Verificar se as propriedades da árvore rubro-negra são mantidas
    assert(tree.verifyProperties() == true);
    
    // Remover alguns valores e verificar novamente
    for (int i = 0; i < 50; i += 2) {
        tree.remove(i);
    }
    
    assert(tree.verifyProperties() == true);
    
    std::cout << "Teste concluído com sucesso!\n";
}

// Teste de desempenho
void testPerformance() {
    printTestHeader("Teste de Desempenho");
    
    const int NUM_ELEMENTS = 100000;
    
    // Gerar números aleatórios
    std::vector<int> numbers;
    numbers.reserve(NUM_ELEMENTS);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, NUM_ELEMENTS * 10);
    
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        numbers.push_back(dis(gen));
    }
    
    // Testar inserção
    essential::RBTree<int, int> tree;
    
    auto startInsert = std::chrono::high_resolution_clock::now();
    
    for (int num : numbers) {
        tree.insert(num, num);
    }
    
    auto endInsert = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> insertTime = endInsert - startInsert;
    
    std::cout << "Tempo para inserir " << NUM_ELEMENTS << " elementos: " 
              << insertTime.count() << " ms" << std::endl;
    
    // Testar busca
    auto startSearch = std::chrono::high_resolution_clock::now();
    
    for (int num : numbers) {
        tree.find(num);
    }
    
    auto endSearch = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> searchTime = endSearch - startSearch;
    
    std::cout << "Tempo para buscar " << NUM_ELEMENTS << " elementos: " 
              << searchTime.count() << " ms" << std::endl

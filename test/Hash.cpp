#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <essential/HashTable.h> // Certifique-se de que este caminho está correto

// Função para testar inserção, busca e remoção de muitos elementos
void testManyElements() {
    const int N = 1000;
    essential::HashTable ht(127); // tamanho primo para espalhar melhor

    // Inserir pares do tipo chave="key42", valor="value42"
    for (int i = 0; i < N; ++i) {
        ht.insert("key" + std::to_string(i), "value" + std::to_string(i));
    }

    assert(ht.size() == N);

    // Testar busca
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        const std::string* val = ht.find(key);
        assert(val != nullptr);
        assert(*val == "value" + std::to_string(i));
    }

    // Testar atualização de valor
    ht.insert("key42", "updated_value");
    const std::string* val = ht.find("key42");
    assert(val != nullptr && *val == "updated_value");

    // Testar remoção
    size_t removed = 0;
    for (int i = 0; i < N; i += 2) {
        if (ht.remove("key" + std::to_string(i))) {
            ++removed;
        }
    }
    assert(ht.size() == (N - removed));

    // Testar busca de removidos e restantes
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        const std::string* val = ht.find(key);
        if (i % 2 == 0) {
            assert(val == nullptr); // removido
        } else {
            assert(val != nullptr && *val == "value" + std::to_string(i));
        }
    }

    ht.clear();
    assert(ht.size() == 0);
    std::cout << "[OK] testManyElements passou todos os testes.\n";
}

// Teste de operações básicas
void testBasic() {
    essential::HashTable ht(13);

    ht.insert("nome", "Maria");
    ht.insert("cidade", "São Paulo");
    ht.insert("idade", "30");

    assert(ht.size() == 3);

    // Busca
    assert(ht.find("nome") && *ht.find("nome") == "Maria");
    assert(ht.find("cidade") && *ht.find("cidade") == "São Paulo");
    assert(ht.find("idade") && *ht.find("idade") == "30");
    assert(ht.find("inexistente") == nullptr);

    // Atualização
    ht.insert("idade", "31");
    assert(ht.find("idade") && *ht.find("idade") == "31");

    // Remoção
    assert(ht.remove("nome"));
    assert(ht.find("nome") == nullptr);
    assert(ht.size() == 2);

    ht.clear();
    assert(ht.size() == 0);
    std::cout << "[OK] testBasic passou todos os testes.\n";
}

int main() {
    std::cout << "Testando HashTable...\n";
    testBasic();
    testManyElements();
    std::cout << "Todos os testes passaram!\n";
    return 0;
}

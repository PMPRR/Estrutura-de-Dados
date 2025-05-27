---
tags: uncompleted
link: "[[C++]]"
language: Português
aliases:
- C++ Memória
- Memória em C++
---
content: [[Programming]], [[C++]]
# C++ Memory
A memoria em um programa em C++ possuí o seguinte layout:
![[C++ Memory Layout.png]]
### Text:
Esse tipo de memória é onde se guarda os comando do programa e sua sequência de execução em *machine code*, essa memória é **read-only**
### Data:
Esse tipo de memória pode é declarada no **escopo global** ou tem o modificador `static` na sua declaração. Esse tipo de memória é fixa, ou seja nada pode ser alocado durante o programa, mas as variáveis podem ter seus valores mudados.
- **Initialized:** Variáveis que foram declaradas com um valor.
- **Uninitialized:** Variáveis que foram declaradas sem um valor inicial, nesse caso o compilador zera as variáveis.
### Heap:
- É uma memória de acesso permanente, ou seja, diferente do Stack, uma vez que a memória é alocada ela pode ser acessa a qualquer hora.
- É necessário guardar um ponteira para essa memória no Stack para conseguir acessa-la.
### Stack:
- É uma memória que depende do escopo, ou seja, ela guarda **apenas** variáveis locais e argumentos de uma função.
# References:
# Related:

%% ---------- FOOTNOTES -------%%
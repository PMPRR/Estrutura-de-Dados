---
tags: uncompleted
link:
language: Português
aliases:
- C++ Header Files
---
content: [[C++]]
# C++ Header Files
Header files são um arquivo em que se coloca a declaração de funções e classes, para que sejam importados para outros scripts.
## Estrutura:
```cpp
#ifndef NOME_DO_ARQUIVO_H_
#define NOME_DO_ARQUIVO_H_

... Declarações

#endif
```
### Exemplos:
- animals.h:
```cpp
#ifndef ANIMALS_H_
#define ANIMALS_H_

class Animal{
public:
	string name;
	void Sound();
	Animal(string name);
}

float test(int a, int b);

#endif
```
- animals.cpp:
```cpp
#include "animals.h"

void Animal::Sound(){
	printf("Bark!\n");
	return;
}

Animal::Animal(string name){
	this->name = name;
}

float test(int a, int b){
	return a/b;
}
```
# References:
# Related:
- [[C++ Classes]]
- [[C++ Templates]]

%% ---------- FOOTNOTES -------%%
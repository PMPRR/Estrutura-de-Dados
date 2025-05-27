---
tags: uncompleted
link:
language: Português
aliases:
- C++ Templates
---
content: [[C++]]
# C++ Templates
**Templates** é uma forma de generalização de funções e classes, cada instância de um template é criada durante a compilação do código.
### Exemplo
```cpp
template<typename T>
T add(T a, T b){
	return a + b;
}

template<typename U>
class Random{
	U varible;

	U getVariable(){
		return variable;
	}
	void setVariable(U variable){
		this->variable = variable
	}
}

int main(){
	int a = 5, b = 10;
	int sum = add<int>(a,b); // Retorna uma valor inteiro = 15

	float c = 2.5, d = 1.2;
	float soma = add<float>(c,d) // Retorna uma valor real = 3.7

	Random<char> C;
	C.setVariable('A');
	C.getVariable(); // Retorna um valor CHAR = "A"
}
```
# References:
# Related:
- [[C++ Classes]]

%% ---------- FOOTNOTES -------%%
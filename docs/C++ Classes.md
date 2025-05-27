---
tags: uncompleted
linklist: 
- "[[OOP]]"
- "[[C++]]"
language: Português
aliases:
- C++ Classes
- Classes em C++
---
content: [[Programming]], [[C++]]
# C++ Classes
Em C++ classes são declaradas da seguinte maneira:
```cpp
class Nome{
	public:
		\\Membros de acesso público
	protected:
		\\Membros de acesso protegido
	private:
		\\Membros de acesso privado
}
```
### Modificadores de Acesso:
- `public:` é um modificador que permite seus membros serem acessados fora da classe e por classes derivadas.
- `protected:` é um modificador que permite seus membro serem acessados **apenas** por classes derivadas, não é possível acessar fora da classe.
- `private:` é um modificador que permite  seus membros serem acessados **apenas** dentro da classe original.
### Construtores:
Construtores são uma função que são executados na **inicialização** do objeto;
Por padrão, construtores **devem** ser declarados com acesso `public`.
```cpp
class Nome{
	public:
		Nome(){
			... \\ comandos que serão chamados na inicialização
		}
}
```
>[!warning] Atenção para a memória Heap
>- Caso o objeto seja inicializado com o comando `malloc`, o construtor não será chamado automaticamente.
>- O único comando que chama o construtor na inicialização é o `new`
### Destrutores:
Destrutores são uma série de comandos que serão executados no **destruição** do objeto.
Por padrão, destrutores **devem** ser declarados com acesso `public`
```cpp
class Nome{
	public:
		~Nome(){
			... \\ comandos que serão chamados na destruição
		}
}
```
>[!warning] Atenção para a memória Heap
>- Caso o objeto seja destruído com o comando `free`, o destrutor não será chamado automaticamente
>- O único comando que chama o destrutor antes da liberação de memória é `delete`
## Criação de Objetos:
Em C++, diferente da linguagem C, para alocar memória no [[C++ Memory#Heap|Heap]] para o objeto **não se usa** o comando `malloc`.
Também **não se usa** o comando `free` para liberar a memória no [[C++ Memory#Heap|Heap]] .
```cpp
class Classe{
	public:
		int x;
}

int main(){
	Classe obj1; \\ Objeto com memória no Stack
	Classe* obj2 = new Classe(); \\ Objeto com memória no Heap

	delete obj2; \\ Destruição do objeto no Heap
}
```

# References:
# Related:
- [[C++ Memory]]

%% ---------- FOOTNOTES -------%%
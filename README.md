 Projeto de Estrutura de Dados
## Project Structure
```
├── CMakeLists.txt
├── README.md
├── docs/
├── test/
├── build/
├── include/
|   └── module/
|       └── module.h
└── src/
    └── module/
        └── module.cpp
```
- **docs:** Diretório para explicar funções/comportamento do código
- **test:** Diretório para colocar testes sobre o código
- **build:** Diretório para as dependencias do Cmake
- **include:** Diretório para as declarações(arquivos.h) do projeto
- **src:** Diretório para as definições(arquivos.cpp) do projeto

## How to Run:
#### Docker:
```shell
sudo docker build -t cpp_project .
sudo docker run --rm --name my_cpp_application -d cpp_project
```
### Docker Compose:
Para rodar uma vez o código use o `app-run`, caso queria programar dentro do container use `app-dev`
```shell
docker compose build app-run
docker compose run --rm app-run NOME_DO_EXECUTAVEL
```
```shell
docker compose build app-dev
docker compose up app-dev -d
docker compose exec -it app-dev bash
```
### Caso o docker Daemon NÂO estiver funcionando
```shell
$ systemctl --user start docker
```
### Para ativa-lo no início
```shell
$ systemctl --user enable docker
```
#### Manually:
É necessário ter instalado `gcc`, `cmake`.
Caso ainda não tenha criado o diretório `build/`:
```
mkdir build
```
Para compilar o projeto:
```
cd build
cmake .. && make
```
## ROADMAP
- [x] datastet escolhido (0.5)
- [x] implementar Linked List, AVL e Hashset (0.6)
- [x] implementar Cuckoo Hash e Segment Tree (1.0)
- [x] implementar Red-Black tree (0.3)
- [x] fazer interface gráfica (0.6)
- [x] benchmark (1)
- [x] predição por IA apartir do dataset (2)
- [x] fazer a simulação (0.3)
- [x] fazer calculos estatisticos (0.3)
- [x] fazer filtro e ordenação (0.3)
- [x] estrutura de dado otimizada Cuckoo (1)
- [x] estrutura de dado otimizada Skip-List otimizada (0.3)
- [ ] 5 testes difernetes em codições de restrição (1.5)


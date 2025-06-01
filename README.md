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
docker compose up app-run
```
```shell
docker compose build app-dev
docker compose up app-dev -d
docker exec -it CONTAINER_NUMBER bash
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


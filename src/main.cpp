#include <iostream>
#include "module-test/test.h"

#include <iostream>
#include "/home/fernandonishio/Estrutura-de-Dados/include/data.h"
#include "/home/fernandonishio/Estrutura-de-Dados/src/essential/LinkedList.cpp" // inclui a implementação completa

using namespace std;

int main() {
    // Criando uma lista encadeada
    DoublyLinkedList networkTrafficList;

    // Criando alguns dados de exemplo
    Data* data1 = new Data(
        1, // id
        10.5, // dur
        100.2, // rate
        500.0, // sload
        300.0, // dload
        0.01, // sinpkt
        0.02, // dinpkt
        5.0, // sjit
        6.0, // djit
        0.1, // tcprtt
        0.05, // synack
        0.07, // ackdat
        150, // spkts
        120, // dpkts
        75000, // sbytes
        60000, // dbytes
        64, // sttl
        128, // dttl
        2, // sloss
        1, // dloss
        8192, // swin
        12345, // stcpb
        54321, // dtcpb
        16384, // dwin
        500, // smean
        400, // dmean
        3, // trans_depth
        1024, // response_body_len
        5, // ct_srv_src
        10, // ct_state_ttl
        15, // ct_dst_ltm
        2, // ct_src_dport_ltm
        3, // ct_dst_sport_ltm
        4, // ct_dst_src_ltm
        0, // ct_ftp_cmd
        2, // ct_flw_http_mthd
        20, // ct_src_ltm
        8, // ct_srv_dst
        false, // is_ftp_login
        false, // is_sm_ips_ports
        false, // label (não é ataque)
        Protocolo::TCP, // proto
        State::CON, // state
        Attack_cat::NORMAL, // attack_category
        Servico::HTTP // service
    );

    Data* data2 = new Data(
        2, // id
        5.2, // dur
        200.5, // rate
        800.0, // sload
        400.0, // dload
        0.005, // sinpkt
        0.01, // dinpkt
        3.0, // sjit
        4.0, // djit
        0.08, // tcprtt
        0.03, // synack
        0.05, // ackdat
        200, // spkts
        150, // dpkts
        100000, // sbytes
        75000, // dbytes
        128, // sttl
        64, // dttl
        0, // sloss
        0, // dloss
        16384, // swin
        23456, // stcpb
        65432, // dtcpb
        32768, // dwin
        600, // smean
        500, // dmean
        5, // trans_depth
        2048, // response_body_len
        8, // ct_srv_src
        15, // ct_state_ttl
        20, // ct_dst_ltm
        3, // ct_src_dport_ltm
        4, // ct_dst_sport_ltm
        5, // ct_dst_src_ltm
        0, // ct_ftp_cmd
        3, // ct_flw_http_mthd
        30, // ct_src_ltm
        12, // ct_srv_dst
        true, // is_ftp_login
        true, // is_sm_ips_ports
        true, // label (é ataque)
        Protocolo::UDP, // proto
        State::FIN, // state
        Attack_cat::DOS, // attack_category
        Servico::FTP // service
    );

    Data* data3 = new Data(
        3, // id
        2.1, // dur
        50.8, // rate
        200.0, // sload
        100.0, // dload
        0.02, // sinpkt
        0.03, // dinpkt
        8.0, // sjit
        9.0, // djit
        0.15, // tcprtt
        0.08, // synack
        0.1, // ackdat
        80, // spkts
        60, // dpkts
        40000, // sbytes
        30000, // dbytes
        32, // sttl
        64, // dttl
        5, // sloss
        3, // dloss
        4096, // swin
        34567, // stcpb
        76543, // dtcpb
        8192, // dwin
        300, // smean
        250, // dmean
        1, // trans_depth
        512, // response_body_len
        3, // ct_srv_src
        5, // ct_state_ttl
        8, // ct_dst_ltm
        1, // ct_src_dport_ltm
        2, // ct_dst_sport_ltm
        1, // ct_dst_src_ltm
        1, // ct_ftp_cmd
        1, // ct_flw_http_mthd
        10, // ct_src_ltm
        5, // ct_srv_dst
        false, // is_ftp_login
        false, // is_sm_ips_ports
        false, // label (não é ataque)
        Protocolo::ICMP, // proto
        State::ECO, // state
        Attack_cat::NORMAL, // attack_category
        Servico::DNS // service
    );

    // Testando operações básicas
    cout << "=== Testando operações básicas ===" << endl;
    
    // Adicionando elementos
    networkTrafficList.append(data1);
    networkTrafficList.append(data2);
    networkTrafficList.insertAt(1, data3);
    
    cout << "Tamanho da lista: " << networkTrafficList.size() << endl;
    
    // Buscando por ID
    Data* foundData = networkTrafficList.findById(2);
    if (foundData) {
        cout << "Encontrado registro com ID 2. Duração: " << foundData->dur << "s" << endl;
    } else {
        cout << "Registro com ID 2 não encontrado." << endl;
    }
    
    // Removendo por ID
    bool removed = networkTrafficList.removeById(1);
    cout << "Remoção do ID 1: " << (removed ? "sucesso" : "falha") << endl;
    cout << "Novo tamanho da lista: " << networkTrafficList.size() << endl;
    
    // Testando estatísticas
    cout << "\n=== Testando estatísticas ===" << endl;
    
    // Média de duração
    auto durAccessor = [](Data* d) { return d->dur; };
    cout << "Média de duração: " << networkTrafficList.average(durAccessor) << "s" << endl;
    
    // Desvio padrão da taxa
    auto rateAccessor = [](Data* d) { return d->rate; };
    cout << "Desvio padrão da taxa: " << networkTrafficList.stddev(rateAccessor) << endl;
    
    // Mediana de bytes de origem
    auto sbytesAccessor = [](Data* d) { return d->sbytes; };
    cout << "Mediana de bytes de origem: " << networkTrafficList.median(sbytesAccessor) << endl;
    
    // Mínimo e máximo de jitter de destino
    auto djitAccessor = [](Data* d) { return d->djit; };
    cout << "Mínimo jitter de destino: " << networkTrafficList.min(djitAccessor) << "ms" << endl;
    cout << "Máximo jitter de destino: " << networkTrafficList.max(djitAccessor) << "ms" << endl;
    
    // Histograma de duração
    cout << "\nHistograma de duração (3 bins):" << endl;
    networkTrafficList.histogram(durAccessor, 3);
    
    return 0;
}
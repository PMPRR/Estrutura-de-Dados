#include "essential/AVL.h"
#include <iostream>
#include <vector>

std::vector<Data> create_sample_data_for_testing() {
    std::vector<Data> samples;

    // Sample 1: Simulating a "Normal" TCP HTTP connection
    samples.emplace_back(
        1001,                           // id
        1.5f,                           // dur
        10.0f,                          // rate
        1500.0f,                        // sload
        800.0f,                         // dload
        0.15f,                          // sinpkt
        0.08f,                          // dinpkt
        5.0f,                           // sjit
        3.0f,                           // djit
        0.05f,                          // tcprtt
        0.02f,                          // synack
        0.03f,                          // ackdat
        15,                             // spkts
        12,                             // dpkts
        7500,                           // sbytes
        4800,                           // dbytes
        64,                             // sttl
        128,                            // dttl
        0,                              // sloss
        0,                              // dloss
        65535,                          // swin
        100000,                         // stcpb
        200000,                         // dtcpb
        32768,                          // dwin
        500,                            // smean
        400,                            // dmean
        1,                              // trans_depth
        1200,                           // response_body_len
        5,                              // ct_srv_src
        // REMOVED: 2,                 // ct_state_ttl (this argument has been removed)
        10,                             // ct_dst_ltm
        3,                              // ct_src_dport_ltm
        4,                              // ct_dst_sport_ltm
        8,                              // ct_dst_src_ltm
        0,                              // ct_ftp_cmd
        1,                              // ct_flw_http_mthd
        20,                             // ct_src_ltm
        6,                              // ct_srv_dst
        false,                          // is_ftp_login
        false,                          // is_sm_ips_ports
        false,                          // label (Normal)
        Protocolo::TCP,
        State::FIN,
        Attack_cat::NORMAL,
        Servico::HTTP
    );

    // Sample 2: Simulating a "DoS" UDP DNS query
    samples.emplace_back(
        1002,                           // id
        0.8f,                           // dur
        100.0f,                         // rate (high rate)
        50000.0f,                       // sload (high load)
        100.0f,                         // dload
        0.01f,                          // sinpkt (fast packets)
        0.5f,                           // dinpkt
        20.0f,                          // sjit
        1.0f,                           // djit
        0.0f,                           // tcprtt (UDP has no TCP RTT)
        0.0f,                           // synack
        0.0f,                           // ackdat
        500,                            // spkts (many source packets)
        5,                              // dpkts
        250000,                         // sbytes (large source bytes)
        250,                            // dbytes
        32,                             // sttl
        0,                              // dttl (maybe no response)
        10,                             // sloss (some loss)
        0,                              // dloss
        0,                              // swin (UDP)
        0,                              // stcpb
        0,                              // dtcpb
        0,                              // dwin
        500,                            // smean
        50,                             // dmean
        0,                              // trans_depth
        0,                              // response_body_len
        200,                            // ct_srv_src (many connections from same service/src)
        // REMOVED: 50,                // ct_state_ttl (this argument has been removed)
        5,                              // ct_dst_ltm
        150,                            // ct_src_dport_ltm
        2,                              // ct_dst_sport_ltm
        10,                             // ct_dst_src_ltm
        0,                              // ct_ftp_cmd
        0,                              // ct_flw_http_mthd
        300,                            // ct_src_ltm (many from this source)
        10,                             // ct_srv_dst
        false,                          // is_ftp_login
        true,                           // is_sm_ips_ports (could be true for DoS)
        true,                           // label (Attack)
        Protocolo::UDP,
        State::INT,                     // Internal, or perhaps REQ if it's a request flood
        Attack_cat::DOS,
        Servico::DNS
    );

    // Sample 3: Simulating a "Reconnaissance" ICMP Ping
    samples.emplace_back(
        1003,                           // id
        12.5f,                          // dur (longer duration for scanning)
        0.5f,                           // rate (slow rate for stealth)
        60.0f,                          // sload
        60.0f,                          // dload
        2.0f,                           // sinpkt
        2.0f,                           // dinpkt
        0.1f,                           // sjit
        0.1f,                           // djit
        0.0f,                           // tcprtt
        0.0f,                           // synack
        0.0f,                           // ackdat
        5,                              // spkts
        5,                              // dpkts
        300,                            // sbytes (small ICMP packets)
        300,                            // dbytes
        255,                            // sttl
        255,                            // dttl
        0,                              // sloss
        0,                              // dloss
        0,                              // swin
        0,                              // stcpb
        0,                              // dtcpb
        0,                              // dwin
        60,                             // smean
        60,                             // dmean
        0,                              // trans_depth
        0,                              // response_body_len
        1,                              // ct_srv_src
        // REMOVED: 1,                 // ct_state_ttl (this argument has been removed)
        1,                              // ct_dst_ltm (probing different hosts)
        1,                              // ct_src_dport_ltm
        1,                              // ct_dst_sport_ltm
        1,                              // ct_dst_src_ltm
        0,                              // ct_ftp_cmd
        0,                              // ct_flw_http_mthd
        5,                              // ct_src_ltm
        1,                              // ct_srv_dst
        false,                          // is_ftp_login
        false,                          // is_sm_ips_ports
        true,                           // label (Attack)
        Protocolo::ICMP,
        State::ECO,                     // Echo request/reply
        Attack_cat::RECONNAISSANCE,
        Servico::NOTHING                // ICMP often doesn't map to a service like FTP/HTTP
    );

    // Sample 4: Simulating an "FTP Backdoor" attempt
    samples.emplace_back(
        1004,                           // id
        60.2f,                          // dur (longer lived connection)
        2.0f,                           // rate
        200.0f,                         // sload
        150.0f,                         // dload
        0.8f,                           // sinpkt
        1.2f,                           // dinpkt
        10.0f,                          // sjit
        15.0f,                          // djit
        0.2f,                           // tcprtt
        0.1f,                           // synack
        0.1f,                           // ackdat
        30,                             // spkts
        25,                             // dpkts
        3000,                           // sbytes
        2000,                           // dbytes
        60,                             // sttl
        60,                             // dttl
        1,                              // sloss
        1,                              // dloss
        8192,                           // swin
        50000,                          // stcpb
        60000,                          // dtcpb
        4096,                           // dwin
        100,                            // smean
        80,                             // dmean
        5,                              // trans_depth (multiple commands)
        50,                             // response_body_len (small command responses)
        2,                              // ct_srv_src
        // REMOVED: 2,                 // ct_state_ttl (this argument has been removed)
        3,                              // ct_dst_ltm
        1,                              // ct_src_dport_ltm
        1,                              // ct_dst_sport_ltm
        2,                              // ct_dst_src_ltm
        4,                              // ct_ftp_cmd (multiple FTP commands)
        0,                              // ct_flw_http_mthd
        5,                              // ct_src_ltm
        2,                              // ct_srv_dst
        true,                           // is_ftp_login (successful or attempted login)
        true,                           // is_sm_ips_ports (suspicious if backdoor port)
        true,                           // label (Attack)
        Protocolo::TCP,
        State::CON,                     // Connection established, ongoing
        Attack_cat::BACKDOOR,
        Servico::FTP
    );


    return samples;
}

int main(){
    std::vector<Data> test_data = create_sample_data_for_testing();

    AVL* tree = new AVL();

    for(const auto& data_item : test_data){
        tree->insert(&data_item);
    }
    tree->printAsciiTree();

    AVL::Node_AVL* node = tree->queryById(1002);
    // Ensure node->data is not nullptr before dereferencing
    if (node && node->data) {
        std::cout << "Query Result for ID 1002: ct_srv_dst = " << node->data->ct_srv_dst << std::endl;
    } else {
        std::cout << "Node with ID 1002 not found or data is null." << std::endl;
    }


    tree->removeById(1002);
    tree->printAsciiTree();

    // Remember to delete the AVL tree to free allocated memory
    delete tree;
    tree = nullptr; // Set to nullptr after deletion
}


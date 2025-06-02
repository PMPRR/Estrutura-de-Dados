#ifndef DATA_H_
#define DATA_H_

#include <cstdint>

/*
 * Com ajuda do ChatGPT eu pedi para ele documentar e criar ENUMS para dados categóricos
 *
 * [FORAM MUITOS PROMPTS]
 *
*/
enum class Protocolo : uint8_t {
    TCP = 0,             ///< Transmission Control Protocol
    UDP,                ///< User Datagram Protocol
    ARP,                ///< Address Resolution Protocol
    OSPF,               ///< Open Shortest Path First
    ICMP,               ///< Internet Control Message Protocol
    IGMP,               ///< Internet Group Management Protocol
    RTP,                ///< Real-time Transport Protocol
    DDP,                ///< Datagram Delivery Protocol
    IPV6_FRAG,          ///< IPv6 Fragment Header
    CFTP,               ///< CFTP
    WSN,                ///< Wireless Sensor Network
    PVP,                ///< PVP Protocol
    WB_EXPAK,           ///< Wideband Expedited Packet
    MTP,                ///< Multicast Transport Protocol
    PRI_ENC,            ///< Private Encryption
    SAT_MON,            ///< Satellite Monitoring
    CPHB,               ///< CPHB Protocol
    SUN_ND,             ///< Sun Neighbor Discovery
    ISO_IP,             ///< ISO Internet Protocol
    XTP,                ///< Xpress Transfer Protocol
    IL,                 ///< IL Transport Protocol
    UNAS,               ///< UNAS Protocol
    MFE_NSP,            ///< MFE Network Services Protocol
    THREE_PC,           ///< Three-Way Handshake Protocol
    IPV6_ROUTE,         ///< IPv6 Routing Header
    IDRP,               ///< Inter-Domain Routing Protocol
    BNA,                ///< Banyan VINES
    SWIPE,              ///< IP with Encryption
    KRYPTOLAN,          ///< Kryptolan Protocol
    CPNX,               ///< CPNX Protocol
    RSVP,               ///< Resource Reservation Protocol
    WB_MON,             ///< Wideband Monitoring
    VMTP,               ///< Versatile Message Transport Protocol
    IB,                 ///< IB Protocol
    DGP,                ///< Dissimilar Gateway Protocol
    EIGRP,              ///< Enhanced Interior Gateway Routing Protocol
    AX_25,              ///< Amateur Radio AX.25
    GMTP,               ///< GMTP Protocol
    PNNI,               ///< PNNI Protocol
    SEP,                ///< SEP Protocol
    PGM,                ///< Pragmatic General Multicast
    IDPR_CMTP,          ///< IDPR Control Message Transport Protocol
    ZERO,              ///< Protocol Zero
    RVD,                ///< Remote Virtual Disk
    MOBILE,             ///< IP Mobility
    NARP,               ///< NBMA Address Resolution Protocol
    FC,                 ///< Fibre Channel
    PIPE,               ///< PIPE Protocol
    IPCOMP,             ///< IP Payload Compression Protocol
    IPV6_NO,            ///< IPv6 No Next Header
    SAT_EXPAK,          ///< SATNET and Backroom EXPAK
    IPV6_OPTS,          ///< IPv6 Destination Options
    SNP,                ///< SNP Protocol
    IPCV,               ///< IPCV Protocol
    BR_SAT_MON,         ///< Backroom SATNET Monitoring
    TTP,                ///< TTP Protocol
    TCF,                ///< TCF Protocol
    NSFNET_IGP,         ///< NSFNET Interior Gateway Protocol
    SPRITE_RPC,         ///< Sprite RPC Protocol
    AES_SP3_D,          ///< AES SP3-D Protocol
    SCCOPMCE,           ///< SCCOPMCE Protocol
    SCTP,               ///< Stream Control Transmission Protocol
    QNX,                ///< QNX Protocol
    SCPS,               ///< SCPS Protocol
    ETHERIP,            ///< Ethernet-in-IP
    ARIS,               ///< ARIS Protocol
    PIM,                ///< Protocol Independent Multicast
    COMPAQ_PEER,        ///< Compaq Peer Protocol
    VRRP,               ///< Virtual Router Redundancy Protocol
    IATP,               ///< IATP Protocol
    STP,                ///< Stream Protocol
    L2TP,               ///< Layer 2 Tunneling Protocol
    SRP,                ///< SRP Protocol
    SM,                 ///< SM Protocol
    ISIS,               ///< IS-IS over IPv4
    SMP,                ///< SMP Protocol
    FIRE,               ///< FIRE Protocol
    PTP,                ///< Precision Time Protocol
    CRTP,               ///< Compressed RTP
    SPS,                ///< SPS Protocol
    MERIT_INP,          ///< MERIT Internodal Protocol
    IDPR,               ///< Inter-Domain Policy Routing
    SKIP,               ///< Simple Key Management for Internet Protocols
    ANY,                ///< Any protocol
    LARP,               ///< LARP Protocol
    IPIP,               ///< IP-in-IP Encapsulation
    MICP,               ///< MICP Protocol
    ENCAP,              ///< Encap Protocol
    IFMP,               ///< Ipsilon Flow Management Protocol
    TP_PP,              ///< TP++ Transport Protocol
    AN,                 ///< AN Protocol
    IPV6,               ///< Internet Protocol version 6
    I_NLSP,             ///< I-NLSP Protocol
    IPX_N_IP,           ///< IPX in IP
    SDRP,               ///< Source Demand Routing Protocol
    TLSP,               ///< TLSP Protocol
    GRE,                ///< Generic Routing Encapsulation
    MHRP,               ///< Mobile Host Routing Protocol
    DDX,                ///< DDX Protocol
    IPPC,               ///< IPPC Protocol
    VISA,               ///< VISA Protocol
    SECURE_VMTP,        ///< Secure Versatile Message Transport
    UTI,                ///< UTI Protocol
    VINES,              ///< VINES Protocol
    CRUDP,              ///< CRUDP Protocol
    IPLT,               ///< IPLT Protocol
    GGP,                ///< Gateway-to-Gateway Protocol
    IP,                 ///< Internet Protocol
    IPNIP,              ///< IP-within-IP
    ST2,                ///< ST-II Protocol
    ARGUS,              ///< ARGUS Protocol
    BBN_RCC,            ///< BBN RCC Monitoring
    EGP,                ///< Exterior Gateway Protocol
    EMCON,              ///< Emission Control Protocol
    IGP,                ///< Interior Gateway Protocol
    NVP,                ///< Network Voice Protocol
    PUP,                ///< PARC Universal Packet
    XNET,               ///< Cross Net Debugger
    CHAOS,              ///< CHAOS Protocol
    MUX,                ///< Multiplexing Protocol
    DCN,                ///< DCN Measurement Subsystems
    HMP,                ///< Host Monitoring Protocol
    PRM,                ///< Packet Radio Measurement
    TRUNK_1,            ///< TRUNK-1 Protocol
    XNS_IDP,            ///< Xerox NS IDP
    LEAF_1,             ///< Leaf-1 Protocol
    LEAF_2,             ///< Leaf-2 Protocol
    RDP,                ///< Reliable Data Protocol
    IRTP,               ///< Internet Reliable Transaction Protocol
    ISO_TP4,            ///< ISO Transport Protocol Class 4
    NETBLT,             ///< Bulk Data Transfer Protocol
    TRUNK_2,            ///< TRUNK-2 Protocol
    CBT                 ///< Core Based Trees Multicast Routing
};

enum class Servico : uint8_t {
    NOTHING = 0, ///< No service detected or unspecified.
    FTP,         ///< File Transfer Protocol (control connection) - typically port 21.
    SMTP,        ///< Simple Mail Transfer Protocol - used for email transmission.
    SNMP,        ///< Simple Network Management Protocol - for network device monitoring.
    HTTP,        ///< HyperText Transfer Protocol - typically port 80 (web traffic).
    FTP_DATA,    ///< FTP Data transfer connection - typically port 20.
    DNS,         ///< Domain Name System - used for resolving domain names.
    SSH,         ///< Secure Shell - encrypted remote login, typically port 22.
    RADIUS,      ///< Remote Authentication Dial-In User Service - used for AAA (Authentication, Authorization, Accounting).
    POP3,        ///< Post Office Protocol 3 - used for retrieving email from a server.
    DHCP,        ///< Dynamic Host Configuration Protocol - for automatic IP address assignment.
    SSL,         ///< Secure Sockets Layer - encrypted communications, often superseded by TLS.
    IRC          ///< Internet Relay Chat - real-time text communication protocol.
};

enum class Attack_cat : uint8_t {
    NORMAL = 0,       ///< Normal traffic, no attack.
    BACKDOOR,         ///< Attack allowing unauthorized access to systems, e.g., through hidden services.
    ANALYSIS,         ///< Information gathering attacks like scanning, fingerprinting, or packet sniffing.
    FUZZERS,          ///< Fuzzing attacks using malformed inputs to find vulnerabilities.
    SHELLCODE,        ///< Injection of shellcode for unauthorized system control.
    RECONNAISSANCE,   ///< Passive/active information gathering (e.g., port scanning, vulnerability probing).
    EXPLOITS,         ///< Direct exploitation of vulnerabilities, e.g., buffer overflows.
    DOS,              ///< Denial of Service attacks aimed at exhausting system resources.
    WORMS,            ///< Self-replicating malware spreading over the network.
    GENERIC          ///< Generic attacks not fitting into other specific categories, often automated exploit tools.
};

enum class State : uint8_t {
    FIN = 0,  ///< Connection finished properly.
    INT,      ///< Internal network traffic.
    CON,      ///< Connection attempt seen.
    ECO,      ///< Echo request (e.g., ICMP).
    REQ,      ///< Application layer request.
    RST,      ///< Connection reset.
    PAR,      ///< Partial connection or parsing issue.
    URN,      ///< Unknown or unreachable.
    NO,       ///< No state recorded.
    ACC,      ///< Connection accepted.
    CLO,      ///< Connection closed.
};

/*
    * Automatizado com a ajuda do CHATGPT, prompt abaixo:
    * """
    * ifndef DATA_H_
    * #define DATA_H_
    * 
    * class Data{
    * public:
    *     bool label;  ///< True when is a attack, False when is a normal connection
    *     float dur;  ///< Flow duration in seconds
    * };
    * 
    * 
    * #endif
    * 
    * Continue this class with the table you created
    * """
    *
*/
class Data{
public:
    // ——   Identifiers   ——
    uint32_t    id;                 ///< Unique flow/record ID (one per row)

    // ——   Numeric / Continuous Features   ——
    float       dur;                ///< Flow duration in seconds
    float       rate;               ///< Mean packet rate (packets/sec) for this flow
    float       sload;              ///< Source‐side load in bytes/sec
    float       dload;              ///< Destination‐side load in bytes/sec
    float       sinpkt;             ///< Inter‐arrival time (seconds) of source packets
    float       dinpkt;             ///< Inter‐arrival time (seconds) of destination packets
    float       sjit;               ///< Jitter (ms) observed at source side
    float       djit;               ///< Jitter (ms) observed at destination side
    float       tcprtt;             ///< TCP round‐trip time (seconds)
    float       synack;             ///< SYN‐ACK response time (seconds)
    float       ackdat;             ///< ACK/Data response time (seconds)

    // ——   Integer (Integral) Features   ——
    uint16_t    spkts;              ///< Number of packets from source → destination
    uint16_t    dpkts;              ///< Number of packets from destination → source
    uint32_t    sbytes;             ///< Total bytes from source → destination
    uint32_t    dbytes;             ///< Total bytes from destination → source

    uint8_t     sttl;               ///< Time‐to‐live value at source (0–255)
    uint8_t     dttl;               ///< Time‐to‐live value at destination (0–255)

    uint16_t    sloss;              ///< Total packets lost at source side
    uint16_t    dloss;              ///< Total packets lost at destination side

    uint16_t    swin;               ///< TCP window size (bytes) at source side
    uint16_t    stcpb;              ///< TCP base sequence number at source
    uint16_t    dtcpb;              ///< TCP base sequence number at destination
    uint16_t    dwin;               ///< TCP window size (bytes) at destination

    uint16_t    smean;              ///< Mean packet size (bytes) from source
    uint16_t    dmean;              ///< Mean packet size (bytes) from destination

    uint16_t    trans_depth;        ///< Transaction depth (number of app‐level transactions)

    uint32_t    response_body_len;  ///< Length (bytes) of response body (if any)

    uint16_t    ct_srv_src;         ///< Count of flows with same (service, source IP) in last 100
    uint16_t    ct_state_ttl;       ///< Count of flows with same (state, TTL) in last 100
    uint16_t    ct_dst_ltm;         ///< Count of connections to same destination in last 1000
    uint16_t    ct_src_dport_ltm;   ///< Count of connections from same source & destination port in last 1000
    uint16_t    ct_dst_sport_ltm;   ///< Count of connections to same destination & source port in last 1000
    uint16_t    ct_dst_src_ltm;     ///< Count of connections where (dest IP, source IP) appear in last 1000

    uint16_t    ct_ftp_cmd;         ///< Count of FTP commands in this flow
    uint16_t    ct_flw_http_mthd;   ///< Count of HTTP methods (GET/POST/…) in this flow
    uint16_t    ct_src_ltm;         ///< Count of connections from this source IP in last 1000
    uint16_t    ct_srv_dst;         ///< Count of connections with same (service, dest IP) in last 1000

    // ——   Boolean (Flags) Features   ——
    bool        is_ftp_login;       ///< True if an FTP login occurred, else False
    bool        is_sm_ips_ports;    ///< True if suspicious mixing of IPs/ports, else False

    bool        label;              ///< True = attack, False = normal connection
    
    // -- Categorical Features --

    Protocolo   proto;
    State       state;
    Attack_cat  attack_category;
    Servico     service;

    // -- Constructor --
    Data(uint32_t id,
        float dur, float rate, float sload, float dload,
        float sinpkt, float dinpkt, float sjit, float djit,
        float tcprtt, float synack, float ackdat,
        uint16_t spkts, uint16_t dpkts, uint32_t sbytes, uint32_t dbytes,
        uint8_t sttl, uint8_t dttl, uint16_t sloss, uint16_t dloss,
        uint16_t swin, uint16_t stcpb, uint16_t dtcpb, uint16_t dwin,
        uint16_t smean, uint16_t dmean, uint16_t trans_depth, uint32_t response_body_len,
        uint16_t ct_srv_src, uint16_t ct_state_ttl, uint16_t ct_dst_ltm,
        uint16_t ct_src_dport_ltm, uint16_t ct_dst_sport_ltm, uint16_t ct_dst_src_ltm,
        uint16_t ct_ftp_cmd, uint16_t ct_flw_http_mthd,
        uint16_t ct_src_ltm, uint16_t ct_srv_dst,
        bool is_ftp_login, bool is_sm_ips_ports, bool label,
        Protocolo proto, State state, Attack_cat attack_category, Servico service
    ) :
        id(id),
        dur(dur), rate(rate), sload(sload), dload(dload),
        sinpkt(sinpkt), dinpkt(dinpkt), sjit(sjit), djit(djit),
        tcprtt(tcprtt), synack(synack), ackdat(ackdat),
        spkts(spkts), dpkts(dpkts), sbytes(sbytes), dbytes(dbytes),
        sttl(sttl), dttl(dttl),
        sloss(sloss), dloss(dloss),
        swin(swin), stcpb(stcpb), dtcpb(dtcpb), dwin(dwin),
        smean(smean), dmean(dmean),
        trans_depth(trans_depth), response_body_len(response_body_len),
        ct_srv_src(ct_srv_src), ct_state_ttl(ct_state_ttl), ct_dst_ltm(ct_dst_ltm),
        ct_src_dport_ltm(ct_src_dport_ltm), ct_dst_sport_ltm(ct_dst_sport_ltm), ct_dst_src_ltm(ct_dst_src_ltm),
        ct_ftp_cmd(ct_ftp_cmd), ct_flw_http_mthd(ct_flw_http_mthd),
        ct_src_ltm(ct_src_ltm), ct_srv_dst(ct_srv_dst),
        is_ftp_login(is_ftp_login), is_sm_ips_ports(is_sm_ips_ports),
        label(label),
        proto(proto), state(state), attack_category(attack_category), service(service)
    {};
};

constexpr const char* PATH_DATA_TESTING = "/app/data/UNSW_NB15_testing-set.csv";
constexpr const char* PATH_DATA_TRAINING = "/app/data/UNSW_NB15_training-set.csv";

#endif


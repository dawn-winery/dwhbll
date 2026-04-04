module;

// #include <cstdint>
// #include <string>
// #include <variant>
// #include <vector>
//
// #include <dwhbll/network/ip_address.h>

module dwhbll.dns2:proto;

// TODO: use Fixed String!
namespace dwhbll::modules::dns2 {
    // struct Label {
    //     std::string name;
    // };
    //
    // struct Domain {
    //     std::vector<Label> labels;
    // };
    //
    // enum class QTYPE {
    //     A = 1,
    //     NS = 2,
    //     MD = 3,
    //     MF = 4,
    //     CNAME = 5,
    //     SOA = 6,
    //     MB = 7,
    //     MG = 8,
    //     MR = 9,
    //     NUL = 10,
    //     WKS = 11,
    //     PTR = 12,
    //     HINFO = 13,
    //     MINFO = 14,
    //     MX = 15,
    //     TXT = 16,
    //
    //     AXFR = 252,
    //     MAILB = 253,
    //     MAILA = 254,
    //     STAR = 255,
    // };
    //
    // enum class QCLASS {
    //     IN = 1,
    //     CS = 2,
    //     CH = 3,
    //     HS = 4,
    //
    //     STAR = 255,
    // };
    //
    // struct RRCNAME {
    //     Domain cname;
    // };
    //
    // struct RRHINFO {
    //     std::string CPU;
    //     std::string OS;
    // };
    //
    // struct RRMB {
    //     Domain madname;
    // };
    //
    // struct RRMD {
    //     Domain madname;
    // };
    //
    // struct RRMF {
    //     Domain madname;
    // };
    //
    // struct RRMG {
    //     Domain madname;
    // };
    //
    // struct RRMINFO {
    //     Domain rmailbox;
    //     Domain emailbox;
    // };
    //
    // struct RRMR {
    //     Domain newname;
    // };
    //
    // struct RRMX {
    //     std::int16_t preference;
    //     Domain exchange;
    // };
    //
    // struct RRNUL {
    //     std::vector<std::uint8_t> data;
    // };
    //
    // struct RRNS {
    //     Domain nsdname;
    // };
    //
    // struct RRPTR {
    //     Domain ptrname;
    // };
    //
    // struct RRSOA {
    //     Domain mname;
    //     Domain dname;
    //     std::uint32_t serial;
    //     std::uint32_t refresh;
    //     std::uint32_t retry;
    //     std::uint32_t expire;
    //     std::uint32_t minimum;
    // };
    //
    // struct RRTXT {
    //     std::vector<std::string> data;
    // };
    //
    // struct RRA {
    //     network::IPAddress address;
    // };
    //
    // struct RRWKS {
    //     network::IPAddress address;
    //     std::uint8_t protocol;
    //     std::vector<bool> map;
    // };
    //
    // struct ResourceRecord {
    //     Domain name;
    //     QTYPE type;
    //     QCLASS clazz;
    //     std::int32_t ttl;
    //     std::uint16_t rdlength;
    //
    //     struct unknown {
    //         std::vector<std::uint8_t> data;
    //     };
    //
    //     std::variant<RRCNAME, RRHINFO, RRMB, RRMD, RRMF, RRMG, RRMINFO, RRMR, RRMX, RRNUL, RRNS, RRPTR, RRSOA, RRTXT,
    //         RRA, RRWKS, unknown> data;
    // };
    //
    // struct DNSHeader {
    //     std::uint16_t id;
    //     bool is_response; ///< QR in spec
    //     enum OPCODE {
    //         QUERY = 0,
    //         IQUERY = 1,
    //         STATUS = 2,
    //     } opcode;
    //
    //     bool authoritative;
    //     bool truncation;
    //     bool recursion_desired;
    //     bool recursion_available;
    //     enum RCODE {
    //         NO_ERROR = 0,
    //         FORMAT_ERROR = 1,
    //         SERVER_FAILURE = 2,
    //         NAME_ERROR = 3,
    //         NOT_IMPLEMENTED = 4,
    //         REFUSED = 5,
    //     } return_code;
    //     std::uint16_t question_count;
    //     std::uint16_t answer_count;
    //     std::uint16_t authority_count;
    //     std::uint16_t additional_count;
    // };
    //
    // struct DNSQuestion {
    //     Domain name;
    //     QTYPE type;
    //     QCLASS clazz;
    // };
    //
    // struct DNSMessage {
    //     DNSHeader header;
    //     std::vector<DNSQuestion> questions;
    //     std::vector<ResourceRecord> answer;
    //     std::vector<ResourceRecord> authority;
    //     std::vector<ResourceRecord> additional;
    // };
}

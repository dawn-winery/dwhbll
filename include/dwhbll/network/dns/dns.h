#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <netinet/ip.h>

#include <dwhbll/collections/ring.h>
#include <dwhbll/network/SocketManager.h>

namespace dwhbll::network::dns {
    /*
     * Constant data: Root Server data
     */
    const std::vector<std::uint32_t> root_servers = {
        BuildIPV4(198, 41, 0, 4),
        BuildIPV4(170, 247, 170, 2),
        BuildIPV4(192, 33, 4, 12),
        BuildIPV4(199, 7, 91, 13),
        BuildIPV4(192, 203, 230, 10),
        BuildIPV4(192, 5, 5, 241),
        BuildIPV4(192, 112, 36, 4),
        BuildIPV4(198, 97, 190, 53),
        BuildIPV4(192, 36, 148, 17),
        BuildIPV4(192, 58, 128, 30),
        BuildIPV4(193, 0, 14, 129),
        BuildIPV4(199, 7, 83, 42),
        BuildIPV4(202, 12, 27, 33),
    };

    std::optional<in_addr> query_dns(const std::string& domain);

    struct MemoryStream {
        collections::Ring<char> data;
        std::int64_t current_head{0};

        std::uint8_t get_uint8();

        std::uint16_t get_uint16();

        std::uint32_t get_uint32();

        std::vector<std::uint8_t> get_array(std::size_t length);

        void write_uint8(std::uint8_t value);

        void write_uint16(std::uint16_t value);

        void write_uint32(std::uint32_t value);

        void write_array(const std::span<std::uint8_t>& data);
    };

    // TODO: move domain parsing to a separate header or library.
    class Domain {
        std::vector<std::string> labels;

    public:
        enum class parse_errors;

    private:
        static std::expected<std::string, parse_errors> try_parse_label(const std::string& domain, std::size_t& start);

    public:
        enum class parse_errors {
            INVALID_LABEL_START, ///< Label start is not a letter
            INVALID_LABEL_END, ///< Label end is not a letter or digit.
        };

        static Domain parse(const std::string& domain);

        static Domain unpack(MemoryStream& stream);

        static void pack(const Domain& domain, MemoryStream& stream);

        [[nodiscard]] std::string to_string() const;

        friend bool operator==(const Domain &lhs, const Domain &rhs) {
            if (lhs.labels.size() != rhs.labels.size())
                return false;
            for (int i = 0; i < lhs.labels.size(); i++) {
                if (lhs.labels[i] != rhs.labels[i])
                    return false;
            }
            return true;
        }

        friend bool operator!=(const Domain &lhs, const Domain &rhs) {
            return !operator==(lhs, rhs);
        }
    };

    namespace ResourceRecordInner {
        // covers: CNAME, MB, MD, MF, MG, MR, NS, PTR records
        struct DOMAIN_RECORD {
            Domain name;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        struct HINFO {
            std::string cpu, os;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        struct MINFO {
            Domain rmailbx, emailbx;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        struct MX {
            std::int16_t preference;
            Domain exchange;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        /**
         * very special record, literally just a vector with data, it personally doesn't even know its own size while
         * parsing. This record ends up getting filled by the unpack records of the ResourceRecord class
         */
        struct NUL {
            std::vector<char> data;

            [[nodiscard]] std::string to_string() const;
        };

        struct SOA {
            Domain mname;
            Domain rname;
            std::uint32_t serial;
            std::int32_t refresh;
            std::int32_t retry;
            std::int32_t expire;
            std::int32_t minimum;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        /**
         * another special record, literally just a vector with strings, it personally doesn't even know its own size
         * while parsing. This record ends up getting filled by the unpack records of the ResourceRecord class
         */
        struct TXT {
            std::vector<std::string> txt;

            [[nodiscard]] std::string to_string() const;
        };

        struct A {
            std::uint32_t address;

            void unpack(MemoryStream& stream);

            void pack(MemoryStream& stream) const;

            [[nodiscard]] std::string to_string() const;
        };

        /**
         * another special record, this one has a variable size, it ends up getting filled by the unpack records of
         * the ResourceRecord class. This is because the ResourceRecord class would know how long the bitmap is.
         */
        struct WKS {
            std::int32_t address;
            std::int8_t protocol;
            std::vector<bool> map;

            [[nodiscard]] std::string to_string() const;
        };
    }

    enum class QTYPE {
        A=1,
        NS=2,
        MD=3,
        MF=4,
        CNAME=5,
        SOA=6,
        MB=7,
        MG=8,
        MR=9,
        NUL=10,
        WKS=11,
        PTR=12,
        HINFO=13,
        MINFO=14,
        MX=15,
        TXT=16,

        AXFR=252,
        MAILB=253,
        MAILA=254,
        STAR=255,
    };

    enum class QCLASS {
        IN=1,
        CS=2,
        CH=3,
        HS=4,

        STAR=255,
    };

    struct ResourceRecord {
        Domain name;

        QTYPE type;

        QCLASS clazz;

        std::int32_t ttl;
        std::uint16_t rdlength;

        std::variant<
            ResourceRecordInner::A,
            ResourceRecordInner::DOMAIN_RECORD,
            ResourceRecordInner::HINFO,
            ResourceRecordInner::MX,
            ResourceRecordInner::MINFO,
            ResourceRecordInner::TXT,
            ResourceRecordInner::SOA,
            ResourceRecordInner::WKS,
            ResourceRecordInner::NUL,
            std::monostate
        > rdata;

        void unpack(MemoryStream& stream);

        void pack(MemoryStream& stream) const;

        [[nodiscard]] static std::string to_string(QTYPE type);

        [[nodiscard]] static std::string to_string(QCLASS clazz);

        [[nodiscard]] std::string to_string() const;
    };

    enum class OPCODE {
        QUERY=0,
        IQUERY=1,
        STATUS=2,
    };

    enum class RCODE {
        NONE=0,
        FMTERR=1,
        SERVERR=2,
        NAMEERR=3,
        NOIMPL=4,
        REFUSED=5,
    };

    struct MessageHeader {
        std::uint16_t id;

        bool qr;
        OPCODE opcode;
        bool aa;
        bool tc;
        bool rd;
        bool ra;
        std::int8_t z;
        RCODE rcode;

        std::uint16_t qdcount;
        std::uint16_t ancount;
        std::uint16_t nscount;
        std::uint16_t arcount;

        void unpack(MemoryStream& stream);

        void pack(MemoryStream& stream) const;

        [[nodiscard]] static std::string to_string(OPCODE code);

        [[nodiscard]] static std::string to_string(RCODE code);

        [[nodiscard]] std::string to_string() const;
    };

    struct MessageQuestion {
        Domain qname;
        QTYPE type;
        QCLASS clazz;

        void unpack(MemoryStream& stream);

        void pack(MemoryStream& stream) const;

        [[nodiscard]] std::string to_string() const;
    };

    struct Message {
        MessageHeader header;

        std::vector<MessageQuestion> questions;
        std::vector<ResourceRecord> answers, authorities, additionals;

        void unpack(MemoryStream& stream);

        void pack(MemoryStream& stream) const;

        [[nodiscard]] std::string to_string() const;
    };

    [[nodiscard]] std::string addr_to_string(std::uint32_t address);

    /**
     * RFC 1035 compliant DNS resolver class
     */
    class Resolver {
        SocketManager socketMGR;

        static std::uint16_t queryID;

        std::optional<in_addr> get_from_msg(const Message &msg, const Domain& domain);

        std::optional<in_addr> query_dns(std::uint32_t addr, const Domain& domain);

    public:
        std::optional<in_addr> query_dns(std::uint32_t addr, const std::string& domain);

        std::optional<in_addr> query_dns(const std::string& domain);
    };

    extern Resolver default_resolver;
}

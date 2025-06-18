#include <dwhbll/network/dns/dns.h>

#include <format>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <utility>

#include <dwhbll/console/Logging.h>

namespace dwhbll::network::dns {
    Resolver default_resolver;

    std::uint8_t MemoryStream::get_uint8() {
        return data[current_head++];
    }

    std::uint16_t MemoryStream::get_uint16() {
        const std::uint8_t high = get_uint8();
        const std::uint8_t low = get_uint8();
        return (high << 8) | low;
    }

    std::uint32_t MemoryStream::get_uint32() {
        const std::uint16_t high = get_uint16();
        const std::uint16_t low = get_uint16();
        return (high << 16) | low;
    }

    void MemoryStream::write_uint8(const std::uint8_t value) {
        data.push_back(value);
    }

    void MemoryStream::write_uint16(std::uint16_t value) {
        data.push_back(value >> 8);
        data.push_back(value & 0xFF);
    }

    void MemoryStream::write_uint32(std::uint32_t value) {
        write_uint16(value >> 16);
        write_uint16(value & 0xFFFF);
    }

    std::expected<std::string, Domain::parse_errors> Domain::try_parse_label(const std::string &domain, std::size_t& start) {
        std::string result;
        char c;

        while (c = domain[start], std::isalnum(c) || c == '-') {
            start++;
            result += c;
        }

        if (!result.empty()) {
            // sanity check
            if (!std::isalpha(result.front()))
                return std::unexpected(parse_errors::INVALID_LABEL_START);
            if (!std::isalnum(result.back()))
                return std::unexpected(parse_errors::INVALID_LABEL_END);
        }

        return result;
    }

    Domain Domain::parse(const std::string &domain) {
        Domain result;

        if (domain.empty() || domain == " ") {
            // empty domain, there is a trailing label.
            result.labels = {""};
            return result;
        }

        // parse the subdomain
        std::size_t ind = 0;
        std::expected<std::string, parse_errors> subd;
        while (subd = try_parse_label(domain, ind), subd.has_value()) {
            if (subd.value().empty()) {
                // last trailing label
                result.labels.emplace_back("");
                break;
            }

            result.labels.emplace_back(subd.value());
            if (domain[ind] != '.') {
                break; // done parsing
            }
            ind++;
        }
        if (!subd.has_value()) {
            console::error("[PARSE ERROR] {}", subd.error() == parse_errors::INVALID_LABEL_START ? "invalid label start" : "invalid label end");
        } else {
            console::trace("[PARSE RESULTS] {}", std::accumulate(std::begin(result.labels) + 1, std::end(result.labels), result.labels[0], [](const auto& a, const auto& b) {
                return a + "." + b;
            }) + ".");
        }

        return result;
    }

    Domain Domain::unpack(MemoryStream &stream) {
        Domain result;

        bool inCompressed = false;
        auto head = stream.current_head;

        while (stream.data[head] != 0) {
            const std::uint16_t size = static_cast<unsigned char>(stream.data[head++]);
            if ((size & 0xC0) == 0xC0) {
                // if not already reading compressed data
                if (!inCompressed) {
                    // write back the head
                    stream.current_head = head + 1;
                }

                // compressed
                inCompressed = true;

                std::uint16_t sbyte = static_cast<std::uint16_t>(stream.data[head++]) & 0xFF;

                head = size & 0x3F;
                head <<= 8;
                head &= 0xFF00;
                head |= sbyte;
                continue;
            }
            result.labels.emplace_back("");
            auto& label = result.labels.back();
            label.reserve(size);
            for (int i = 0; i < size; i++) {
                label.push_back(stream.data[head++]);
            }
        }
        if (!inCompressed) {
            head++; // eat the null byte that is the trailing label
            stream.current_head = head; // write the head back
        }

        result.labels.emplace_back(""); // add the empty label.
        return result;
    }

    void Domain::pack(const Domain &domain, MemoryStream &stream) {
        for (auto& label : domain.labels) {
            if (label.empty()) {
                // null label reached
                stream.data.push_back(0);
                break;
            }
            if (label.size() > 63) {
                console::error("Domain label was too long to fit in 63 char limit! (was {} chars)", label.size());
                console::trace("Truncating label to 63 chars from {}", label.size());

                // make a copy of the label then truncate
                std::string lab = label;
                lab.resize(63);
                stream.data.push_back(static_cast<char>(lab.size()));
                for (const auto c : lab) {
                    stream.data.push_back(c);
                }
            } else {
                stream.data.push_back(static_cast<char>(label.size()));
                for (const auto c : label) {
                    stream.data.push_back(c);
                }
            }
        }
    }

    std::string Domain::to_string() const {
        std::string result;
        for (const auto& label : labels) {
            if (!label.empty())
                result += label + ".";
        }
        return result;
    }

    std::string ResourceRecord::to_string(QTYPE type) {
        switch (type) {
            case QTYPE::A:
                return "A";
            case QTYPE::NS:
                return "NS";
            case QTYPE::MD:
                return "MD";
            case QTYPE::MF:
                return "MF";
            case QTYPE::CNAME:
                return "CNAME";
            case QTYPE::SOA:
                return "SOA";
            case QTYPE::MB:
                return "MB";
            case QTYPE::MG:
                return "MG";
            case QTYPE::MR:
                return "MR";
            case QTYPE::NUL:
                return "NUL";
            case QTYPE::WKS:
                return "WKS";
            case QTYPE::PTR:
                return "PTR";
            case QTYPE::HINFO:
                return "HINFO";
            case QTYPE::MINFO:
                return "MINFO";
            case QTYPE::MX:
                return "MX";
            case QTYPE::TXT:
                return "TXT";
            case QTYPE::AXFR:
                return "AXFR";
            case QTYPE::MAILB:
                return "MAILB";
            case QTYPE::MAILA:
                return "MAILA";
            case QTYPE::STAR:
                return "*";
            default:
                return std::format("UNKNOWN({:#x})", static_cast<std::uint16_t>(type));
        }
    }

    std::string ResourceRecord::to_string(QCLASS clazz) {
        switch (clazz) {
            case QCLASS::IN:
                return "IN";
            case QCLASS::CS:
                return "CS";
            case QCLASS::CH:
                return "CH";
            case QCLASS::HS:
                return "HS";
            case QCLASS::STAR:
                return "*";
            default:
                return std::format("UNKNOWN({:#x})", static_cast<std::uint16_t>(clazz));
        }
    }

    void ResourceRecord::unpack(MemoryStream &stream) {
        name = Domain::unpack(stream);
        type = static_cast<QTYPE>(stream.get_uint16());
        clazz = static_cast<QCLASS>(stream.get_uint16());
        ttl = stream.get_uint32();
        rdlength = stream.get_uint16();

        switch (type) {
            case QTYPE::CNAME:
            case QTYPE::MB:
            case QTYPE::MD:
            case QTYPE::MF:
            case QTYPE::MG:
            case QTYPE::MR:
            case QTYPE::PTR:
            case QTYPE::NS: {
                ResourceRecordInner::DOMAIN_RECORD record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::A: {
                ResourceRecordInner::A record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::SOA: {
                ResourceRecordInner::SOA record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::HINFO: {
                ResourceRecordInner::HINFO record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::MINFO: {
                ResourceRecordInner::MINFO record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::MX: {
                ResourceRecordInner::MX record;
                record.unpack(stream);
                rdata = record;
                return;
            }
            case QTYPE::NUL: {
                ResourceRecordInner::NUL record;
                record.data.reserve(rdlength);
                for (int i = 0; i < rdlength; i++)
                    record.data.push_back(stream.get_uint8());
                rdata = record;
                return;
            }
            case QTYPE::TXT: {
                ResourceRecordInner::TXT record;
                std::size_t eaten = 0;
                while (eaten < rdlength) {
                    std::string r;
                    std::uint8_t size = stream.get_uint8();
                    r.reserve(size);
                    eaten += size;
                    for (int i = 0; i < size; i++)
                        r.push_back(stream.get_uint8());
                    record.txt.emplace_back(r);
                }
                rdata = record;
                return;
            }
            case QTYPE::WKS: {
                ResourceRecordInner::WKS record;
                record.address = stream.get_uint32();
                record.protocol = stream.get_uint8();
                std::size_t eaten = 0;
                record.map.reserve(8 * (rdlength - 5));
                while (eaten < rdlength) {
                    std::uint8_t data = stream.get_uint8();
                    eaten++;
                    for (int i = 0; i < 8; i++) {
                        record.map.emplace_back(data & 0x80);
                        data <<= 1;
                    }
                }
                rdata = record;
                return;
            }
            default:
                console::warn("Unknown DNS record type of {:#x}", static_cast<std::uint16_t>(type));
                for (int i = 0; i < rdlength; i++)
                    stream.get_uint8();
                rdata = std::monostate{};
                return;
        }
    }

    void ResourceRecord::pack(MemoryStream &stream) const {
        console::fatal("packing of ResourceRecords not currently supported!!!");
        std::terminate();
    }

    std::string ResourceRecord::to_string() const {
        std::string result = "ResourceRecord: {\n  NAME: " + name.to_string() + ",\n";
        result += std::format("  TYPE: {},\n", to_string(type));
        result += std::format("  CLASS: {},\n", to_string(clazz));
        result += std::format("  TTL: {},\n", ttl);
        result += std::format("  RDLENGTH: {},\n  ", rdlength);
        switch (type) {
            case QTYPE::CNAME:
            case QTYPE::MB:
            case QTYPE::MD:
            case QTYPE::MF:
            case QTYPE::MG:
            case QTYPE::MR:
            case QTYPE::PTR:
            case QTYPE::NS: {
                const auto& rec = std::get<ResourceRecordInner::DOMAIN_RECORD>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::A: {
                const auto& rec = std::get<ResourceRecordInner::A>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::SOA: {
                const auto& rec = std::get<ResourceRecordInner::SOA>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::NUL: {
                const auto& rec = std::get<ResourceRecordInner::NUL>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::WKS: {
                const auto& rec = std::get<ResourceRecordInner::WKS>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::HINFO: {
                const auto& rec = std::get<ResourceRecordInner::HINFO>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::MINFO: {
                const auto& rec = std::get<ResourceRecordInner::MINFO>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::MX: {
                const auto& rec = std::get<ResourceRecordInner::MX>(rdata);
                result += rec.to_string();
                break;
            }
            case QTYPE::TXT: {
                const auto& rec = std::get<ResourceRecordInner::TXT>(rdata);
                result += rec.to_string();
                break;
            }
            default:
                break;
        }
        result += "\n}";
        return result;
    }

    void MessageHeader::unpack(MemoryStream &stream) {
        id = stream.get_uint16();

        // unpack the next byte
        std::uint16_t b2 = stream.get_uint16();
        qr = b2 >> 15 & 0x1;
        opcode = static_cast<OPCODE>(b2 >> 11 & 0xF);
        aa = b2 >> 10 & 0x1;
        tc = b2 >> 9 & 0x1;
        rd = b2 >> 8 & 0x1;
        ra = b2 >> 7 & 0x1;
        z = b2 >> 4 & 0x7;
        rcode = static_cast<RCODE>(b2 & 0xF);

        qdcount = stream.get_uint16();
        ancount = stream.get_uint16();
        nscount = stream.get_uint16();
        arcount = stream.get_uint16();
    }

    void MessageHeader::pack(MemoryStream &stream) const {
        stream.write_uint16(id);

        // package the next byte
        std::uint16_t b2 = static_cast<std::uint16_t>(qr) << 15;
        b2 |= static_cast<std::uint16_t>(opcode) << 11;
        b2 |= static_cast<std::uint16_t>(aa) << 10;
        b2 |= static_cast<std::uint16_t>(tc) << 9;
        b2 |= static_cast<std::uint16_t>(rd) << 8;
        b2 |= static_cast<std::uint16_t>(ra) << 7;
        b2 |= static_cast<std::uint16_t>(z) << 4;
        b2 |= static_cast<std::uint16_t>(rcode);

        stream.write_uint16(b2);
        stream.write_uint16(qdcount);
        stream.write_uint16(ancount);
        stream.write_uint16(nscount);
        stream.write_uint16(arcount);
    }

    std::string MessageHeader::to_string(OPCODE code) {
        switch (code) {
            case OPCODE::QUERY:
                return "QUERY";
            case OPCODE::IQUERY:
                return "IQUERY";
            case OPCODE::STATUS:
                return "STATUS";
            default:
                return std::format("UNKNOWN({:#x})", static_cast<std::uint8_t>(code));
        }
    }

    std::string MessageHeader::to_string(RCODE code) {
        switch (code) {
            case RCODE::NONE:
                return "NONE";
            case RCODE::FMTERR:
                return "FMTERR";
            case RCODE::SERVERR:
                return "SERVERR";
            case RCODE::NAMEERR:
                return "NAMEERR";
            case RCODE::NOIMPL:
                return "NOIMPL";
            case RCODE::REFUSED:
                return "REFUSED";
            default:
                return std::format("UNKNOWN({:#x})", static_cast<std::uint8_t>(code));
        }
    }

    std::string MessageHeader::to_string() const {
        std::string result = "MessageHeader: {\n";

        result += std::format("  ID: {:#x},\n  QR: {},\n", id, qr ? "YES" : "NO");
        result += std::format("  OPCODE: {},\n  AA: {},\n", to_string(opcode), aa ? "YES" : "NO");
        result += std::format("  TC: {},\n  RD: {},\n", tc ? "YES" : "NO", rd ? "YES" : "NO");
        result += std::format("  RA: {},\n  Z: {:#x},\n", ra ? "YES" : "NO", z);
        result += std::format("  RCODE: {},\n", to_string(rcode));

        result += std::format("  Questions: {},\n", qdcount);
        result += std::format("  Answers: {},\n", ancount);
        result += std::format("  Authorities: {},\n", nscount);
        result += std::format("  Additionals: {},\n", arcount);

        result += "}\n";
        return result;
    }

    void MessageQuestion::unpack(MemoryStream &stream) {
        qname = Domain::unpack(stream);
        type = static_cast<QTYPE>(stream.get_uint16());
        clazz = static_cast<QCLASS>(stream.get_uint16());
    }

    void MessageQuestion::pack(MemoryStream &stream) const {
        Domain::pack(qname, stream);
        stream.write_uint16(static_cast<std::uint16_t>(type));
        stream.write_uint16(static_cast<std::uint16_t>(clazz));
    }

    std::string MessageQuestion::to_string() const {
        return std::format("Question: {{ NAME: {}, TYPE: {}, CLASS: {} }}", qname.to_string(), ResourceRecord::to_string(type), ResourceRecord::to_string(clazz));
    }

    void Message::unpack(MemoryStream &stream) {
        header.unpack(stream);
        questions.resize(header.qdcount);
        for (int i = 0; i < header.qdcount; i++)
            questions[i].unpack(stream);
        answers.resize(header.ancount);
        for (int i = 0; i < header.ancount; i++)
            answers[i].unpack(stream);
        authorities.resize(header.nscount);
        for (int i = 0; i < header.nscount; i++)
            authorities[i].unpack(stream);
        additionals.resize(header.arcount);
        for (int i = 0; i < header.arcount; i++)
            additionals[i].unpack(stream);
    }

    void Message::pack(MemoryStream &stream) const {
        header.pack(stream);
        for (int i = 0; i < header.qdcount; i++)
            questions[i].pack(stream);
        for (int i = 0; i < header.ancount; i++)
            answers[i].pack(stream);
        for (int i = 0; i < header.nscount; i++)
            authorities[i].pack(stream);
        for (int i = 0; i < header.arcount; i++)
            additionals[i].pack(stream);
    }

    std::string Message::to_string() const {
        std::string result = "Message: {\n";

        result += header.to_string();

        result += "QUESTIONS: {\n";
        for (const auto [i, entry] : questions | std::views::enumerate) {
            result += std::format("[{}]: {},\n", i, entry.to_string());
        }
        result += "},\n";

        result += "ANSWERS: {\n";
        for (const auto [i, entry] : answers | std::views::enumerate) {
            result += std::format("[{}]: {},\n", i, entry.to_string());
        }
        result += "},\n";

        result += "AUTHORITIES: {\n";
        for (const auto [i, entry] : authorities | std::views::enumerate) {
            result += std::format("[{}]: {},\n", i, entry.to_string());
        }
        result += "},\n";

        result += "ADDITIONALS: {\n";
        for (const auto [i, entry] : additionals | std::views::enumerate) {
            result += std::format("[{}]: {},\n", i, entry.to_string());
        }
        result += "},\n}";

        return result;
    }

    std::string addr_to_string(const std::uint32_t address) {
        std::uint8_t octet1 = (address >> 24) & 0xFF;
        std::uint8_t octet2 = (address >> 16) & 0xFF;
        std::uint8_t octet3 = (address >> 8) & 0xFF;
        std::uint8_t octet4 = address & 0xFF;

        return std::format("A: {}.{}.{}.{}", octet4, octet3, octet2, octet1);
    }

    std::uint16_t Resolver::queryID = 0;

    std::optional<in_addr> Resolver::get_from_msg(const Message &msg, const Domain &domain) {
        for (int i = 0; i < msg.header.ancount; i++) {
            auto& an = msg.answers[i];
            if (an.name == domain) {
                switch (an.type) {
                    case QTYPE::A: {
                        const auto& addr = htonl(std::get<ResourceRecordInner::A>(an.rdata).address);
                        return in_addr{addr};
                    }
                    case QTYPE::NS:
                        break;
                    case QTYPE::MD:
                        break;
                    case QTYPE::MF:
                        break;
                    case QTYPE::CNAME:
                        return get_from_msg(msg, std::get<ResourceRecordInner::DOMAIN_RECORD>(an.rdata).name);
                    case QTYPE::SOA:
                        break;
                    case QTYPE::MB:
                        break;
                    case QTYPE::MG:
                        break;
                    case QTYPE::MR:
                        break;
                    case QTYPE::NUL:
                        break;
                    case QTYPE::WKS:
                        break;
                    case QTYPE::PTR:
                        break;
                    case QTYPE::HINFO:
                        break;
                    case QTYPE::MINFO:
                        break;
                    case QTYPE::MX:
                        break;
                    case QTYPE::TXT:
                        break;
                    case QTYPE::AXFR:
                        break;
                    case QTYPE::MAILB:
                        break;
                    case QTYPE::MAILA:
                        break;
                    case QTYPE::STAR:
                        break;
                }
            }
        }
        return std::nullopt;
    }

    std::optional<in_addr> Resolver::query_dns(std::uint32_t addr, const Domain &domain) {
        MemoryStream stream;

        Message msg {
            {
                queryID++,
                false,
                OPCODE::QUERY,
                false,
                false,
                false,
                false,
                0,
                RCODE::NONE,
                1,
                0,
                0,
                0
            }, // header
            {
                {
                    domain,
                    QTYPE::A,
                    QCLASS::IN
                }
            }, // questions
            {}, {}, {} // no responses in a query
        };

        msg.pack(stream);

        Message result;
        if (stream.data.size() <= 512) {
            // will use UDP
            Socket* socket = socketMGR.getIPv4UDPSocket(in_addr{addr}, 53);
            stream.data.make_cont();
            stream.data.data().resize(stream.data.size());

            socket->send(stream.data.data());

            socket->wait();

            MemoryStream recvStream;
            recvStream.data.resize(512);
            std::size_t bufSize = socket->recv(recvStream.data.data()); // get the rest of the buffer
            recvStream.data.used(bufSize);

            result.unpack(recvStream);

            socketMGR.offer(socket);

            if (result.header.tc) {
                console::trace("Result got truncated. Trying with TCP.");
            }
        }

        if (stream.data.size() > 512 || result.header.tc) {
            // must use TCP
            Socket* socket = socketMGR.getIPv4TCPSocket(in_addr{addr}, 53);
            std::uint16_t size = stream.data.size();
            stream.data.push_front(size & 0xFF);
            stream.data.push_front((size >> 0x8) & 0xFF);
            stream.data.make_cont();
            stream.data.data().resize(stream.data.size());

            socket->send(stream.data.data());

            socket->wait();

            std::vector<char> buf(2);
            socket->recv(buf); // get the length
            size = (static_cast<std::uint16_t>(buf[0]) << 8 & 0xFF00) | (static_cast<std::uint16_t>(buf[1]) & 0xFF);

            MemoryStream recvStream;
            recvStream.data.resize(size);
            recvStream.data.used(size);
            socket->recv(recvStream.data.data()); // get the rest of the buffer

            result.unpack(recvStream);

            socketMGR.offer(socket);
        }

        if (result.header.aa) {
            // this is the authoritative NS for the job.
            console::trace("found an authoritative NS for the job!");
            return get_from_msg(result, domain);
        } else {
            // recurse
            if (result.header.nscount == 0)
                return std::nullopt;
            std::optional<in_addr> resultAddr;
            for (const auto& authority : result.authorities) {
                if (authority.type == QTYPE::NS) {
                    auto& target = std::get<ResourceRecordInner::DOMAIN_RECORD>(authority.rdata);

                    // search for the ip of the given authoritative NS.
                    bool has = false;
                    for (const auto& additional : result.additionals) {
                        if (additional.type == QTYPE::A && target.name == additional.name) {
                            has = true;
                            console::trace("querying the next NS: {}", additional.name.to_string());
                            resultAddr = query_dns({htonl(std::get<ResourceRecordInner::A>(additional.rdata).address)}, domain);
                            if (resultAddr.has_value())
                                return resultAddr;
                        }
                    }

                    // if (!has) {
                    //     // make another request for the authoritative NS
                    //     std::optional<in_addr> ns = query_dns(target.name.to_string());
                    //     if (!ns.has_value())
                    //         continue;
                    //     // query the new NS with our data
                    //     if (std::optional<in_addr> r = query_dns(htonl(ns.value().s_addr), domain); r.has_value())
                    //         return r;
                    // }
                }
            }
            std::cerr << result.to_string() << std::endl;
        }

        // TODO: handle result.
        return std::nullopt;
    }

    std::optional<in_addr> Resolver::query_dns(std::uint32_t addr, const std::string &domain) {
        Domain d = Domain::parse(domain);
        return query_dns(addr, d);
    }

    std::optional<in_addr> Resolver::query_dns(const std::string &domain) {
        // query one of the root servers to start with.
        std::optional<in_addr> result;
        for (std::uint32_t target : root_servers) {
            console::trace("sending a dns query to {} for {}", addr_to_string(target), domain);
            result = query_dns(target, domain);
            if (result.has_value())
                return result;
        }
        return std::nullopt;
    }

    namespace ResourceRecordInner {
        void DOMAIN_RECORD::unpack(MemoryStream &stream) {
            name = Domain::unpack(stream);
        }

        void DOMAIN_RECORD::pack(MemoryStream &stream) const {
            Domain::pack(name, stream);
        }

        std::string DOMAIN_RECORD::to_string() const {
            return std::format("DOMAINRECORD: {}", name.to_string());
        }

        void HINFO::unpack(MemoryStream &stream) {
            std::size_t size = stream.get_uint8();
            for (int i = 0; i < size; i++)
                cpu += stream.get_uint8();
            size = stream.get_uint8();
            for (int i = 0; i < size; i++)
                os += stream.get_uint8();
        }

        void HINFO::pack(MemoryStream &stream) const {
            stream.write_uint8(cpu.size());
            for (const char c : cpu)
                stream.write_uint8(c);
            stream.write_uint8(os.size());
            for (const char c : os)
                stream.write_uint8(c);
        }

        std::string HINFO::to_string() const {
            return std::format("SERVER INFO: {{ CPU: {}, OS: {} }}", cpu, os);
        }

        void MINFO::unpack(MemoryStream &stream) {
            rmailbx = Domain::unpack(stream);
            emailbx = Domain::unpack(stream);
        }

        void MINFO::pack(MemoryStream &stream) const {
            Domain::pack(rmailbx, stream);
            Domain::pack(emailbx, stream);
        }

        std::string MINFO::to_string() const {
            return std::format("MINFO: {{ RMAILBX: {}, EMAILBX: {} }}", rmailbx.to_string(), emailbx.to_string());
        }

        void MX::unpack(MemoryStream &stream) {
            preference = stream.get_uint16();
            exchange = Domain::unpack(stream);
        }

        void MX::pack(MemoryStream &stream) const {
            stream.write_uint16(preference);
            Domain::pack(exchange, stream);
        }

        std::string MX::to_string() const {
            return std::format("MX: {{ preference: {:#x}, exchange: {} }}", preference, exchange.to_string());
        }

        std::string NUL::to_string() const {
            std::string result = "NUL: {\n";

            int i;
            for (i = 0; i < data.size() / 16; i++) {
                for (int j = 0; j < 16; j++)
                    result += std::format("{:#x}", data[i * 16 + j]) + ",";
                result += "\n";
            }

            // outstanding bytes
            for (int j = 0; j < data.size() % 16; j++)
                result += std::format("{:#x}", data[i * 16 + j]) + ",";
            result += "\n}";
            return result;
        }

        void SOA::unpack(MemoryStream &stream) {
            mname = Domain::unpack(stream);
            rname = Domain::unpack(stream);
            serial = stream.get_uint32();
            refresh = stream.get_uint32();
            retry = stream.get_uint32();
            expire = stream.get_uint32();
            minimum = stream.get_uint32();
        }

        void SOA::pack(MemoryStream &stream) const {
            Domain::pack(mname, stream);
            Domain::pack(rname, stream);
            stream.write_uint32(serial);
            stream.write_uint32(refresh);
            stream.write_uint32(retry);
            stream.write_uint32(expire);
            stream.write_uint32(minimum);
        }

        std::string SOA::to_string() const {
            return "TODO: SOA RECORD";
        }

        std::string TXT::to_string() const {
            std::string result = "TXT: {\n";

            for (const auto& e : txt)
                result += e + ",\n";

            result += "\n}";

            return result;
        }

        void A::unpack(MemoryStream &stream) {
            address = stream.get_uint32();
        }

        void A::pack(MemoryStream &stream) const {
            stream.write_uint32(address);
        }

        std::string A::to_string() const {
            std::uint8_t octet1 = (address >> 24) & 0xFF;
            std::uint8_t octet2 = (address >> 16) & 0xFF;
            std::uint8_t octet3 = (address >> 8) & 0xFF;
            std::uint8_t octet4 = address & 0xFF;

            return std::format("A: {}.{}.{}.{}", octet1, octet2, octet3, octet4);
        }

        std::string WKS::to_string() const {
            return "TODO: WKS RECORD";
        }
    }
}

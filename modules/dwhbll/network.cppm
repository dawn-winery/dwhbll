module;

#include <dwhbll/network/address.h>
#include <dwhbll/network/buffered_socket.h>
#include <dwhbll/network/socket_manager.h>
#include <dwhbll/network/http.h>
#include <dwhbll/network/http_server.h>

#include <dwhbll/network/dns/dns.h>

#include <dwhbll/network/http/methods.h>
#include <dwhbll/network/http/versions.h>

export module dwhbll.network;

export namespace dwhbll::network {
    using dwhbll::network::address;
    using dwhbll::network::inbound_network_buffer;
    using dwhbll::network::outbound_network_buffer;
    using dwhbll::network::buffered_socket;
    using dwhbll::network::Socket;
    using dwhbll::network::SocketManager;
    using dwhbll::network::http_request;
    using dwhbll::network::http_response;
    using dwhbll::network::HTTP;

    namespace http_server {
        using dwhbll::network::http_server::Server;
    }

    namespace dns {
        using dwhbll::network::dns::root_servers;
        using dwhbll::network::dns::query_dns;
        using dwhbll::network::dns::MemoryStream;
        using dwhbll::network::dns::Domain;
        namespace ResourceRecordInner {
            using dwhbll::network::dns::ResourceRecordInner::A;
            using dwhbll::network::dns::ResourceRecordInner::DOMAIN_RECORD;
            using dwhbll::network::dns::ResourceRecordInner::HINFO;
            using dwhbll::network::dns::ResourceRecordInner::MINFO;
            using dwhbll::network::dns::ResourceRecordInner::MX;
            using dwhbll::network::dns::ResourceRecordInner::NUL;
            using dwhbll::network::dns::ResourceRecordInner::SOA;
            using dwhbll::network::dns::ResourceRecordInner::TXT;
            using dwhbll::network::dns::ResourceRecordInner::WKS;
        }
        using dwhbll::network::dns::QTYPE;
        using dwhbll::network::dns::QCLASS;
        using dwhbll::network::dns::ResourceRecord;
        using dwhbll::network::dns::OPCODE;
        using dwhbll::network::dns::RCODE;
        using dwhbll::network::dns::MessageHeader;
        using dwhbll::network::dns::MessageQuestion;
        using dwhbll::network::dns::Message;
        using dwhbll::network::dns::addr_to_string;
        using dwhbll::network::dns::Resolver;
        using dwhbll::network::dns::default_resolver;
    }

    namespace conv {
        using dwhbll::network::conv::make_ipv4;
    }

    namespace http {
        using dwhbll::network::http::HTTP_METHOD;
        using dwhbll::network::http::HTTP_VERSION;
    }
}

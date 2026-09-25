import std;
import dwhbll.network;
import dwhbll.sanify;

namespace dwhbll::network {
    address::address() : type(EMPTY) {}

    address::address(std::string host, const u16 port): type(DOMAIN), host(std::move(host)),
                                                                  port(port) {}

    address::address(std::array<u8, 4> host, u16 port): type(IPV4), host(host),
                                                                  port(port) {}

    address::address(std::array<u16, 8> host, u16 port): type(IPV6), host(host),
                                                                  port(port) {}

    address::address(const address &other) = default;

    address::address(address &&other) noexcept: type(other.type),
                                                host(std::move(other.host)),
                                                port(other.port) {
    }

    address & address::operator=(const address &other) {
        if (this == &other)
            return *this;
        type = other.type;
        host = other.host;
        port = other.port;
        return *this;
    }

    address & address::operator=(address &&other) noexcept {
        if (this == &other)
            return *this;
        type = other.type;
        host = std::move(other.host);
        port = other.port;
        return *this;
    }
}

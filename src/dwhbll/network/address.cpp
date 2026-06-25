#include <dwhbll/network/address.h>

namespace dwhbll::network {
    address::address() : type(EMPTY) {}

    address::address(std::string host, const std::uint16_t port): type(DOMAIN), host(std::move(host)),
                                                                  port(port) {}

    address::address(std::array<std::uint8_t, 4> host, std::uint16_t port): type(IPV4), host(host),
                                                                  port(port) {}

    address::address(std::array<std::uint16_t, 8> host, std::uint16_t port): type(IPV6), host(host),
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

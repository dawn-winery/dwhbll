#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace dwhbll::network {
    namespace conv {
        constexpr uint32_t make_ipv4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
            return (a << 24 & 0xFF000000) | (b << 16 & 0xFF0000) | (c << 8 & 0xFF00) |
                   (d & 0xFF);
        }
        constexpr uint32_t make_ipv4(const std::array<std::uint8_t, 4>& addr) {
            return (addr[0] << 24 & 0xFF000000) | (addr[1] << 16 & 0xFF0000) | (addr[2] << 8 & 0xFF00) |
                   (addr[3] & 0xFF);
        }
    }

    struct address {
        enum TYPE {
            EMPTY,
            DOMAIN,
            IPV4,
            IPV6
        } type;
        std::variant<std::monostate, std::string, std::array<std::uint8_t, 4>, std::array<std::uint16_t, 8>> host;
        std::uint16_t port;

        address();
        address(std::string host, std::uint16_t port);
        address(std::array<std::uint8_t, 4> host, std::uint16_t port);
        address(std::array<std::uint16_t, 8> host, std::uint16_t port);

        address(const address &other);

        address(address &&other) noexcept;

        address & operator=(const address &other);

        address & operator=(address &&other) noexcept;
    };
}

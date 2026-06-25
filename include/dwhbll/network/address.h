#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace dwhbll::network {
    struct address {
        enum TYPE {
            DOMAIN,
            IPV4,
            IPV6
        } type;
        std::variant<std::string, std::array<std::uint8_t, 4>, std::array<std::uint16_t, 8>> host;
        std::uint16_t port;

        address(std::string host, std::uint16_t port);
        address(std::array<std::uint8_t, 4> host, std::uint16_t port);
        address(std::array<std::uint16_t, 8> host, std::uint16_t port);

        address(const address &other);

        address(address &&other) noexcept;

        address & operator=(const address &other);

        address & operator=(address &&other) noexcept;
    };
}

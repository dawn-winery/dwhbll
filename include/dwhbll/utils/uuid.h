#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace dwhbll::utils {
    class UUID {
        std::array<std::uint8_t, 16> _data{};

        static const UUID _nil_uuid;
        static const UUID _max_uuid;

    public:
        UUID();

        explicit UUID(const std::array<std::uint8_t, 16>& data);

        [[nodiscard]] std::string to_string() const;
    };
}

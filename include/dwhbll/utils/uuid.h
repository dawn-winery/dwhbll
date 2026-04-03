#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dwhbll::utils {
    class UUID {
        std::array<std::uint8_t, 16> _data{};

        static const UUID _nil_uuid;
        static const UUID _max_uuid;

    public:
        UUID();

        explicit UUID(const std::array<std::uint8_t, 16>& data);

        UUID(const UUID &other);

        UUID(UUID &&other) noexcept;

        UUID & operator=(const UUID &other);

        UUID & operator=(UUID &&other) noexcept;

        [[nodiscard]] std::string to_string() const;

        friend bool operator==(const UUID &lhs, const UUID &rhs);

        friend bool operator!=(const UUID &lhs, const UUID &rhs);
    };
}

#pragma once

#include <cstdint>
#include <span>

namespace dwhbll::cryptography {
    class ihash {
    public:
        virtual ~ihash() = default;

        virtual void update(const std::span<const std::uint8_t> &in) = 0;
        virtual void finalize(std::span<std::uint8_t> output) = 0;
        virtual void reset() = 0;

        [[nodiscard]] virtual size_t digest_size() const = 0;
        [[nodiscard]] virtual size_t block_size() const = 0;

        void operator()(const std::span<const std::uint8_t> &in) {
            update(in);
        }
    };
}

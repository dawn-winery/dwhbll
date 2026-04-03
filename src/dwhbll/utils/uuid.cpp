#include <dwhbll/utils/uuid.h>

#include <format>

namespace dwhbll::utils {
    const UUID UUID::_max_uuid = UUID{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    const UUID UUID::_nil_uuid = UUID{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

    UUID::UUID() : _data(_nil_uuid._data) {}

    UUID::UUID(const std::array<std::uint8_t, 16> &data) : _data(data) {}

    UUID::UUID(const UUID &other): _data(other._data) {
    }

    UUID::UUID(UUID &&other) noexcept: _data(std::move(other._data)) {
    }

    UUID & UUID::operator=(const UUID &other) {
        if (this == &other)
            return *this;
        _data = other._data;
        return *this;
    }

    UUID & UUID::operator=(UUID &&other) noexcept {
        if (this == &other)
            return *this;
        _data = std::move(other._data);
        return *this;
    }

    std::string UUID::to_string() const {
        return std::format("{:x}{:x}{:x}{:x}-{:x}{:x}-{:x}{:x}-{:x}{:x}-{:x}{:x}{:x}{:x}{:x}{:x}",
            _data[0], _data[1], _data[2], _data[3], _data[4], _data[5], _data[6], _data[7],
            _data[8], _data[9], _data[10], _data[11], _data[12], _data[13], _data[14], _data[15]);
    }

    bool operator==(const UUID &lhs, const UUID &rhs) {
        return lhs._data == rhs._data;
    }

    bool operator!=(const UUID &lhs, const UUID &rhs) {
        return !(lhs == rhs);
    }
}

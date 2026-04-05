#include <dwhbll/math/vec4.h>

#include <dwhbll/console/debug.hpp>

namespace dwhbll::math {
    Vec4::Vec4(const std::span<const float> &values) {
        if (values.size() < 4)
            throw std::out_of_range("Size too small!");

        _w = values[0];
        _x = values[1];
        _y = values[2];
        _z = values[3];
    }

    Vec4::Vec4(const float w, const float x, const float y, const float z) : _w(w), _x(x), _y(y), _z(z) {}

    Vec4::Vec4(const float value) : _w(value), _x(value), _y(value), _z(value) {}
}

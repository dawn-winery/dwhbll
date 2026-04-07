#pragma once

#include <limits>
#include <cmath>
#include <span>

namespace dwhbll::math {
    class Vec4 {
        float _w;
        float _x;
        float _y;
        float _z;

    public:
        Vec4(const std::span<const float> &values);

        Vec4(float w, float x, float y, float z);

        Vec4(float value);

        Vec4(const Vec4 &other);

        Vec4(Vec4 &&other) noexcept;

        Vec4 & operator=(const Vec4 &other);

        Vec4 & operator=(Vec4 &&other) noexcept;

        // Vec4(const Vec2 &v2, float z, float w);

        // Vec4(const Vec3 &v3, float w);

        [[nodiscard]] constexpr float w() const {
            return _w;
        }

        [[nodiscard]] constexpr float x() const {
            return _x;
        }

        [[nodiscard]] constexpr float y() const {
            return _y;
        }

        [[nodiscard]] constexpr float z() const {
            return _z;
        }

        [[nodiscard]] constexpr static Vec4 vec_e() {
            return Vec4(std::numbers::e);
        }

        [[nodiscard]] constexpr static Vec4 vec_epsilon() {
            return Vec4(std::numeric_limits<float>::epsilon());
        }

        [[nodiscard]] constexpr static Vec4 vec_nan() {
            return Vec4(std::numeric_limits<float>::quiet_NaN());
        }

        [[nodiscard]] constexpr static Vec4 vec_neg_inf() {
            return Vec4(-std::numeric_limits<float>::infinity());
        }

        [[nodiscard]] constexpr static Vec4 vec_one() {
            return Vec4(1.f);
        }

        [[nodiscard]] constexpr static Vec4 vec_pi() {
            return Vec4(std::numbers::pi);
        }

        [[nodiscard]] constexpr static Vec4 vec_pos_inf() {
            return Vec4(std::numeric_limits<float>::infinity());
        }

        [[nodiscard]] constexpr static Vec4 vec_tau() {
            return Vec4(std::numbers::pi * 2);
        }

        [[nodiscard]] constexpr static Vec4 unit_w() {
            return Vec4{1.f, 0.f, 0.f, 0.f};
        }

        [[nodiscard]] constexpr static Vec4 unit_x() {
            return Vec4{0.f, 1.f, 0.f, 0.f};
        }

        [[nodiscard]] constexpr static Vec4 unit_y() {
            return Vec4{0.f, 0.f, 1.f, 0.f};
        }

        [[nodiscard]] constexpr static Vec4 unit_z() {
            return Vec4{0.f, 0.f, 0.f, 1.f};
        }

        [[nodiscard]] constexpr static Vec4 vec_zero() {
            return Vec4(0.f);
        }

        [[nodiscard]] constexpr static Vec4 abs(const Vec4& vec) {
            return Vec4{std::abs(vec.w()), std::abs(vec.x()), std::abs(vec.y()), std::abs(vec.z())};
        }

        [[nodiscard]] constexpr static Vec4 add(const Vec4& a, const Vec4& b) {
            return Vec4{a.w() + b.w(), a.x() + b.x(), a.y() + b.y(), a.z() + b.z()};
        }

        // TODO: finish :xdd:
    };
}

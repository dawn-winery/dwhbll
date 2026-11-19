#pragma once

#include <filesystem>

#include <dwhbll/sanify/stl_ext.h>

namespace dwhbll::files {
    class binary_file {
        uint8_t* file{};
        std::size_t len{};

        [[nodiscard]] stl_ext::errno_result<stl_ext::UNIT> load(const std::filesystem::path &path);

        void unload();

    public:
        binary_file();

        explicit binary_file(const std::filesystem::path &path);

        ~binary_file();

        binary_file(const binary_file &other) = delete;

        binary_file(binary_file &&other) noexcept;

        binary_file & operator=(const binary_file &other) = delete;

        binary_file & operator=(binary_file &&other) noexcept;

        [[nodiscard]] constexpr std::span<uint8_t> get_file() const {
            return std::span{file, file + len};
        }

        [[nodiscard]] constexpr std::size_t get_len() const {
            return len;
        }
    };
}

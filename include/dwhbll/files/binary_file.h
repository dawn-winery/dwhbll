#pragma once

#include <filesystem>

namespace dwhbll::files {
    class binary_file {
        uint8_t* file{};
        std::size_t len{};

        void load(const std::filesystem::path &path);

        void unload();

    public:
        explicit binary_file(const std::filesystem::path &path);

        ~binary_file();

        binary_file(const binary_file &other) = delete;

        binary_file(binary_file &&other) noexcept;

        binary_file & operator=(const binary_file &other) = delete;

        binary_file & operator=(binary_file &&other) noexcept;

        [[nodiscard]] constexpr uint8_t* get_file() const {
            return file;
        }

        [[nodiscard]] constexpr std::size_t get_len() const {
            return len;
        }
    };
}

#pragma once

#include <filesystem>
#include <vector>

namespace dwhbll::files::filejar {
    class file {
        std::filesystem::path _path;
        std::vector<char> _contents;

        bool _exists;

    public:
        explicit file(const std::filesystem::path &path);

        file(const file &other) = delete;

        file(file &&other) noexcept;

        file & operator=(const file &other) = delete;

        file & operator=(file &&other) noexcept;

        [[nodiscard]] constexpr bool exists() const {
            return _exists;
        }

        [[nodiscard]] constexpr std::filesystem::path path() const {
            return _path;
        }

        [[nodiscard]] constexpr const std::vector<char>& contents() const {
            return _contents;
        }
    };
}

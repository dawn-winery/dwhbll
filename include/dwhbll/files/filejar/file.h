#pragma once

#include <filesystem>
#include <vector>
#include <dwhbll/debug/panic.h>
#include <dwhbll/sanify/types.h>

#ifdef _WIN32
#error Windows builds not supported
#endif

namespace dwhbll::files::filejar {
    struct file_metadata {
        std::filesystem::path _path;
        std::filesystem::file_time_type last_write_time{};
        std::size_t len{};
        bool _exists{};

        mutable std::optional<std::uint64_t> hash{};

        [[nodiscard]] constexpr bool probably_unchanged(const file_metadata& other) const {
            if (!_exists || !other._exists)
                return false;
            return len == other.len &&
                last_write_time == other.last_write_time;
        }

        [[nodiscard]] constexpr bool operator==(const file_metadata& other) const {
            return probably_unchanged(other) && hash == other.hash;
        }
    };

    class file {
        file_metadata metadata;
        int fd{-1};
        sanify::u8* data{nullptr};

    public:
        explicit file(const std::filesystem::path &path);

        ~file();

        file(const file &other) = delete;

        file(file &&other) noexcept
            : metadata(std::move(other.metadata)),
              fd(other.fd),
              data(other.data) {
            other.fd = -1;
            other.data = nullptr;
        }

        file & operator=(const file &other) = delete;

        file & operator=(file &&other) noexcept {
            if (this == &other)
                return *this;
            metadata = std::move(other.metadata);
            fd = other.fd;
            data = other.data;
            other.fd = -1;
            other.data = nullptr;
            return *this;
        }

        void compute_hash() const;

        [[nodiscard]] bool file_unchanged_against(const file_metadata& other) const;

        [[nodiscard]] constexpr bool exists() const {
            return metadata._exists;
        }

        [[nodiscard]] constexpr std::filesystem::path path() const {
            return metadata._path;
        }

        [[nodiscard]] constexpr std::span<sanify::u8> contents() const {
            if (!metadata._exists)
                debug::panic("File doesn't exist!");
            return {data, metadata.len};
        }

        [[nodiscard]] constexpr file_metadata get_metadata() const {
            return metadata;
        }
    };
}

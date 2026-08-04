#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>

namespace dwhbll::files::filejar {
    class file;
    struct fileid;
}

template <>
struct std::hash<dwhbll::files::filejar::fileid> {
    [[nodiscard]] std::size_t operator()(const dwhbll::files::filejar::fileid &obj) const noexcept;
};

namespace dwhbll::files::filejar {
    /**
     * @brief Represents one file, requires the correct file_mgr instance to be of any use.
     */
    struct fileid {
        std::uint64_t id;

        [[nodiscard]] explicit fileid(std::uint64_t id)
            : id(id) {
        }

        fileid(const fileid &other) = default;

        fileid(fileid &&other) noexcept
            : id(other.id) {
        }

        fileid & operator=(const fileid &other) {
            if (this == &other)
                return *this;
            id = other.id;
            return *this;
        }

        fileid & operator=(fileid &&other) noexcept {
            if (this == &other)
                return *this;
            id = other.id;
            return *this;
        }

        [[nodiscard]] constexpr std::strong_ordering operator<=>(const fileid &other) const noexcept {
            return id <=> other.id;
        }

        [[nodiscard]] constexpr bool operator==(const fileid &other) const noexcept {
            return id == other.id;
        }

        [[nodiscard]] constexpr bool operator!=(const fileid &other) const noexcept {
            return id != other.id;
        }
    };

    class file_mgr {
        static std::atomic_uint64_t id_alloc;

        std::unordered_map<fileid, std::unique_ptr<file>> files;
        std::unordered_map<std::filesystem::path, fileid> file_ids;

        std::unique_ptr<file>& get(const fileid &id);

        const std::unique_ptr<file>& get(const fileid &id) const;

    public:
        file_mgr();

        file_mgr(const file_mgr &other) = delete;

        file_mgr(file_mgr &&other) noexcept;

        file_mgr & operator=(const file_mgr &other) = delete;

        file_mgr & operator=(file_mgr &&other) noexcept;

        fileid add_file(const std::filesystem::path &path);

        [[nodiscard]] bool exists(const fileid &id) const;

        [[nodiscard]] std::filesystem::path path(const fileid &id) const;

        [[nodiscard]] const std::vector<char>& contents(const fileid &id) const;
    };
}

inline std::size_t std::hash<dwhbll::files::filejar::fileid>::operator()(
    const dwhbll::files::filejar::fileid &obj) const noexcept {
    return std::hash<std::uint64_t>{}(obj.id);
}

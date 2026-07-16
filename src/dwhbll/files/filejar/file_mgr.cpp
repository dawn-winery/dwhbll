#include <dwhbll/console/debug.hpp>
#include <dwhbll/files/filejar/file_mgr.h>

#include <dwhbll/files/filejar/file.h>

namespace dwhbll::files::filejar {
    std::atomic_uint64_t file_mgr::id_alloc = 0;

    std::unique_ptr<file> & file_mgr::get(const fileid &id) {
        auto it = files.find(id);

        if (it == files.end())
            debug::panic("File has not been registered with this file_mgr!");

        return it->second;
    }

    const std::unique_ptr<file> & file_mgr::get(const fileid &id) const {
        auto it = files.find(id);

        if (it == files.end())
            debug::panic("File has not been registered with this file_mgr!");

        return it->second;
    }

    file_mgr::file_mgr() = default;

    file_mgr::file_mgr(file_mgr &&other) noexcept: files(std::move(other.files)) {
    }

    file_mgr & file_mgr::operator=(file_mgr &&other) noexcept {
        if (this == &other)
            return *this;
        files = std::move(other.files);
        return *this;
    }

    fileid file_mgr::add_file(const std::filesystem::path &path) {
        auto abspath = absolute(path);

        if (const auto it = file_ids.find(abspath); it != file_ids.end())
            return it->second;

        auto id = fileid{id_alloc++};
        auto f = std::make_unique<file>(abspath);

        file_ids.emplace(abspath, id);
        files.emplace(id, std::move(f));

        return id;
    }

    bool file_mgr::exists(const fileid &id) const {
        auto& f = get(id);

        return f->exists();
    }

    std::filesystem::path file_mgr::path(const fileid &id) const {
        auto& f = get(id);

        return f->path();
    }

    const std::vector<char> & file_mgr::contents(const fileid &id) const {
        auto& f = get(id);

        return f->contents();
    }
}

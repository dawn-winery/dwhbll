import std;
import dwhbll.files;

namespace dwhbll::files::filejar {
    file::file(const std::filesystem::path &path) : _path(path),
        _exists(std::filesystem::exists(path)) {
        if (_exists) {
            std::ifstream file(_path, std::ios::in);
            _contents = std::vector<char>{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };
        }
    }

    file::file(file &&other) noexcept: _path(std::move(other._path)),
                                       _contents(std::move(other._contents)),
                                       _exists(other._exists) {
    }

    file & file::operator=(file &&other) noexcept {
        if (this == &other)
            return *this;
        _path = std::move(other._path);
        _contents = std::move(other._contents);
        _exists = other._exists;
        return *this;
    }
}

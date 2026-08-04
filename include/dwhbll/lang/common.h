#pragma once

#include <dwhbll/files/filejar/file_mgr.h>

#include <utility>

namespace dwhbll::lang {
    struct cursor {
        files::filejar::fileid file;
        std::size_t line{};
        std::size_t column{};

        explicit cursor(const files::filejar::fileid &id);

        cursor(files::filejar::fileid file, const std::size_t line,
            const std::size_t column)
            : file(std::move(file)),
              line(line),
              column(column) {
        }

        constexpr void next_line() {
            line++;
            column = 0;
        }

        constexpr void next_col() {
            column++;
        }
    };

    class span {
        files::filejar::fileid file;
        std::size_t line_begin{};
        std::size_t line_end{};
        std::size_t column_begin{};
        std::size_t column_end{};

        explicit span(const files::filejar::fileid &id);

        explicit span(const cursor &begin, const cursor &end);

        [[nodiscard]] constexpr cursor begin() const {
            return cursor{file, line_begin, column_begin};
        }

        [[nodiscard]] constexpr cursor end() const {
            return cursor{file, line_end, column_end};
        }
    };
}

#include <dwhbll/lang/common.h>

#include <dwhbll/console/debug.hpp>

namespace dwhbll::lang {
    cursor::cursor(const files::filejar::fileid &id): file(id) {}

    span::span(const files::filejar::fileid &id): file(id) {}

    span::span(const cursor &begin, const cursor &end) : file(begin.file) {
        if (begin.file != end.file)
            debug::panic("Mismatched cursor in span.");

        line_begin = begin.line;
        line_end = end.line;
        column_begin = begin.column;
        column_end = end.column;
    }
}

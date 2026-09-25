module;

#include <dwhbll/lang/common.h>
#include <dwhbll/lang/common/stream.h>
#include <dwhbll/lang/common/file_stream.h>
#include <dwhbll/lang/c/tokenize.h>
#include <dwhbll/lang/cpp/cpp_raw_stream.h>
#include <dwhbll/lang/cpp/preprocess/line_splice_stream.h>

export module dwhbll.lang;

export namespace dwhbll::lang {
    using dwhbll::lang::cursor;
    using dwhbll::lang::span;

    namespace common {
        using dwhbll::lang::common::stream;
        using dwhbll::lang::common::file_stream;
    }

    namespace c {
        using dwhbll::lang::c::token_type;
        using dwhbll::lang::c::kw_type;
        using dwhbll::lang::c::token_name;
        using dwhbll::lang::c::token;
        using dwhbll::lang::c::tokenize;
    }

    namespace cpp {
        using dwhbll::lang::cpp::cpp_raw_stream;
        namespace preprocess {
            using dwhbll::lang::cpp::preprocess::ls_token;
            using dwhbll::lang::cpp::preprocess::line_splice_helper;
        }
    }
}

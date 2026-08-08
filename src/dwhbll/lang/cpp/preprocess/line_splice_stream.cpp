#include <dwhbll/lang/cpp/preprocess/line_splice_stream.h>

#include <dwhbll/lang/cpp/common.h>

namespace dwhbll::lang::cpp::preprocess {
    void line_splice_helper::refill() {
        if (!tokens.empty())
            return;

        if (!_source->has_next())
            return;

        std::u32string buffer;

        auto begin = _head;

        auto n = _source->next();
        if (n != '\\') {
            _head.next_col();
            tokens.emplace_back(false, span{begin, _head}, n);
            return;
        }

        buffer.push_back(n);

        while (_source->has_next()) {
            n = _source->peek();
            _head.next_col();

            if (n == '\n') {
                // completed line splice
                _head.next_line();
                tokens.emplace_back(true, span{begin, _head}, buffer);
                return;
            }

            if (cpp_is_whitespace(n))
                buffer += _source->next();
            else
                break; // non whitespace means not valid.
        }

        auto end = begin;

        // return unmatched line splice back to stream
        for (char32_t c : buffer) {
            end.next_col();
            tokens.emplace_back(false, span{begin, end}, c);
            begin.next_col();
        }
    }

    bool line_splice_helper::has_next0() {
        refill();

        return !tokens.empty();
    }

    ls_token line_splice_helper::next0() {
        refill();

        if (tokens.empty())
            debug::panic("No more tokens!");

        auto next = std::move(tokens.front());
        tokens.pop_front();
        return next;
    }

    line_splice_helper::line_splice_helper(
        const files::filejar::fileid &file,
        std::unique_ptr<stream<char32_t, 16>> &source) : _source(source), _head(file) {
    }
}

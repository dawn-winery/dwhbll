#include <dwhbll/testing/testing.h>
#include <dwhbll/debug/debug.h>


namespace dwhbll::test {

namespace detail {

void result::add_failure(std::string msg, std::source_location loc) {
    failures_.push_back({std::move(msg), loc});
}

bool result::passed() const {
    return failures_.empty();
}

const std::vector<failure>& result::failures() const {
    return failures_;
}

thread_local result* current_result = nullptr;

void report_failure(std::string msg, std::source_location loc) {
    ASSERT(current_result);
    current_result->add_failure(std::move(msg), loc);
}

std::vector<entry>& registry() {
    static std::vector<entry> r;
    return r;
}

} // namespace detail

bool tag_filter::matches(const std::vector<std::string_view>& tags) const {
    for (const auto& ex : exclude) {
        if (std::find(tags.begin(), tags.end(), ex) != tags.end())
            return false;
    }
    if (include.empty())
        return true;
    for (const auto& inc : include) {
        if (std::find(tags.begin(), tags.end(), inc) != tags.end())
            return true;
    }
    return false;
}

bool expect(bool cond, std::string_view msg, std::source_location loc) {
    if (!cond)
        detail::report_failure(msg.empty() ? std::string("expect failed")
                                        : std::string(msg), loc);
    return cond;
}

tag_filter parse_filter(std::string_view input) {
    tag_filter f;
    std::size_t pos = 0;
    while (pos <= input.size()) {
        auto comma = input.find(',', pos);
        auto token = input.substr(pos, comma == std::string_view::npos
                                            ? std::string_view::npos
                                            : comma - pos);
        if (!token.empty()) {
            if (token.front() == '~')
                f.exclude.emplace_back(token.substr(1));
            else
                f.include.emplace_back(token);
        }

        if (comma == std::string_view::npos)
            break;
        pos = comma + 1;
    }
    return f;
}

int run_all(const options& options) {
    return runner().run(options);
}

int run_all(const tag_filter& filter) {
    options opts;
    opts.tags = filter;
    return runner().run(opts);
}

} // namespace dwhbll::test

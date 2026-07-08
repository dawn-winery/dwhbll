#include <dwhbll/testing/testing.hpp>
#include <dwhbll/console/debug.hpp>

#include <print>
#include <exception>

namespace dwhbll::test {

namespace detail {

void result::add_failure(std::string msg, std::source_location loc) {
    failures_.push_back({std::move(msg), loc});
}

bool result::passed() const {
    return failures_.empty();
}

std::vector<failure> const& result::failures() const {
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

bool tag_filter::matches(std::vector<std::string_view> tags) const {
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
    if (!cond) {
        detail::report_failure(msg.empty() ? std::string("expect failed")
                                        : std::string(msg), loc);
    }
    return cond;
}

namespace {

namespace color {
    constexpr std::string_view reset  = "\e[0m";
    constexpr std::string_view bold = "\e[1m";
    constexpr std::string_view dim = "\e[2m";
    constexpr std::string_view red = "\e[31m";
    constexpr std::string_view green = "\e[32m";
    constexpr std::string_view yellow = "\e[33m";
}

std::string format_tags(detail::entry const& t) {
    if (t.tags.empty())
        return "";
    std::string s = " ";
    s += color::dim;
    s += "[";
    for (std::size_t i = 0; i < t.tags.size(); ++i) {
        if (i) s += ", ";
        s += t.tags[i];
    }
    s += "]";
    s += color::reset;
    return s;
}

} // namespace

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

int run_all() {
    return run_all(tag_filter{});
}

int run_all(int argc, char** argv) {
    tag_filter filt;
    // TODO: ideally, we'd have a CLI parsing lib (in dwhbll ofc :xdd:)
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.starts_with("--filter="))
            filt = parse_filter(arg.substr(sizeof("--filter=") - 1));
        else if (arg == "--filter" && i + 1 < argc)
            filt = parse_filter(argv[++i]);
    }
    return run_all(filt);
}

int run_all(tag_filter const& filt) {
    auto& tests = detail::registry();

    std::vector<detail::entry const*> selected;
    selected.reserve(tests.size());
    for (const auto& t : tests)
        if (filt.matches(t.tags))
            selected.push_back(&t);

    std::println("{}running {} test(s) out of {}{}", color::bold, selected.size(),
                tests.size(), color::reset);

    std::size_t passed = 0, failed = 0, skipped = 0;

    for (const auto* tp : selected) {
        const auto& t = *tp;

        if (t.skip) {
            if (!t.skip_reason.empty())
                std::println("{}[skip]{} {}{} ({})", color::yellow, color::reset, t.name, format_tags(t), t.skip_reason);
            else
                std::println("{}[skip]{} {}{}", color::yellow, color::reset, t.name, format_tags(t));
            ++skipped;
            continue;
        }

        detail::result res;
        detail::current_result = &res;
        try {
            t.fn();
        } catch (std::exception const& e) {
            res.add_failure(std::format("uncaught exception: {}", e.what()),
                            std::source_location::current());
        } catch (...) {
            res.add_failure("uncaught exception of unknown type",
                            std::source_location::current());
        }
        detail::current_result = nullptr;

        if (res.passed()) {
            std::println("{}[ ok ]{} {}{}", color::green, color::reset, t.name, format_tags(t));
            ++passed;
        } else {
            std::println("{}[FAIL]{} {}{}", color::red, color::reset, t.name, format_tags(t));
            for (const auto& f : res.failures()) {
                std::println("    {}{}:{}:{} {}", color::dim, f.loc.file_name(),
                             f.loc.line(), color::reset, f.msg);
            }
            ++failed;
        }
    }

    if (failed == 0) {
        std::println("\n{}{} passed{}, {} skipped", color::green, passed,
                        color::reset, skipped);
        return 0;
    }
    else {
        std::println("\n{}{} passed{}, {}{} failed{}, {} skipped", color::green, passed,
                        color::reset, color::red, failed, color::reset, skipped);
        return 1;
    }
}

} // namespace dwhbll::test

#include <dwhbll/testing/testing.hpp>
#include <dwhbll/console/debug.hpp>

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

}

int run_all() {
    auto& tests = detail::registry();
    std::println("{}running {} test(s){}", color::bold, tests.size(), color::reset);

    std::size_t passed = 0, failed = 0, skipped = 0;

    for (auto const& t : tests) {
        if (t.skip) {
            std::println("{}[skip]{} {}", color::yellow, color::reset, t.name);
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
            std::println("{}[ ok ]{} {}", color::green, color::reset, t.name);
            ++passed;
        } else {
            std::println("{}[FAIL]{} {}", color::red, color::reset, t.name);
            for (auto const& f : res.failures()) {
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

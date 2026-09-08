#include <dwhbll/testing/harness.h>

#include <chrono>

namespace dwhbll::test {

std::string_view to_status_string(test_status status) {
    switch (status) {
        case test_status::pass: return "PASS";
        case test_status::fail: return "FAIL";
        case test_status::xfail: return "XFAIL";
        case test_status::xpass: return "XPASS";
        case test_status::unsupported: return "UNSUPPORTED";
        case test_status::unresolved: return "UNRESOLVED";
        case test_status::untested: return "UNTESTED";
    }
    return "UNKNOWN";
}

namespace {

bool match_glob(std::string_view text, std::string_view pattern) {
    if (pattern.empty())
        return text.empty();
    std::size_t t = 0, p = 0;
    std::size_t star_p = std::string_view::npos, star_t = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++t;
            ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star_p = p++;
            star_t = t;
        } else if (star_p != std::string_view::npos) {
            p = star_p + 1;
            t = ++star_t;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

bool matches_patterns(std::string_view name, const std::vector<std::string>& patterns) {
    if (patterns.empty())
        return true;
    for (const auto& pat : patterns) {
        // probably not necessary, but whatever
        if (pat.find('*') != std::string::npos || pat.find('?') != std::string::npos) {
            if (match_glob(name, pat))
                return true;
        }
        else if (name.find(pat) != std::string_view::npos)
            return true;
    }
    return false;
}

} // namespace

std::vector<test_info> default_harness::list_tests() const {
    const auto& registry = detail::registry();
    std::vector<test_info> list;
    list.reserve(registry.size());
    for (const auto& e : registry) {
        list.push_back({
            .name = std::string(e.name),
            .suite = std::string(name()),
            .tags = e.tags,
            .is_skip = e.skip,
            .skip_reason = e.skip_reason,
            .is_xfail = e.xfail,
            .xfail_reason = e.xfail_reason,
        });
    }
    return list;
}

suite_result default_harness::run(const options& options) {
    suite_result result;
    result.suite_name = std::string(name());

    const auto& registry = detail::registry();

    for (const auto& t : registry) {
        if (!options.tags.matches(t.tags))
            continue;
        if (!matches_patterns(t.name, options.patterns))
            continue;

        test_result tr;
        tr.name = std::string(t.name);
        tr.suite = std::string(name());
        tr.tags = t.tags;

        if (t.skip) {
            tr.status = test_status::unsupported;
            tr.message = std::string(t.skip_reason);
            result.add_result(std::move(tr));
            continue;
        }

        detail::result res;
        detail::current_result = &res;
        bool uncaught_exception = false;
        std::string exception_msg;

        auto start_time = std::chrono::steady_clock::now();
        try {
            t.fn();
        } catch (const std::exception& e) {
            uncaught_exception = true;
            exception_msg = e.what();
            res.add_failure(std::format("uncaught exception: {}", exception_msg),
                            std::source_location::current());
        } catch (...) {
            uncaught_exception = true;
            exception_msg = "uncaught exception of unknown type";
            res.add_failure(exception_msg,
                            std::source_location::current());
        }
        auto end_time = std::chrono::steady_clock::now();
        detail::current_result = nullptr;

        // TODO: actually use this
        tr.duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        tr.failures = res.failures();

        if (t.xfail) {
            tr.message = std::string(t.xfail_reason);
            if (!res.passed())
                tr.status = test_status::xfail;
            else
                tr.status = test_status::xpass;
        } else {
            if (uncaught_exception) {
                tr.status = test_status::unresolved;
                tr.message = std::move(exception_msg);
            } else if (res.passed())
                tr.status = test_status::pass;
            else
                tr.status = test_status::fail;
        }

        bool was_failure = tr.failed();
        result.add_result(std::move(tr));

        if (options.fail_fast && was_failure)
            break;
    }

    return result;
}

} // namespace dwhbll::test

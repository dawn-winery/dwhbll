import std;
import dwhbll.testing;
import dwhbll.stl_ext;

namespace dwhbll::test {

std::string_view to_status_string(test_status status) {
    switch (status) {
        case test_status::pass: return "PASS";
        case test_status::fail: return "FAIL";
        case test_status::xfail: return "XFAIL";
        case test_status::xpass: return "XPASS";
        case test_status::skip: return "SKIP";
        case test_status::unresolved: return "UNRESOLVED";
        case test_status::untested: return "UNTESTED";
    }
    return "UNKNOWN";
}

std::vector<test_info> default_harness::list_tests() const {
    const auto& registry = detail::registry();
    std::vector<test_info> list;
    list.reserve(registry.size());
    for (const auto& e : registry) {
        list.push_back({
            .name = std::string(e.name),
            .suite = std::string(name()),
            .is_skip = e.is_skip,
            .skip_reason = e.skip_reason,
            .is_xfail = e.is_xfail,
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
        if (!stl_ext::matches_patterns(t.name, options.patterns))
            continue;

        test_info info{
            .name = std::string(t.name),
            .suite = std::string(name()),
            .is_skip = t.is_skip,
            .skip_reason = t.skip_reason,
            .is_xfail = t.is_xfail,
            .xfail_reason = t.xfail_reason,
        };

        if (options.on_test_start)
            options.on_test_start(info);

        test_result tr;
        tr.name = std::string(t.name);
        tr.suite = std::string(name());

        if (t.is_skip) {
            tr.status = test_status::skip;
            tr.message = std::string(t.skip_reason);
            if (options.on_test_end)
                options.on_test_end(tr);
            result.add_result(std::move(tr));
            continue;
        }

        detail::result res;
        detail::current_result = &res;
        bool uncaught_exception = false;
        std::string exception_msg;

        {
            struct Guard {
                ~Guard() { detail::current_result = nullptr; }
            } guard;

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
                res.add_failure(exception_msg, std::source_location::current());
            }
            auto end_time = std::chrono::steady_clock::now();
            // TODO: actually use this
            tr.duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        }

        tr.failures = res.failures();

        if (uncaught_exception) {
            tr.status = test_status::unresolved;
            tr.message = std::move(exception_msg);
        } else if (t.is_xfail) {
            tr.message = std::string(t.xfail_reason);
            tr.status = res.passed() ? test_status::xpass : test_status::xfail;
        } else
            tr.status = res.passed() ? test_status::pass : test_status::fail;

        if (options.on_test_end)
            options.on_test_end(tr);

        bool was_failure = tr.failed();
        result.add_result(std::move(tr));

        if (options.fail_fast && was_failure)
            break;
    }

    return result;
}

} // namespace dwhbll::test

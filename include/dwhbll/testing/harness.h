#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dwhbll::test {

struct failure {
    std::string msg;
    std::string filename;
    std::uint32_t line = 0;
    std::string fname;

    failure() = default;
    failure(std::string msg_, std::source_location loc)
        : msg(std::move(msg_))
        , filename(loc.file_name() ? loc.file_name() : "")
        , line(loc.line())
        , fname(loc.function_name() ? loc.function_name() : "") {}
    failure(std::string msg_, std::string file_, std::uint32_t line_, std::string func_ = "")
        : msg(std::move(msg_)), filename(std::move(file_)), line(line_), fname(std::move(func_)) {}
};

enum class test_status {
    pass,
    fail,
    xfail,
    xpass,
    unsupported,
    unresolved,
    untested
};

std::string_view to_status_string(test_status status);

struct test_result {
    std::string name;
    test_status status = test_status::pass;
    std::vector<failure> failures;
    std::string message;

    [[nodiscard]] bool passed() const {
        return status == test_status::pass || status == test_status::xfail;
    }

    [[nodiscard]] bool failed() const {
        return status == test_status::fail || status == test_status::xpass || status == test_status::unresolved;
    }

    [[nodiscard]] bool skipped() const {
        return status == test_status::unsupported || status == test_status::untested;
    }
};

struct test_info {
    std::string name;
    std::string suite;
    bool is_skip = false;
    std::string_view skip_reason;
    bool is_xfail = false;
    std::string_view xfail_reason;
};

struct options {
    std::string suite_filter;
    std::vector<std::string> patterns;
    bool list_only = false;
    bool color = true;
    std::size_t jobs = 0;
    FILE* console_out = stdout;
};

struct summary_counts {
    std::size_t passes = 0;
    std::size_t failures = 0;
    std::size_t xfails = 0;
    std::size_t xpasses = 0;
    std::size_t unsupported = 0;
    std::size_t unresolved = 0;
    std::size_t untested = 0;

    void add(test_status status) {
        switch (status) {
            case test_status::pass:
                ++passes; break;
            case test_status::fail:
                ++failures; break;
            case test_status::xfail:
                ++xfails; break;
            case test_status::xpass:
                ++xpasses; break;
            case test_status::unsupported:
                ++unsupported; break;
            case test_status::unresolved:
                ++unresolved; break;
            case test_status::untested:
                ++untested; break;
        }
    }

    void add(const test_result& result) {
        add(result.status);
    }

    summary_counts& operator+=(const summary_counts& other) {
        passes += other.passes;
        failures += other.failures;
        xfails += other.xfails;
        xpasses += other.xpasses;
        unsupported += other.unsupported;
        unresolved += other.unresolved;
        untested += other.untested;
        return *this;
    }

    [[nodiscard]] std::size_t total() const {
        return passes + failures + xfails + xpasses + unsupported + unresolved + untested;
    }

    [[nodiscard]] bool is_success() const {
        return failures == 0 && xpasses == 0 && unresolved == 0;
    }
};

struct suite_result {
    std::string suite_name;
    std::vector<test_result> results;
    summary_counts counts;

    void add_result(test_result res) {
        counts.add(res.status);
        results.push_back(std::move(res));
    }
};

class test_harness {
public:
    virtual ~test_harness() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual std::vector<test_info> list_tests() const = 0;
    virtual suite_result run(const options& options) = 0;
};

class default_harness : public test_harness {
public:
    [[nodiscard]] std::string_view name() const override { return "default"; }
    [[nodiscard]] std::vector<test_info> list_tests() const override;
    suite_result run(const options& options) override;
};

class runner {
public:
    explicit runner(bool def_harness = true);

    runner& add_harness(std::shared_ptr<test_harness> harness);

    template <typename H, typename... Args>
    requires std::derived_from<H, test_harness>
    runner& add_harness(Args&&... args) {
        return add_harness(std::make_shared<H>(std::forward<Args>(args)...));
    }

    [[nodiscard]] std::vector<test_info> list_all_tests() const;

    [[nodiscard]] std::vector<suite_result> run_suites(const options& options) const;

    [[nodiscard]] int run(const options& options = {}) const;

private:
    std::vector<std::shared_ptr<test_harness>> harnesses_;
};


} // namespace dwhbll::test

#pragma once

#include <chrono>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dwhbll::test {

struct failure {
    std::string msg;
    std::source_location loc;
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
    std::string suite = "unit";
    test_status status = test_status::pass;
    std::vector<failure> failures;
    std::string message;
    std::chrono::microseconds duration{0};

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

struct options {
    std::string suite_filter;
    std::vector<std::string> patterns;
    bool list_only = false;
    bool fail_fast = false;
    bool color = true;
};

struct test_info {
    std::string name;
    std::string suite;
    bool is_skip = false;
    std::string_view skip_reason;
    bool is_xfail = false;
    std::string_view xfail_reason;
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

    [[nodiscard]] const std::vector<std::shared_ptr<test_harness>>& harnesses() const {
        return harnesses_;
    }

    [[nodiscard]] std::vector<test_info> list_all_tests() const;

    [[nodiscard]] std::vector<suite_result> run_suites(const options& options) const;

    [[nodiscard]] int run(const options& options = {}) const;

private:
    std::vector<std::shared_ptr<test_harness>> harnesses_;
};


} // namespace dwhbll::test

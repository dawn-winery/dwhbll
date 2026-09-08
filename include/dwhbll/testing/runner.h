#pragma once

#include <dwhbll/testing/harness.h>

#include <concepts>
#include <memory>
#include <vector>

namespace dwhbll::test {

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

    int run(const options& options = {}) const;

private:
    std::vector<std::shared_ptr<test_harness>> harnesses_;
};

} // namespace dwhbll::test

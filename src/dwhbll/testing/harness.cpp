#include <dwhbll/testing/testing.h>
#include <dwhbll/testing/harness.h>
#include <dwhbll/console/ansi_escape.h>
#include <dwhbll/stl_ext/string.h>

#include <print>
#include <fstream>
#include <thread>
#include <cstring>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace dwhbll::test {

namespace {

namespace color = console::ansi_escape::color;

std::string_view status_color(test_status status) {
    switch (status) {
        case test_status::pass: return color::green;
        case test_status::fail: return color::red;
        case test_status::xfail: return color::yellow;
        case test_status::xpass: return color::magenta;
        case test_status::unsupported: return color::yellow;
        case test_status::unresolved: return color::red;
        case test_status::untested: return color::dim;
    }
    return color::reset;
}

std::string get_source_line(std::string_view file_path, std::uint32_t line_num) {
    if (file_path.empty() || line_num == 0)
        return "";
    std::ifstream file{std::string(file_path)};
    if (!file.is_open())
        return "";
    std::string line;
    std::uint32_t current_line = 0;
    while (std::getline(file, line)) {
        if (++current_line == line_num) {
            auto start = line.find_first_not_of(" \t");
            return (start != std::string::npos) ? line.substr(start) : line;
        }
    }
    return "";
}

void print_test_result(FILE* out, const test_result& tr, bool use_color) {
    auto st_str = to_status_string(tr.status);

    bool has_reason = !tr.message.empty() &&
        (tr.status == test_status::unsupported ||
         tr.status == test_status::xfail ||
         tr.status == test_status::xpass);

    auto col = use_color ? status_color(tr.status) : "";
    auto reset = use_color ? color::reset : "";
    auto err_col = use_color ? color::red : "";

    if (has_reason)
        std::println(out, "{}{}:{} {} ({})", col, st_str, reset, tr.name, tr.message);
    else
        std::println(out, "{}{}:{} {}", col, st_str, reset, tr.name);

    for (const auto& f : tr.failures) {
        auto src_line = get_source_line(f.filename, f.line);
        std::string_view fn_name = f.fname;
        if (!fn_name.empty())
            std::println(out, "    {}:{}: in {}:", f.filename, f.line, fn_name);
        else if (!f.filename.empty())
            std::println(out, "    {}:{}:", f.filename, f.line);
        if (!src_line.empty())
            std::println(out, "      {:4d} | {}", f.line, src_line);
        std::println(out, "      {}{}{}", err_col, f.msg, reset);
    }
    std::fflush(out);
}

} // namespace

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

    std::vector<std::size_t> test_indices;
    for (std::size_t i = 0; i < registry.size(); ++i) {
        if (stl_ext::matches_patterns(registry[i].name, options.patterns))
            test_indices.push_back(i);
    }

    std::size_t num_jobs = options.jobs;
    if (num_jobs == 0)
        num_jobs = std::thread::hardware_concurrency();

    auto run_single_test = [&](std::size_t idx, int write_fd) {
        const auto& t = registry[idx];
        detail::result res;
        detail::current_result = &res;

        // TODO: Righ now logs are not properly split when running in parallel
        //       and are hard to read.
        //       Ideally we'd either send stdout to the main process which
        //       would then append it to log once the test ends, or have
        //       each test write in a different file (and make a log/ dir)
        std::println(stdout, "\n=== RUNNING: {} ===", t.name);
        std::fflush(stdout);

        try {
            t.fn();
        } catch (const std::exception& e) {
            res.add_failure(std::format("uncaught exception: {}", e.what()),
                            std::source_location::current());
        } catch (...) {
            res.add_failure("uncaught exception of unknown type", std::source_location::current());
        }

        const auto& failures = res.failures();
        std::uint32_t fail_count = failures.size();
        write(write_fd, &fail_count, sizeof(fail_count));

        for (const auto& f : failures) {
            std::uint32_t msg_len = f.msg.size();
            write(write_fd, &msg_len, sizeof(msg_len));
            if (msg_len > 0)
                write(write_fd, f.msg.data(), msg_len);

            std::uint32_t file_len = f.filename.size();
            write(write_fd, &file_len, sizeof(file_len));
            if (file_len > 0)
                write(write_fd, f.filename.data(), file_len);

            std::uint32_t line_num = f.line;
            write(write_fd, &line_num, sizeof(line_num));

            std::uint32_t func_len = f.fname.size();
            write(write_fd, &func_len, sizeof(func_len));
            if (func_len > 0)
                write(write_fd, f.fname.data(), func_len);
        }

        test_status st;
        if (t.is_xfail)
            st = failures.empty() ? test_status::xpass : test_status::xfail;
        else
            st = failures.empty() ? test_status::pass : test_status::fail;

        std::println(stdout, "=== END: {} ({}) ===", t.name, to_status_string(st));
        std::fflush(stdout);

        close(write_fd);
        _exit(res.passed() ? 0 : 1);
    };

    auto read_test_result = [](int read_fd, test_result& tr) {
        std::uint32_t fail_count = 0;
        if (read(read_fd, &fail_count, sizeof(fail_count)) != sizeof(fail_count))
            return;
        for (std::uint32_t i = 0; i < fail_count; ++i) {
            std::uint32_t msg_len = 0;
            read(read_fd, &msg_len, sizeof(msg_len));
            std::string msg(msg_len, '\0');
            if (msg_len > 0)
                read(read_fd, msg.data(), msg_len);

            std::uint32_t file_len = 0;
            read(read_fd, &file_len, sizeof(file_len));
            std::string file_str(file_len, '\0');
            if (file_len > 0)
                read(read_fd, file_str.data(), file_len);

            std::uint32_t line_num = 0;
            read(read_fd, &line_num, sizeof(line_num));

            std::uint32_t func_len = 0;
            read(read_fd, &func_len, sizeof(func_len));
            std::string func_str(func_len, '\0');
            if (func_len > 0)
                read(read_fd, func_str.data(), func_len);

            tr.failures.push_back(failure{
                std::move(msg),
                std::move(file_str),
                line_num,
                std::move(func_str)
            });
        }
    };

    int ep_fd = epoll_create1(0);

    struct Worker {
        pid_t pid;
        int read_fd;
        std::size_t test_idx;
    };

    std::unordered_map<int, Worker> active_workers;
    std::size_t next_work_idx = 0;

    FILE* console_out = options.console_out ? options.console_out : stdout;

    auto spawn_worker = [&](std::size_t test_list_idx) -> bool {
        std::size_t reg_idx = test_indices[test_list_idx];
        const auto& t = registry[reg_idx];

        if (t.is_skip) {
            test_result tr;
            tr.name = std::string(t.name);
            tr.status = test_status::unsupported;
            tr.message = std::string(t.skip_reason);
            std::println(stdout, "\n=== RUNNING: {} ===", t.name);
            std::println(stdout, "=== END: {} ({}) ===", t.name, to_status_string(tr.status));
            std::fflush(stdout);
            print_test_result(console_out, tr, options.color);
            result.add_result(std::move(tr));
            return false;
        }

        int pipe_fds[2];
        if (pipe(pipe_fds) != 0) {
            test_result tr;
            tr.name = std::string(t.name);
            tr.status = test_status::unresolved;
            tr.message = "pipe failed: " + std::string(strerror(errno));
            print_test_result(console_out, tr, options.color);
            result.add_result(std::move(tr));
            return false;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            test_result tr;
            tr.name = std::string(t.name);
            tr.status = test_status::unresolved;
            tr.message = "fork failed: " + std::string(strerror(errno));
            print_test_result(console_out, tr, options.color);
            result.add_result(std::move(tr));
            return false;
        }

        if (pid == 0) {
            close(pipe_fds[0]);
            if (ep_fd >= 0)
                close(ep_fd);
            run_single_test(reg_idx, pipe_fds[1]);
        }

        close(pipe_fds[1]);

        if (ep_fd >= 0) {
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
            ev.data.fd = pipe_fds[0];
            epoll_ctl(ep_fd, EPOLL_CTL_ADD, pipe_fds[0], &ev);
        }

        active_workers[pipe_fds[0]] = {
            .pid = pid,
            .read_fd = pipe_fds[0],
            .test_idx = test_list_idx,
        };
        return true;
    };

    while (next_work_idx < test_indices.size() && active_workers.size() < num_jobs) {
        spawn_worker(next_work_idx);
        next_work_idx++;
    }

    std::vector<epoll_event> events(num_jobs > 0 ? num_jobs : 16);

    while (!active_workers.empty()) {
        int nfds = 0;
        if (ep_fd < 0)
            break;

        nfds = epoll_wait(ep_fd, events.data(), events.size(), -1);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            auto it = active_workers.find(fd);
            if (it == active_workers.end())
                continue;

            Worker w = it->second;
            active_workers.erase(it);
            if (ep_fd >= 0) {
                epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, nullptr);
            }

            std::size_t reg_idx = test_indices[w.test_idx];
            const auto& t = registry[reg_idx];

            test_result tr;
            tr.name = std::string(t.name);

            read_test_result(w.read_fd, tr);
            close(w.read_fd);

            int wstatus = 0;
            waitpid(w.pid, &wstatus, 0);

            if (WIFSIGNALED(wstatus)) {
                int sig = WTERMSIG(wstatus);
                tr.status = test_status::unresolved;
                const char* sig_name = strsignal(sig);
                tr.message = std::format("test process terminated by signal {} ({})", sig, sig_name ? sig_name : "unknown");
                tr.failures.push_back(failure(tr.message, std::source_location::current()));
                std::println(stdout, "=== END: {} ({}) ===", tr.name, to_status_string(tr.status));
                std::fflush(stdout);
            } else if (t.is_xfail) {
                tr.message = std::string(t.xfail_reason);
                tr.status = tr.failures.empty() ? test_status::xpass : test_status::xfail;
            } else {
                tr.status = tr.failures.empty() ? test_status::pass : test_status::fail;
                if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0 && tr.failures.empty()) {
                    tr.status = test_status::fail;
                    tr.message = std::format("test process exited with status {}",
                            WEXITSTATUS(wstatus));
                }
            }

            print_test_result(console_out, tr, options.color);
            result.add_result(std::move(tr));

            while (next_work_idx < test_indices.size() && active_workers.size() < num_jobs) {
                spawn_worker(next_work_idx);
                next_work_idx++;
            }
        }
    }

    if (ep_fd >= 0)
        close(ep_fd);

    return result;
}

} // namespace dwhbll::test

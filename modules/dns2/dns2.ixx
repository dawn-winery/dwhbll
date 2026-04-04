module;

// #include <chrono>
// #include <vector>
//
// #include <dwhbll/concurrency/coroutine/task.h>
// #include <dwhbll/network/ip_address.h>

export module dwhbll.dns2;

export import :proto;

export namespace dwhbll::modules::dns2 {
    // class DNSResolver {
    //     template <typename T>
    //     using task = concurrency::coroutine::task<T>;
    //
    // public:
    //     struct Options {
    //         std::chrono::milliseconds timeout{};
    //         bool ipv4;
    //         bool ipv6;
    //         bool use_cache;
    //
    //         Options() {
    //             using namespace std::chrono_literals;
    //
    //             timeout = 50ms;
    //             ipv4 = true;
    //             ipv6 = true;
    //             use_cache = true;
    //         }
    //     };
    //
    //     task<std::vector<network::IPAddress>> resolve(const std::string_view& domain);
    //
    //     task<std::vector<network::IPAddress>> resolve(const std::string_view& domain, const Options& options);
    // };
    //
    // auto DNSResolver::resolve(const std::string_view& domain) -> task<std::vector<network::IPAddress>> {
    //     return resolve(domain, Options());
    // }
}


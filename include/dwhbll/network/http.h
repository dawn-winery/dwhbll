#pragma once

#include <dwhbll/network/buffered_socket.h>
#include <dwhbll/network/SocketManager.h>

namespace dwhbll::network {
    struct http_request {
        enum Method {
            GET,
            HEAD,
            POST,
        } method;

        std::string path;

        std::unordered_map<std::string, std::string> headers;

        std::vector<sanify::u8> body;
    };

    struct http_response {
        struct status_line {
            std::int32_t http_major, http_minor;
            std::uint32_t status_code;
            std::string status_info;

            [[nodiscard]] std::string to_string() const;
        } status;

        std::unordered_map<std::string, std::string> headers;

        std::vector<sanify::u8> body;

        [[nodiscard]] std::string to_string() const;
    };

    class http {
        buffered_socket socket;

        /// Base socket manager, can use a different one if necessary (functionality todo)
        static SocketManager socketManager;

    public:
        http(in_addr addr, unsigned short port = 80);

        http(memory::Pool<Socket>::ObjectWrapper&& socket);

        std::optional<http_response> make_request(http_request req); // todo

    private:
        /*
         * REQUESTS REGION
         */

        void write_request_line(const http_request& req);

        void write_request_method(http_request::Method method);

        void write_request_abs_path(const std::string& abs_path);

        void write_request_header(const std::string& key, const std::string& value);

        void write_request_headers(const std::unordered_map<std::string, std::string>& headers);

        void write_body(const std::span<sanify::u8>& body);

        /*
         * RESPONSES REGION
         */

        void parse_status_line(http_response& resp);

        void parse_headers(http_response& resp);

        void parse_body(http_response& resp);
    };
}

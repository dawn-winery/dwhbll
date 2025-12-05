#include "../../../include/dwhbll/network/http.h"

#include <iostream>
#include <dwhbll/console/Logging.h>
#include <dwhbll/exceptions/rt_exception_base.h>

#define CRLF "\r\n"

namespace dwhbll::network {
    SocketManager http::socketManager{};

    std::string http_response::status_line::to_string() const {
        return std::format("{}: {}, HTTP/{}.{}", status_code, status_info, http_major, http_minor);
    }

    std::string http_response::to_string() const {
        std::string result = std::format("{}\n\n", status.to_string());

        for (const auto& [k, v]: headers) {
            result += std::format("HEADER: {{{}, {}}}\n", k, v);
        }

        for (auto c : body) {
            // TODO: needed because clion treads \r as "clear last output line"
            if (c == '\r')
                continue;
            result += c;
        }

        return result;
    }

    http::http(in_addr addr, unsigned short port) : socket(socketManager.getIPv4TCPSocket(addr, port)) {}

    http::http(memory::Pool<Socket>::ObjectWrapper &&socket) : socket(std::move(socket)) {}

    std::optional<http_response> http::make_request(http_request req) {
        write_request_line(req);
        write_request_headers(req.headers);
        if (req.body.empty()) {
            socket.outbound.write_string(CRLF);
        } else {
            std::span spn = req.body;
            write_body(spn);
        }

        socket.outbound.flush();

        socket.socket_ref()->wait();

        http_response resp;

        try {
            parse_status_line(resp);

            parse_headers(resp);

            socket.inbound.read_u16(); // read the CRLF

            parse_body(resp);

            return resp;
        } catch (const exceptions::rt_exception_base& e) {
            console::error("Unexpected error while parsing data down.");
            console::info(e.what());
            e.trace_to_stderr();
        }

        return std::nullopt;
    }

    void http::write_request_line(const http_request &req) {
        write_request_method(req.method);
        socket.outbound.write_u8(' ');
        write_request_abs_path(req.path);
        socket.outbound.write_string(" HTTP/1.0" CRLF);
    }

    void http::write_request_method(http_request::Method method) {
        switch (method) {
            case http_request::Method::GET:
                socket.outbound.write_string("GET");
                break;
            case http_request::Method::POST:
                socket.outbound.write_string("POST");
                break;
            case http_request::Method::HEAD:
                socket.outbound.write_string("HEAD");
                break;
            default:
                throw exceptions::rt_exception_base("unhandled request type!");
        }
    }

    void http::write_request_abs_path(const std::string &abs_path) {
        socket.outbound.write_string(abs_path);
    }

    void http::write_request_header(const std::string &key, const std::string &value) {
        socket.outbound.write_string(key);
        socket.outbound.write_string(":");
        socket.outbound.write_string(value);
        socket.outbound.write_string(CRLF);
    }

    void http::write_request_headers(const std::unordered_map<std::string, std::string> &headers) {
        for (const auto& [key, value] : headers) {
            write_request_header(key, value);
        }
    }

    void http::write_body(const std::span<sanify::u8>& body) {
        write_request_header("Content-Length", std::to_string(body.size()));

        // CRLFCRLF
        socket.outbound.write_string(CRLF);

        socket.outbound.write_vector(body);
    }

    void http::parse_status_line(http_response& resp) {
        socket.inbound.expect("HTTP/");
        resp.status.http_major = socket.inbound.parse_u64();
        socket.inbound.expect('.');
        resp.status.http_minor = socket.inbound.parse_u64();
        socket.inbound.consume_any_whitespace();
        resp.status.status_code = socket.inbound.parse_u64(3);
        socket.inbound.consume_any_whitespace();
        resp.status.status_info = std::move(socket.inbound.consume_until_eol(files::EOLType::crlf));
    }

    void http::parse_headers(http_response &resp) {
        // not CRLF
        // TODO: this might blow up wont it :xdd:
        while (socket.inbound.peek_u8() != '\r' && socket.inbound.peek_u8(1) != '\n') {
            socket.inbound.refill_buffer();

            std::string key = socket.inbound.consume_until_token(':');
            std::string value = socket.inbound.consume_until_eol(files::EOLType::crlf);

            char peeked;

            while (peeked = socket.inbound.peek_u8(), peeked == '\t' || peeked == ' ') {
                value += socket.inbound.consume_until_eol(files::EOLType::crlf);
            }

            resp.headers[key] = value;
        }
    }

    void http::parse_body(http_response &resp) {
        while (true) {
            socket.inbound.refill_buffer();
            if (socket.inbound.empty())
                break;
            auto buf = socket.inbound.read_vector(socket.inbound.size());
            resp.body.insert(resp.body.end(), buf.begin(), buf.end());
        }
    }
}

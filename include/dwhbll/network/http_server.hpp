#pragma once

#include "dwhbll/console/Logging.h"
#include <cerrno>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <vector>

namespace dwhbll::network::http_server {

enum class Method {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

enum class Version {
  V_11,
  // TODO V2, V3, maybe V_10
};

struct Request {
  Method method;
  std::string uri;
  Version version;
  std::unordered_map<std::string, std::string> fields;
  std::vector<std::byte> body;

  void reset() {
    fields.clear();
    body.clear();
  }
};

struct Response {
  Version version;
  std::string code;
  std::string reason;
  std::unordered_map<std::string, std::string> fields;
  std::vector<std::byte> body;

  void reset() {
    code.clear();
    reason.clear();
    fields.clear();
    body.clear();
  }
};

}; // namespace dwhbll::network::http_server

namespace {

struct string_hash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;

  std::size_t operator()(const char *str) const { return hash_type{}(str); }
  std::size_t operator()(std::string_view str) const {
    return hash_type{}(str);
  }
  std::size_t operator()(std::string const &str) const {
    return hash_type{}(str);
  }
};

static const std::unordered_map<std::string,
                                dwhbll::network::http_server::Method,
                                string_hash, std::equal_to<>>
    method_map = {{"GET", dwhbll::network::http_server::Method::GET},
                  {"HEAD", dwhbll::network::http_server::Method::HEAD},
                  {"POST", dwhbll::network::http_server::Method::POST},
                  {"PUT", dwhbll::network::http_server::Method::PUT},
                  {"DELETE", dwhbll::network::http_server::Method::DELETE},
                  {"CONNECT", dwhbll::network::http_server::Method::CONNECT},
                  {"OPTIONS", dwhbll::network::http_server::Method::OPTIONS},
                  {"TRACE", dwhbll::network::http_server::Method::TRACE},
                  {"PATCH", dwhbll::network::http_server::Method::PATCH}};

class SocketBuilder {
  int fd = -1;

public:
  ~SocketBuilder() {
    if (fd != -1) {
      ::close(fd);
    }
  }

  int get_fd() {
    int temp = fd;
    fd = -1;
    return temp;
  }

  int create_socket() {
    fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
      switch (errno) {
      case EACCES:
        dwhbll::console::error(
            "Permission to create the server socket is denied.");
        break;

      case ENFILE:
        dwhbll::console::error(
            "System openfd limit exceeded, cannot create the socket.");
        break;

      case ENOBUFS:
      case ENOMEM:
        dwhbll::console::error("Insufficient memory to create the socket.");
        break;

      default:
        dwhbll::console::error(
            std::format("Error {} raised by the system.", errno));
        break;
      }

      return -1;
    }

    int optval = 1;
    int status =
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (status == -1) {
      dwhbll::console::error("Failed to set SO_REUSEPORT on the socket");
      return -1;
    }

    return 0;
  }

  int bind_addr(const uint32_t address, const uint16_t port) {

    sockaddr_in sock_addr = {.sin_family = AF_INET,
                             .sin_port = htons(port),
                             .sin_addr = {.s_addr = htonl(address)}};

    int status = ::bind(fd, (sockaddr *)&sock_addr, sizeof(sock_addr));

    if (status == -1) {
      switch (errno) {
      case EACCES:
        dwhbll::console::error("Address is protected, binding not possible");
        break;

      case EADDRINUSE:
        dwhbll::console::error("Address already in use");
        break;

      default:
        dwhbll::console::error(std::format("Error {} raised by system", errno));
        break;
      }

      return -1;
    }

    return 0;
  }
};

class Socket {
  int socket;
  std::array<std::byte, 4096> recv_buffer;
  size_t recv_readpos = 0, recv_size = 0;
  pollfd recv_event;

  std::array<std::byte, 4096> send_buffer;
  size_t send_size = 0;
  pollfd send_event;

public:
  void assign_socket(int socket) {
    dwhbll::console::info("assign socket");
    this->socket = socket;

    int value;
    socklen_t size = sizeof(value);

    recv_event = {.fd = socket, .events = POLLIN};
    send_event = {.fd = socket, .events = POLLOUT};

    recv_readpos = 0;
    recv_size = 0;
  }

  void refill_buffer() {
    recv_readpos = 0;
    dwhbll::console::info("refilling buffer");

    if (::poll(&recv_event, 1, 3000) == 1) {
      ssize_t temp = ::read(socket, recv_buffer.data(), recv_buffer.max_size());
      if (temp == -1) {
        recv_size = 0;
      } else {
        recv_size = temp;
      }
    }

    dwhbll::console::info(std::format("read {} bytes", recv_size));
  }

  bool check_buffer() {
    if (recv_readpos < recv_size) {
      return true;
    }

    refill_buffer();
    return recv_readpos < recv_size;
  }

  std::string read_until_crlf() {
    std::string result;
    char prev = 'a';

    while (check_buffer()) {
      char data = (char)recv_buffer[recv_readpos++];

      // scuffed but this avoid UB reading string.back()
      if (data == '\n' && prev == '\r') {
        result.pop_back();
        break;
      }

      result.push_back(data);
      prev = data;
    }

    return result;
  }

  void read_bytes(std::vector<std::byte> &buf, size_t byte_count) {
    for (size_t byte_read = 0; byte_read < byte_count; byte_read++) {
      if (!check_buffer()) {
        return;
      }

      buf.push_back(recv_buffer[recv_readpos++]);
    }
  }

  void write(const std::vector<std::byte> &content) {
    dwhbll::console::info(std::format("current buffer size: {} bytes", send_size));
    // the std has nothing for this op?
    for (auto it = content.begin(); it != content.end(); ++it) {
      send_buffer[send_size++] = *it;

      if (send_size >= send_buffer.max_size()) {
        flush();
      }
    }
  }

  void write(const std::string &content) {
    dwhbll::console::info(std::format("current buffer size: {} bytes", send_size));
    for (auto it = content.begin(); it != content.end(); ++it) {
      send_buffer[send_size++] = std::byte(*it);

      if (send_size >= send_buffer.max_size()) {
        flush();
      }
    }
  }

  void write(const char *content) {
    dwhbll::console::info(std::format("current buffer size: {} bytes", send_size));
    while (*content != '\0') {
      send_buffer[send_size++] = std::byte(*(content++));

      if (send_size >= send_buffer.max_size()) {
        flush();
      }
    }
  }

  void flush() {
    dwhbll::console::info(std::format("flushing {} bytes to socket", send_size));
    size_t send_pos = 0;

    while (send_pos < send_size) {
      if (::poll(&send_event, 1, 1000) == 1) {
        ssize_t status =
            ::write(socket, &send_buffer[send_pos], send_size - send_pos);

        if (status == -1) {
          continue;
        }

        send_pos += status;
        dwhbll::console::info(std::format("sent {} bytes", status));
      }

      // Timed out trying to write
      // TODO: figure out what to do..?
      send_size = 0;
      return;
    }

    send_size = 0;
  }
};

static std::vector<std::string_view> split_string(const std::string &str,
                                                  const char delim) {
  std::vector<std::string_view> result;
  auto left = str.cbegin();
  for (auto it = left; it != str.cend(); ++it) {
    if (*it == delim) {
      result.emplace_back(&*left, it - left);
      left = it + 1;
    }
  }

  if (left != str.cend()) {
    result.emplace_back(&*left, str.cend() - left);
  }

  return result;
}

static void trim_whitespace(std::string_view &str) {
  str.remove_prefix(std::min(str.find_first_not_of(' '), str.size()));
  str.remove_suffix(std::max(str.size() - str.find_last_not_of(' ') - 1, 0ul));
}

static std::string to_lower_case(const std::string_view &str) {
  std::string out;
  out.reserve(str.size());

  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    out.push_back(std::tolower(*it));
  }

  return out;
}

// TODO: switch to error enum for caller to build response
static bool build_request(dwhbll::network::http_server::Request &request,
                          Socket &reader) {
  request.reset();

  dwhbll::console::info("reading from socket");
  const auto header = reader.read_until_crlf();
  dwhbll::console::info(std::format("read line {}", header));
  const auto section = split_string(header, ' ');

  if (section.size() != 3) {
    dwhbll::console::error("Request header does not have exactly 3 sections");
    return false;
  }

  dwhbll::console::info("section size OK");

  {
    auto method_ref = method_map.find(section[0]);
    if (method_ref == method_map.end()) {
      dwhbll::console::error("Unknown method name in header");
      return false;
    }
    request.method = method_ref->second;
  }

  // TODO: provide support for thing other than 1.1
  if (section[2] != "HTTP/1.1") {
    dwhbll::console::error("Unsupported HTTP version");
    return false;
  }

  dwhbll::console::info("version OK");

  request.uri = std::string(section[1]);
  request.version = dwhbll::network::http_server::Version::V_11;
  size_t body_size = 0;

  bool has_length = false, is_chunked = false;
  std::string field;
  while ((field = reader.read_until_crlf()) != "") {
    dwhbll::console::info(std::format("field {}", field));
    auto field_part = split_string(field, ':');
    // if (field_part.size() != 2) {
    //   return false;
    // }
    trim_whitespace(field_part[1]);

    // Surely the field name is ASCII-only
    auto field_name = to_lower_case(field_part[0]);

    if (field_name == "content-length") {
      has_length = true;
      auto result = std::from_chars(field_part[1].data(),
                                    field_part[1].data() + field_part[1].size(),
                                    body_size);
      if (result.ec != std::errc()) {
        return false;
      }
      continue;
    }

    if (field_name == "transfer-encoding" && field_part[1] == "chunked") {
      is_chunked = true;
    }

    request.fields.emplace(std::move(field_name), field_part[1]);
  }

  if (has_length) {
    reader.read_bytes(request.body, body_size);
    return true;
  }

  if (is_chunked) {
    // TODO: reading chunked data
    // dwhbll::debug::unimplemented() when
    throw std::runtime_error("unimplemented");
    return true;
  }

  dwhbll::console::info("no size given?");

  return true;
}

} // anonymous namespace

namespace dwhbll::network::http_server {

template <typename H>
concept Handler = requires(H handler, Request &req, Response &res) {
  { handler.handle(req, res) } -> std::same_as<void>;
};

template <typename F>
concept HandlerFactory = requires(F factory) {
  { factory() } -> Handler;
};

template <HandlerFactory F>
static void executor(const int listen_socket,
                     std::unordered_map<std::string, F> &rt) {
  dwhbll::console::info("executor");
  sockaddr inaddr_buf;
  socklen_t inaddr_bufsize;
  Socket socket;
  Request request;
  Response response;

  // TODO: exiting the loop
  while (true) {
    const int com_sockfd =
        ::accept4(listen_socket, &inaddr_buf, &inaddr_bufsize, SOCK_NONBLOCK);

    if (com_sockfd == -1) {
      continue;
    }

    socket.assign_socket(com_sockfd);

    dwhbll::console::info("handling message");

    if (!build_request(request, socket)) {
      dwhbll::console::info("cannot build request");
      // TODO: send back a malformed request or smth
      // this need to be an enum return maybe.
      continue;
    }

    dwhbll::console::info("matching route");
    auto route = rt.find(request.uri);
    if (route == rt.end()) {
      dwhbll::console::info("cannot match a route");
      // TODO: send back a malformed request
      continue;
    }

    dwhbll::console::info("handling request with route handler");
    route->second().handle(request, response);
    dwhbll::console::info("sending back response");

    // wtf is this
    dwhbll::console::info("writing header");
    socket.write("HTTP/1.1 ");
    socket.write(response.code);
    socket.write(" ");
    socket.write(response.reason);
    socket.write("\r\n");

    dwhbll::console::info("writing fields");
    for (auto it = response.fields.cbegin(); it != response.fields.cend();
         ++it) {
      socket.write(it->first);
      socket.write(": ");
      socket.write(it->second);
      socket.write("\r\n");
    }

    dwhbll::console::info("writing body");
    socket.write("\r\n");
    socket.write(response.body);
    socket.flush();

    ::close(com_sockfd);

    response.reset();
  }
}

template <HandlerFactory F> class Server {
private:
  std::vector<std::thread> thread_pool;
  std::unordered_map<std::string, F> route_table;
  int server_fd = -1;

public:
  ~Server() {
    if (server_fd != -1) {
      ::close(server_fd);
    }
  }

  int listen_to(const uint32_t address, const uint16_t port) {
    const bool enable_bool = 1;
    int status;

    if (server_fd != -1) {
      console::error("Server already bound.");
      return -1;
    }

    SocketBuilder sock;

    if (sock.create_socket()) {
      return -1;
    }

    if (sock.bind_addr(address, port)) {
      return -1;
    }

    server_fd = sock.get_fd();
    return 0;
  }

  void add_route(const std::string route, F factory) {
    route_table.insert(std::make_pair(route, factory));
  }

  int listen(const size_t worker_count = std::thread::hardware_concurrency(),
             const uint32_t pending_queue_size = SOMAXCONN) {
    const int status = ::listen(server_fd, 5);

    if (status == -1) {
      console::error("Cannot set socket to start listening");
      return -1;
    }

    thread_pool.reserve(worker_count);
    for (size_t amount = 0; amount < worker_count; amount++) {
      thread_pool.push_back(
          std::thread(executor<F>, server_fd, std::ref(route_table)));
    }

    return 0;
  }

  void wait_finish() {
    for (auto &thread_handle : thread_pool) {
      thread_handle.join();
    }
  }

  void stop();
};
} // namespace dwhbll::network::http_server

#include <dwhbll/network/http_server.hpp>

struct Handler {
  void handle(dwhbll::network::http_server::Request &request,
              dwhbll::network::http_server::Response &response) {
    response.code = "418";
    response.reason = "I'm a teapot";
  }
};

struct Factory {
  Handler operator()() { return Handler(); }
};

int main() {
  dwhbll::network::http_server::Server<Factory> server;
  dwhbll::console::info("adding route");
  server.add_route("/", Factory());
  dwhbll::console::info("listening to stuff");
  server.listen_to(dwhbll::network::http_server::ipv4(127, 0, 0, 1), 8000);
  dwhbll::console::info("starting threads");
  server.listen();
  dwhbll::console::info("waiting for them");
  server.wait_finish();
}

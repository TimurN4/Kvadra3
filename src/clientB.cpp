#include <iostream>
#include <cmath>
#include <istream>
#include <thread>
#include <boost/asio.hpp>
#include "../include/json.hpp"
#include "../include/protocol.hpp"
#include "../include/client_settings.hpp"

using nlohmann::json;
using boost::asio::ip::tcp;

void run_session_once(const std::string& host, int port) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve(host, std::to_string(port)));

    std::string handshake = proto::handshake_json("B").dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(handshake));

    boost::asio::streambuf sb;
    while (true) {
        boost::system::error_code ec;
        boost::asio::read_until(socket, sb, '\n', ec);
        if (ec) {
            break;
        }

        std::string line;
        std::istream is(&sb);
        std::getline(is, line);
        if (line.empty()) {
            continue;
        }

        json in = json::parse(line);
        proto::require_supported_version(in, "accel");
        float x = in["x"], y = in["y"], z = in["z"];
        double module = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y +
                                  static_cast<double>(z) * z);

        int64_t ts = in["timestamp"].get<int64_t>();
        std::string response = proto::module_json(ts, module).dump() + "\n";
        boost::asio::write(socket, boost::asio::buffer(response));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: clientB <host> <port> [client_config.ini]\n";
        return 1;
    }
    const std::string host = argv[1];
    const int port = std::atoi(argv[2]);
    const std::string cfg_path = (argc >= 4) ? argv[3] : "client_config.ini";
    const ClientSettings cfg = ClientSettings::load_or_default(cfg_path);

    while (true) {
        try {
            run_session_once(host, port);
        } catch (const std::exception& e) {
            std::cerr << "clientB: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(cfg.reconnect_delay);
    }
}

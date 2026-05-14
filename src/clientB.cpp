#include <iostream>
#include <cmath>
#include <boost/asio.hpp>
#include "../include/json.hpp"

using nlohmann::json;
using boost::asio::ip::tcp;

void run_client_b(const std::string& host, int port) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve(host, std::to_string(port)));

    // Хендшейк
    std::string handshake = json({{"client", "B"}}).dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(handshake));

    boost::asio::streambuf sb;
    while (true) {
        boost::system::error_code ec;
        boost::asio::read_until(socket, sb, '\n', ec);
        if (ec) break;

        std::string line;
        std::istream is(&sb);
        std::getline(is, line);
        if (line.empty()) continue;

        json in = json::parse(line);
        
        // САМ считает модуль (по ТЗ)
        float x = in["x"], y = in["y"], z = in["z"];
        double module = std::sqrt(x*x + y*y + z*z);

        json out = {{"timestamp", in["timestamp"]}, {"module", module}};
        std::string response = out.dump() + "\n";
        boost::asio::write(socket, boost::asio::buffer(response));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;
    run_client_b(argv[1], std::atoi(argv[2]));
    return 0;
}
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include "../include/json.hpp"

using nlohmann::json;
using boost::asio::ip::tcp;

void run_client_a(const std::string& host, int port) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve(host, std::to_string(port)));

    // Хендшейк
    std::string handshake = json({{"client", "A"}}).dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(handshake));

    // Поток для записи в файл
    std::thread log_thread([&socket]() {
        try {
            boost::asio::streambuf sb;
            while (true) {
                boost::asio::read_until(socket, sb, '\n');
                std::string line;
                std::istream is(&sb);
                std::getline(is, line);
                if (line.empty()) continue;

                json res = json::parse(line);
                std::filesystem::create_directories("accel");
                std::ofstream log("accel/module.log", std::ios::app);
                log << "[" << res["timestamp"] << "] Module: " << res["module"] << std::endl;
            }
        } catch (...) {}
    });

    // Цикл генерации: строго каждые 20 мс (50 Гц)
    while (true) {
        auto start = std::chrono::steady_clock::now();
        
        json packet = {
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"x", (float)(rand() % 100) / 10.0f},
            {"y", (float)(rand() % 100) / 10.0f},
            {"z", (float)(rand() % 100) / 10.0f}
        };

        std::string s = packet.dump() + "\n";
        boost::asio::write(socket, boost::asio::buffer(s));

        std::this_thread::sleep_until(start + std::chrono::milliseconds(20));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;
    run_client_a(argv[1], std::atoi(argv[2]));
    return 0;
}
#include <iostream>
#include <fstream>
#include <istream>
#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <chrono>
#include <filesystem>
#include "../include/json.hpp"

using nlohmann::json;
using boost::asio::ip::tcp;

namespace {

class ClientA_Session : public std::enable_shared_from_this<ClientA_Session> {
public:
    explicit ClientA_Session(tcp::socket&& socket)
        : socket_(std::move(socket)),
          tick_(socket_.get_executor()) {}

    void start() {
        do_read();
        schedule_tick();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        boost::asio::async_read_until(socket_, read_buf_, '\n',
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    return;
                }
                try {
                    std::string line;
                    std::istream is(&read_buf_);
                    std::getline(is, line);
                    if (line.empty()) {
                        do_read();
                        return;
                    }
                    json res = json::parse(line);
                    std::filesystem::create_directories("accel");
                    std::ofstream log("accel/module.log", std::ios::app);
                    log << "[" << res["timestamp"] << "] Module: " << res["module"] << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "clientA read: " << e.what() << std::endl;
                }
                do_read();
            });
    }

    void schedule_tick() {
        auto self = shared_from_this();
        tick_.expires_after(std::chrono::milliseconds(20));
        tick_.async_wait([this, self](boost::system::error_code ec) {
            if (ec) {
                return;
            }
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            json packet = {
                {"timestamp", static_cast<int64_t>(ms)},
                {"x", static_cast<float>(rand() % 100) / 10.0f},
                {"y", static_cast<float>(rand() % 100) / 10.0f},
                {"z", static_cast<float>(rand() % 100) / 10.0f}
            };
            enqueue_write(packet.dump() + "\n");
            schedule_tick();
        });
    }

    void enqueue_write(std::string msg) {
        write_q_.push_back(std::move(msg));
        do_write();
    }

    void do_write() {
        if (write_busy_ || write_q_.empty()) {
            return;
        }
        write_busy_ = true;
        auto self = shared_from_this();
        boost::asio::async_write(socket_, boost::asio::buffer(write_q_.front()),
            [this, self](boost::system::error_code ec, std::size_t) {
                write_busy_ = false;
                if (ec) {
                    std::cerr << "clientA write: " << ec.message() << std::endl;
                    write_q_.clear();
                    return;
                }
                write_q_.pop_front();
                do_write();
            });
    }

    tcp::socket socket_;
    boost::asio::steady_timer tick_;
    boost::asio::streambuf read_buf_;
    std::deque<std::string> write_q_;
    bool write_busy_{false};
};

}  // namespace

void run_client_a(const std::string& host, int port) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve(host, std::to_string(port)));

    std::string handshake = json({{"client", "A"}}).dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(handshake));

    std::make_shared<ClientA_Session>(std::move(socket))->start();
    io_context.run();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }
    run_client_a(argv[1], std::atoi(argv[2]));
    return 0;
}

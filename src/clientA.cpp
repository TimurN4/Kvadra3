#include <iostream>
#include <fstream>
#include <istream>
#include <memory>
#include <deque>
#include <thread>
#include <boost/asio.hpp>
#include <chrono>
#include <filesystem>
#include "../include/json.hpp"
#include "../include/domain.hpp"
#include "../include/protocol.hpp"
#include "../include/client_settings.hpp"

using nlohmann::json;
using boost::asio::ip::tcp;

namespace {

class ClientA_Session : public std::enable_shared_from_this<ClientA_Session> {
public:
    ClientA_Session(tcp::socket&& socket, boost::asio::io_context& ioc, std::chrono::milliseconds period)
        : socket_(std::move(socket)), ioc_(ioc), tick_(socket_.get_executor()), period_(period) {}

    void start() {
        do_read();
        schedule_tick();
    }

private:
    void stop_ioc_on_error(const boost::system::error_code& ec) {
        if (ec && ec != boost::asio::error::operation_aborted) {
            ioc_.stop();
        }
    }

    void do_read() {
        auto self = shared_from_this();
        boost::asio::async_read_until(socket_, read_buf_, '\n',
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    stop_ioc_on_error(ec);
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
                    proto::require_supported_version(res, "module reply");
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
        tick_.expires_after(period_);
        tick_.async_wait([this, self](boost::system::error_code ec) {
            if (ec) {
                stop_ioc_on_error(ec);
                return;
            }
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            AccelPacket p{static_cast<int64_t>(ms), static_cast<float>(rand() % 100) / 10.0f,
                          static_cast<float>(rand() % 100) / 10.0f,
                          static_cast<float>(rand() % 100) / 10.0f};
            enqueue_write(proto::accel_json(p).dump() + "\n");
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
                    stop_ioc_on_error(ec);
                    return;
                }
                write_q_.pop_front();
                do_write();
            });
    }

    tcp::socket socket_;
    boost::asio::io_context& ioc_;
    boost::asio::steady_timer tick_;
    boost::asio::streambuf read_buf_;
    std::deque<std::string> write_q_;
    bool write_busy_{false};
    std::chrono::milliseconds period_;
};

void run_session_once(const std::string& host, int port, const ClientSettings& cfg) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve(host, std::to_string(port)));

    std::string handshake = proto::handshake_json("A").dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(handshake));

    std::make_shared<ClientA_Session>(std::move(socket), io_context, cfg.sample_period)->start();
    io_context.run();
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: clientA <host> <port> [client_config.ini]\n";
        return 1;
    }
    const std::string host = argv[1];
    const int port = std::atoi(argv[2]);
    const std::string cfg_path = (argc >= 4) ? argv[3] : "client_config.ini";
    const ClientSettings cfg = ClientSettings::load_or_default(cfg_path);

    while (true) {
        try {
            run_session_once(host, port, cfg);
        } catch (const std::exception& e) {
            std::cerr << "clientA: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(cfg.reconnect_delay);
    }
}

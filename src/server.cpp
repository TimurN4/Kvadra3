
#include "../include/server.hpp"
#include <deque>
#include <optional>
#include <string_view>
#include <utility>

class Base_Session : public std::enable_shared_from_this<Base_Session> {
public:
    Base_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : server_(server), socket_ptr_(socket_ptr) {}

    virtual ~Base_Session() = default;

    void send_json(const json& j) {
        write_queue_.push_back(j.dump() + "\n");
        do_write();
    }

    virtual void do_read() = 0;
    
protected:
    virtual void on_transport_error() {}

    void do_write() {
        if (write_in_progress_ || write_queue_.empty()) {
            return;
        }
        write_in_progress_ = true;
        auto self = shared_from_this();
        boost::asio::async_write(
            *socket_ptr_,
            boost::asio::buffer(write_queue_.front()),
            [this, self](boost::system::error_code ec, std::size_t) {
                write_in_progress_ = false;
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                    write_queue_.clear();
                    on_transport_error();
                    return;
                }
                write_queue_.pop_front();
                do_write();
            });
    }

    Server& server_;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr_;
    boost::asio::streambuf read_buff_;

private:
    std::deque<std::string> write_queue_;
    bool write_in_progress_{false};
};

class A_Session final : public Base_Session {
public:
    A_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : Base_Session(socket_ptr, server) {}

    void do_read() override {
        auto self = shared_from_this();
        boost::asio::async_read_until(*socket_ptr_, read_buff_, '\n',
            [this, self](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (error) {
                    server_.on_a_disconnect(self);
                    return;
                }
                try {
                    std::string_view raw_data(
                        boost::asio::buffer_cast<const char*>(read_buff_.data()), 
                        bytes_transferred
                    );

                    json j = json::parse(raw_data);
                    
                    AccelPacket new_packet{
                        j["timestamp"].get<int64_t>(),
                        j["x"].get<float>(),
                        j["y"].get<float>(),
                        j["z"].get<float>()
                    };
                    
                    if(!last_packet_ || *last_packet_ != new_packet) {
                        last_packet_ = new_packet;
                        server_.deliver_to_b(last_packet_.value());
                    }

                } catch (const std::exception& e) {
                    std::cerr << "Ошибка А: " << e.what() << std::endl;
                }
                read_buff_.consume(bytes_transferred);
                do_read();
            }
        );
    }

private:
    std::optional<AccelPacket> last_packet_;

protected:
    void on_transport_error() override {
        server_.on_a_disconnect(shared_from_this());
    }
};

class B_Session final : public Base_Session {
public:
    B_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : Base_Session(socket_ptr, server) {}

    void do_read() override {
        auto self = shared_from_this();
        boost::asio::async_read_until(*socket_ptr_, read_buff_, '\n',
            [this, self](const boost::system::error_code& ec, std::size_t bytes) {
                if (ec) {
                    server_.on_b_disconnect(self);
                    return;
                }
                try {
                    std::string_view raw(boost::asio::buffer_cast<const char*>(read_buff_.data()), bytes);
                    json j = json::parse(raw);
                    
                    server_.deliver_to_a(j); 
                } catch (const std::exception& e) {
                    std::cerr << "Ошибка B: " << e.what() << std::endl;
                }
                read_buff_.consume(bytes);
                do_read();
            });
    }

protected:
    void on_transport_error() override {
        server_.on_b_disconnect(shared_from_this());
    }
};


Server::Server(boost::asio::io_context& context, int PORT) 
    : acceptor_(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT)) {
    start_handshake();
}

void Server::deliver_to_b(AccelPacket new_packet) {
    if(b_session_) {
        b_session_->send_json({
            {"timestamp", new_packet.timestamp}, 
            {"x", new_packet.x}, 
            {"y", new_packet.y}, 
            {"z", new_packet.z}
        });
    } else {
        to_deliver_b_.push_back(std::move(new_packet));
    }
}

void Server::deliver_to_a(const json& res) {
    if (a_session_) {
        a_session_->send_json(res);
    } else {
        to_deliver_a_.push_back(res);
    }
}

void Server::on_a_disconnect(const std::shared_ptr<Base_Session>& session) {
    if (a_session_ && std::static_pointer_cast<Base_Session>(a_session_) == session) {
        a_session_.reset();
    }
}

void Server::on_b_disconnect(const std::shared_ptr<Base_Session>& session) {
    if (b_session_ && std::static_pointer_cast<Base_Session>(b_session_) == session) {
        b_session_.reset();
    }
}

void Server::start_handshake() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if(!ec) {
                identify(socket);
            }
            start_handshake();
        }
    );
}

void Server::identify(boost::asio::ip::tcp::socket& socket) {
    auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
    auto buff_ptr = std::make_shared<boost::asio::streambuf>();
    
    boost::asio::async_read_until(*socket_ptr, *buff_ptr, '\n',
        [this, socket_ptr, buff_ptr](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                try {
                    std::string_view raw_data(boost::asio::buffer_cast<const char*>(buff_ptr->data()), bytes_transferred);
                    json j = json::parse(raw_data);
                    
                    if(j.at("client") == "A") {
                        a_session_ = std::make_shared<A_Session>(socket_ptr, *this);
                        a_session_->do_read();

                        auto pending_to_a = std::move(to_deliver_a_);
                        for (const auto& msg : pending_to_a) {
                            a_session_->send_json(msg);
                        }
                    } else if(j.at("client") == "B") {
                        b_session_ = std::make_shared<B_Session>(socket_ptr, *this);
                        b_session_->do_read();

                        auto pending_to_b = std::move(to_deliver_b_);
                        for (const auto& packet : pending_to_b) {
                            b_session_->send_json({
                                {"timestamp", packet.timestamp},
                                {"x", packet.x},
                                {"y", packet.y},
                                {"z", packet.z}
                            });
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Handshake: " << e.what() << std::endl;
                }
                buff_ptr->consume(bytes_transferred);
            }
        }
    );
}

int main(int argc, char* argv[]) {
    if(argc < 2) return 1;
    const int PORT = std::atoi(argv[1]);

    boost::asio::io_context context;
    Server server(context, PORT);
    context.run();

    return 0;
}
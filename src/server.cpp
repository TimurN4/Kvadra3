
#include "../include/server.hpp"

class Base_Session : public std::enable_shared_from_this<Base_Session> {
public:
    Base_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : server_(server), socket_ptr_(socket_ptr) {}

    virtual ~Base_Session() = default;

    void send_json(const json& j) {
        auto self = shared_from_this();
        std::string data = j.dump() + "\n";
        boost::asio::async_write(*socket_ptr_, boost::asio::buffer(data),
            [self](boost::system::error_code ec, std::size_t) {
                if (ec) std::cerr << "Write error: " << ec.message() << std::endl;
            }
        );
    }

    virtual void do_read() = 0;
    
protected:
    Server& server_;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr_;
    boost::asio::streambuf read_buff_;
};

class A_Session final : public Base_Session {
public:
    A_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : Base_Session(socket_ptr, server) {}

    void do_read() override {
        auto self = shared_from_this();
        boost::asio::async_read_until(*socket_ptr_, read_buff_, '\n',
            [this, self](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error) {
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
                            // Напрямую вызываем метод сервера
                            server_.deliver_to_b(last_packet_.value());
                        }

                    } catch (const std::exception& e) {
                        std::cerr << "Ошибка А: " << e.what() << std::endl;
                    }
                    read_buff_.consume(bytes_transferred);
                    do_read();
                }
            }
        );
    }

private:
    std::optional<AccelPacket> last_packet_;
};

class B_Session final : public Base_Session {
public:
    B_Session(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr, Server& server) 
        : Base_Session(socket_ptr, server) {}

    void do_read() override {
        auto self = shared_from_this();
        boost::asio::async_read_until(*socket_ptr_, read_buff_, '\n',
            [this, self](const boost::system::error_code& ec, std::size_t bytes) {
                if (!ec) {
                    try {
                        std::string_view raw(boost::asio::buffer_cast<const char*>(read_buff_.data()), bytes);
                        json j = json::parse(raw);
                        
                        server_.deliver_to_a(j); 
                    } catch (...) {}
                    read_buff_.consume(bytes);
                    do_read();
                }
            });
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
    if(a_session_) {
        a_session_->send_json(res);
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
                        // Передаем *this в конструктор
                        a_session_ = std::make_shared<A_Session>(socket_ptr, *this);
                        a_session_->do_read();
                    } else if(j.at("client") == "B") {
                        b_session_ = std::make_shared<B_Session>(socket_ptr, *this);
                        b_session_->do_read();

                        for(const auto& packet : to_deliver_b_) {
                            deliver_to_b(packet);
                        }
                        to_deliver_b_.clear();
                    }
                } catch (...) {}
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
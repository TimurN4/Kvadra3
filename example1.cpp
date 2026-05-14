#include <boost/asio.hpp>
#include <iostream>
#include <memory>

using namespace boost::asio;
using ip::tcp;

// Объект сессии для каждого клиента
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read(); }

private:
    void do_read() {
        auto self(shared_from_this());
        // Асинхронно ждем данные. Поток НЕ блокируется здесь!
        socket_.async_read_some(buffer(data_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout << "Получено: " << std::string(data_, length) << std::endl;
                    do_write(length); // Эхо-ответ
                }
            });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        async_write(socket_, buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) do_read(); // Снова ждем данные
            });
    }

    tcp::socket socket_;
    char data_[1024];
};

class Server {
public:
    Server(io_context& context, short port)
        : acceptor_(context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        // Ждем нового клиента асинхронно
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Создаем сессию и запускаем её. 
                    // Теперь этот сокет "живет" внутри Session.
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept(); // Сразу готовы принять следующего
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    io_context context;
    Server s(context, 8080);
    context.run(); // Запуск цикла обработки событий
    return 0;
}
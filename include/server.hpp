#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <cmath>
#include <boost/asio.hpp>

#include "../include/domain.hpp"
#include "../include/json.hpp"

using json = nlohmann::json;

class Base_Session;
class A_Session;
class B_Session;

class Server final {
public:
    Server(boost::asio::io_context& context, int PORT);

    void deliver_to_b(AccelPacket new_packet);

    void deliver_to_a(const json& res);

    void on_a_disconnect(const std::shared_ptr<Base_Session>& session);

    void on_b_disconnect(const std::shared_ptr<Base_Session>& session);

private:
    void start_handshake();

    void identify(boost::asio::ip::tcp::socket& socket);

    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<A_Session> a_session_;
    std::shared_ptr<B_Session> b_session_;
    std::vector<AccelPacket> to_deliver_b_;
    std::vector<json> to_deliver_a_;
};

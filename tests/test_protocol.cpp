#include "../include/protocol.hpp"
#include <cassert>
#include <iostream>

int main() {
    using nlohmann::json;

    assert(proto::version_from_json(json::object()) == proto::kVersion);
    assert(proto::version_from_json(json{{"version", 1}}) == 1);

    bool threw = false;
    try {
        proto::require_supported_version(json{{"version", 2}}, "test");
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);

    AccelPacket p{123, 1.f, 2.f, 3.f};
    json ja = proto::accel_json(p);
    assert(ja["version"] == proto::kVersion);
    AccelPacket p2 = proto::parse_accel_packet(ja);
    assert(p2.timestamp == 123 && p2.x == 1.f);

    json jm = proto::module_json(10, 9.81);
    assert(jm["version"] == proto::kVersion);
    proto::require_supported_version(jm, "mod");

    std::cout << "test_protocol: OK\n";
    return 0;
}

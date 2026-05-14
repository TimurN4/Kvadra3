#pragma once

#include <stdexcept>
#include <string>
#include "domain.hpp"
#include "json.hpp"

namespace proto {

constexpr int kVersion = 1;

inline int version_from_json(const nlohmann::json& j) {
    if (!j.contains("version")) {
        return kVersion;
    }
    return j.at("version").get<int>();
}

inline bool is_supported_version(int v) { return v == kVersion; }

inline void require_supported_version(const nlohmann::json& j, const char* ctx) {
    const int v = version_from_json(j);
    if (!is_supported_version(v)) {
        throw std::runtime_error(std::string(ctx) + ": unsupported protocol version " + std::to_string(v));
    }
}

inline nlohmann::json handshake_json(const std::string& client_letter) {
    return nlohmann::json{{"client", client_letter}, {"version", kVersion}};
}

inline nlohmann::json accel_json(const AccelPacket& p) {
    return nlohmann::json{{"version", kVersion},
                          {"timestamp", p.timestamp},
                          {"x", p.x},
                          {"y", p.y},
                          {"z", p.z}};
}

inline nlohmann::json module_json(int64_t timestamp_ms, double module) {
    return nlohmann::json{
        {"version", kVersion}, {"timestamp", timestamp_ms}, {"module", module}};
}

inline AccelPacket parse_accel_packet(const nlohmann::json& j) {
    require_supported_version(j, "accel");
    return AccelPacket{j.at("timestamp").get<int64_t>(), j.at("x").get<float>(), j.at("y").get<float>(),
                     j.at("z").get<float>()};
}

}  // namespace proto

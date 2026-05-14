#include "../include/client_settings.hpp"
#include <sstream>

namespace {

void trim_inplace(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    s.erase(0, i);
}

}  // namespace

ClientSettings ClientSettings::load_or_default(const std::string& path) {
    ClientSettings out;
    std::ifstream in(path);
    if (!in) {
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        trim_inplace(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        trim_inplace(key);
        trim_inplace(val);
        try {
            if (key == "sample_period_ms") {
                out.sample_period = std::chrono::milliseconds(std::stoi(val));
            } else if (key == "reconnect_delay_ms") {
                out.reconnect_delay = std::chrono::milliseconds(std::stoi(val));
            }
        } catch (...) {
        }
    }
    if (out.sample_period.count() < 1) {
        out.sample_period = std::chrono::milliseconds(20);
    }
    if (out.reconnect_delay.count() < 1) {
        out.reconnect_delay = std::chrono::milliseconds(1500);
    }
    return out;
}

#pragma once

#include <chrono>
#include <fstream>
#include <string>

/// RAII-настройки клиента из простого key=value файла (см. client_config.ini).
struct ClientSettings {
    std::chrono::milliseconds sample_period{20};
    std::chrono::milliseconds reconnect_delay{1500};

    static ClientSettings load_or_default(const std::string& path);
};

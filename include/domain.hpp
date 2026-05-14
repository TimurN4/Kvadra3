#pragma once
#include <cmath>

struct AccelPacket {    
    int64_t timestamp;
    float x;
    float y;
    float z;

    bool operator==(const AccelPacket& other) const {
        return std::abs(other.x - x) < 0.0001 && 
               std::abs(other.y - y) < 0.0001 && 
               std::abs(other.z - z) < 0.0001;
    }

    bool operator!=(const AccelPacket& other) const {
        return !(*this == other);
    }
};
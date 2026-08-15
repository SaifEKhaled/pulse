// src/storage/point.h

// See docs/rfcs/001-storage-api.md for the full design reasoning behind

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace pulse::storage {

struct Point {
    std::string metric;
    std::string job_id;  
    std::unordered_map<std::string, std::string> tags;  
    int64_t timestamp;      
    double value;
};

}  // namespace pulse::storage
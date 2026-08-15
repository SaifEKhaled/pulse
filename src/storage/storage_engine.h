// src/storage/storage_engine.h

#pragma once

#include "point.h"

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cstdint>

namespace pulse::storage {

// Result of a write operation. check the rfc 001-storage-api.md for the reasoning (why instead of throwing exceptions)
struct Result {
    bool ok;
    std::string error_message;  // empty if ok == true

    static Result success() {
        return Result{true, ""};
    }
    static Result failure(std::string message) {
        return Result{false, std::move(message)};
    }
};

// Abstract interface. A class with only pure virtual functions (the "= 0" ones below) 
// acts like an enforcer of knowledge on how to pass the Read() and Write() operations
class StorageEngine { //any storage implementation must provide the 2 operations of write and read
public:
    virtual ~StorageEngine() = default;

    // Writes a single point (not a batch as specified)
    virtual Result write(const Point& point) = 0;

    // Reads all points matching the given filters within [start_ts, end_ts]. again. check the rfc 001-storage-api.md for the reasoning
    virtual std::vector<Point> read(
        const std::string& metric,
        const std::optional<std::string>& job_id,
        const std::unordered_map<std::string, std::string>& tags_filter,
        int64_t start_ts,
        int64_t end_ts
    ) = 0;
};

}  // namespace pulse::storage
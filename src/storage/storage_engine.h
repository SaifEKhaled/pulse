// src/storage/storage_engine.h
//


#pragma once

#include "point.h"

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cstdint>

namespace pulse::storage {

struct Result {
    bool ok;
    std::string error_message;

    static Result success() {
        return Result{true, ""};
    }
    static Result failure(std::string message) {
        return Result{false, std::move(message)};
    }
};

class StorageEngine {
public:
    virtual ~StorageEngine() = default;

    virtual Result write(const Point& point) = 0;

    virtual std::vector<Point> read(
        const std::string& metric,
        const std::optional<std::string>& job_id,
        const std::unordered_map<std::string, std::string>& tags_filter,
        int64_t start_ts,
        int64_t end_ts
    ) = 0;
};

}  // namespace pulse::storage
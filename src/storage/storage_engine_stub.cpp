// src/storage/storage_engine_stub.cpp

#include "storage_engine_stub.h"

namespace pulse::storage {

Result StorageEngineStub::write(const Point& point) {
    // no validation, no durability, no failure modes yet 
    // just append to the in-memory vector.
    points_.push_back(point);
    return Result::success();
}

std::vector<Point> StorageEngineStub::read(
    const std::string& metric,
    const std::optional<std::string>& job_id,
    const std::unordered_map<std::string, std::string>& tags_filter,
    int64_t start_ts,
    int64_t end_ts
) {
    std::vector<Point> results;

    // linear scan over every point ever written "correct but inefficent approach for now"
    for (const auto& p : points_) {
        if (p.metric != metric) {
            continue;
        }
        if (p.timestamp < start_ts || p.timestamp > end_ts) {
            continue;
        }
        if (job_id.has_value() && p.job_id != job_id.value()) {
            continue;
        }

        // the filter only constrains what's explicitly asked for
        bool tags_match = true;
        for (const auto& [key, value] : tags_filter) {
            auto it = p.tags.find(key);
            if (it == p.tags.end() || it->second != value) {
                tags_match = false;
                break;
            }
        }
        if (!tags_match) {
            continue;
        }

        results.push_back(p);
    }

    return results;
}

}  // namespace pulse::storage
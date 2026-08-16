// src/storage/storage_engine_stub.h
//
// in-memory implementation of StorageEngine. there's no persistence, no indexing, 
// and no optimization. Exists purely to unblock the query engine so it can be built
// and tested against a real, working StorageEngine before the durable engine is ready.

#pragma once

#include "storage_engine.h"

#include <vector>

namespace pulse::storage {

class StorageEngineStub : public StorageEngine {
public:
    Result write(const Point& point) override;

    std::vector<Point> read(
        const std::string& metric,
        const std::optional<std::string>& job_id,
        const std::unordered_map<std::string, std::string>& tags_filter,
        int64_t start_ts,
        int64_t end_ts
    ) override;

private:
    std::vector<Point> points_;
};

}  // namespace pulse::storage
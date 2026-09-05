#pragma once

#include "throughput/domain.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace throughput {

struct WorkloadConfig {
    std::size_t records{10'000};
    std::uint64_t seed{0x5eed1234ull};
    std::uint32_t oversized_percent{1};
    std::uint32_t escaping_percent{0};
    std::uint32_t max_attributes{8};
    std::uint32_t max_relationships{4};
    bool skewed{true};
};

struct EncodedBatch {
    WorkloadConfig config;
    std::vector<EncodedEvent> events;
    std::size_t total_bytes{};
};

EncodedBatch generate_batch(const WorkloadConfig& config);

} // namespace throughput


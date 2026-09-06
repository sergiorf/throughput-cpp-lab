#pragma once

#include "financial/common/model.hpp"

#include <chrono>
#include <span>

namespace financial::par {

struct ParallelConfig {
    std::size_t worker_count{4};
    std::size_t queue_capacity{256};
};

struct QueueMetrics {
    std::size_t producer_waits{};
    std::size_t consumer_waits{};
    std::chrono::nanoseconds producer_wait_time{};
    std::chrono::nanoseconds consumer_wait_time{};
    std::size_t max_occupancy{};
};

struct ParallelOutput {
    financial::common::PipelineOutput pipeline;
    QueueMetrics queue;
    std::size_t worker_count{};
    std::size_t queue_capacity{};
};

[[nodiscard]] ParallelOutput process_records(
    std::span<const financial::common::EncodedRecord> input,
    const financial::common::ReferenceData& reference,
    ParallelConfig config = {});

} // namespace financial::par

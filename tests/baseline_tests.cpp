#include "test_support.hpp"

#include "throughput/generator.hpp"
#include "throughput/instrumentation.hpp"
#include "throughput/pipeline_baseline.hpp"
#include "throughput/reference_data.hpp"

namespace {

throughput::WorkloadConfig small_config()
{
    return {
        .records = 256,
        .seed = 0xabc123ull,
        .oversized_percent = 3,
        .escaping_percent = 0,
        .max_attributes = 6,
        .max_relationships = 3,
        .skewed = true,
    };
}

} // namespace

THROUGHPUT_TEST(generator_is_deterministic)
{
    const auto first = throughput::generate_batch(small_config());
    const auto second = throughput::generate_batch(small_config());

    THROUGHPUT_REQUIRE_EQ(first.events.size(), second.events.size());
    THROUGHPUT_REQUIRE_EQ(first.total_bytes, second.total_bytes);
    THROUGHPUT_REQUIRE(!first.events.empty());
    THROUGHPUT_REQUIRE_EQ(first.events.front().bytes, second.events.front().bytes);
    THROUGHPUT_REQUIRE_EQ(first.events.back().bytes, second.events.back().bytes);
}

THROUGHPUT_TEST(baseline_processes_every_record)
{
    const auto reference = throughput::ReferenceData::make_default();
    const auto batch = throughput::generate_batch(small_config());
    const auto result = throughput::baseline::process_batch(batch, reference);

    THROUGHPUT_REQUIRE_EQ(result.normalized.size(), batch.events.size());
    THROUGHPUT_REQUIRE_EQ(result.audit.size(), batch.events.size());
    THROUGHPUT_REQUIRE(result.checksum != 0);
}

THROUGHPUT_TEST(baseline_checksum_is_stable)
{
    const auto reference = throughput::ReferenceData::make_default();
    const auto batch = throughput::generate_batch(small_config());

    const auto first = throughput::baseline::process_batch(batch, reference);
    const auto second = throughput::baseline::process_batch(batch, reference);

    THROUGHPUT_REQUIRE_EQ(first.checksum, second.checksum);
}

THROUGHPUT_TEST(allocation_instrumentation_observes_baseline_work)
{
    const auto reference = throughput::ReferenceData::make_default();
    const auto batch = throughput::generate_batch(small_config());

    throughput::reset_allocation_counters();
    const throughput::AllocationScope scope;
    const auto result = throughput::baseline::process_batch(batch, reference);
    const auto allocations = scope.snapshot();

    THROUGHPUT_REQUIRE(result.checksum != 0);
    THROUGHPUT_REQUIRE(allocations.allocations > 0);
    THROUGHPUT_REQUIRE(allocations.allocated_bytes > 0);
}


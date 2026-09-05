#include "throughput/generator.hpp"
#include "throughput/instrumentation.hpp"
#include "throughput/reference_data.hpp"
#include "throughput/stages.hpp"

#include <chrono>
#include <array>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {

struct ScenarioStage {
    std::string_view name;
    throughput::SemanticResult (*run)(const throughput::EncodedBatch&, const throughput::ReferenceData&);
};

int run_stage(
    const ScenarioStage& stage,
    const throughput::EncodedBatch& batch,
    const throughput::ReferenceData& reference)
{
    using clock = std::chrono::steady_clock;

    throughput::reset_allocation_counters();
    const throughput::AllocationScope allocations;
    const auto start = clock::now();
    const auto result = stage.run(batch, reference);
    const auto stop = clock::now();
    const auto allocation_snapshot = allocations.snapshot();

    const auto elapsed = std::chrono::duration<double>(stop - start).count();
    const auto records_per_second = static_cast<double>(batch.events.size()) / elapsed;

    std::cout << "stage=" << stage.name << '\n';
    std::cout << "records=" << batch.events.size() << '\n';
    std::cout << "input_bytes=" << batch.total_bytes << '\n';
    std::cout << "elapsed_seconds=" << std::fixed << std::setprecision(6) << elapsed << '\n';
    std::cout << "records_per_second=" << std::fixed << std::setprecision(2) << records_per_second << '\n';
    std::cout << "allocations=" << allocation_snapshot.allocations << '\n';
    std::cout << "allocated_bytes=" << allocation_snapshot.allocated_bytes << '\n';
    std::cout << "deallocations=" << allocation_snapshot.deallocations << '\n';
    std::cout << "deallocated_bytes=" << allocation_snapshot.deallocated_bytes << '\n';
    std::cout << "normalized_records=" << result.normalized_records << '\n';
    std::cout << "audit_records=" << result.audit_records << '\n';
    std::cout << "checksum=" << result.checksum << "\n\n";

    return result.normalized_records == batch.events.size() && result.audit_records == batch.events.size() ? 0 : 1;
}

} // namespace

int main()
{
    const throughput::WorkloadConfig config{
        .records = 10'000,
        .seed = 0x5eed1234ull,
        .oversized_percent = 1,
        .escaping_percent = 0,
        .max_attributes = 8,
        .max_relationships = 4,
        .skewed = true,
    };

    const auto reference = throughput::ReferenceData::make_default();
    const auto batch = throughput::generate_batch(config);

    constexpr std::array stages{
        ScenarioStage{"baseline", throughput::stages::baseline},
        ScenarioStage{"reserved", throughput::stages::reserved},
        ScenarioStage{"pmr", throughput::stages::pmr},
        ScenarioStage{"arena", throughput::stages::arena},
    };

    int status = 0;
    for (const auto& stage : stages) {
        status |= run_stage(stage, batch, reference);
    }
    return status;
}

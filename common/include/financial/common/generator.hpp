#pragma once

#include "financial/common/model.hpp"

namespace financial::common {

[[nodiscard]] Workload generate_workload(const WorkloadConfig& config);
[[nodiscard]] RawFinancialRecord make_edge_case_record(std::uint64_t sequence);

} // namespace financial::common

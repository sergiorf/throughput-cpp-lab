#pragma once

#include "financial/common/model.hpp"

#include <span>

namespace financial::oracle {

[[nodiscard]] financial::common::PipelineOutput process_records(
    std::span<const financial::common::EncodedRecord> input,
    const financial::common::ReferenceData& reference);

} // namespace financial::oracle

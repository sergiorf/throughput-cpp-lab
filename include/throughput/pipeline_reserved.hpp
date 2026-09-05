#pragma once

#include "throughput/domain.hpp"
#include "throughput/generator.hpp"
#include "throughput/reference_data.hpp"

namespace throughput::reserved {

PipelineResult process_batch(const EncodedBatch& batch, const ReferenceData& reference);

} // namespace throughput::reserved

#pragma once

#include "throughput/generator.hpp"
#include "throughput/reference_data.hpp"
#include "throughput/semantic_result.hpp"

namespace throughput::stages {

SemanticResult baseline(const EncodedBatch& batch, const ReferenceData& reference);
SemanticResult reserved(const EncodedBatch& batch, const ReferenceData& reference);
SemanticResult pmr(const EncodedBatch& batch, const ReferenceData& reference);
SemanticResult arena(const EncodedBatch& batch, const ReferenceData& reference);

} // namespace throughput::stages

#pragma once

#include <cstddef>
#include <cstdint>

namespace throughput {

struct SemanticResult {
    std::size_t normalized_records{};
    std::size_t audit_records{};
    std::uint64_t checksum{};
};

} // namespace throughput

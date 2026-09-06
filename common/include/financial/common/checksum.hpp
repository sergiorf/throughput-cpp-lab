#pragma once

#include "financial/common/model.hpp"

#include <cstdint>
#include <string_view>

namespace financial::common {

[[nodiscard]] std::uint64_t checksum_records(const std::vector<CanonicalRecord>& records);
void checksum_append(std::uint64_t& state, std::string_view text) noexcept;
void checksum_append(std::uint64_t& state, std::uint64_t value) noexcept;
void checksum_append(std::uint64_t& state, std::int64_t value) noexcept;

} // namespace financial::common

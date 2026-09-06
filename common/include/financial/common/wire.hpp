#pragma once

#include "financial/common/model.hpp"

#include <expected>
#include <span>
#include <string>
#include <vector>

namespace financial::common {

enum class WireError {
    truncated,
    invalid_length,
    invalid_number,
    trailing_bytes,
};

[[nodiscard]] EncodedRecord encode_record(const RawFinancialRecord& record);
[[nodiscard]] std::expected<RawFinancialRecord, WireError> decode_record(std::span<const std::byte> payload);

[[nodiscard]] std::vector<std::byte> frame_payload(std::span<const std::byte> payload);
[[nodiscard]] std::expected<std::vector<EncodedRecord>, WireError> decode_frames(
    std::span<const std::byte> bytes,
    std::uint32_t max_message_size);

[[nodiscard]] std::string wire_error_name(WireError error);

} // namespace financial::common

#include "financial/common/wire.hpp"

#include <cstring>
#include <limits>

namespace financial::common {
namespace {

template <class T>
void append_plain(std::vector<std::byte>& out, T value)
{
    const auto* ptr = reinterpret_cast<const std::byte*>(&value);
    out.insert(out.end(), ptr, ptr + sizeof(T));
}

template <class T>
std::expected<T, WireError> read_plain(std::span<const std::byte> bytes, std::size_t& offset)
{
    if (bytes.size() - offset < sizeof(T)) {
        return std::unexpected(WireError::truncated);
    }
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

void append_string(std::vector<std::byte>& out, std::string_view value)
{
    append_plain(out, static_cast<std::uint32_t>(value.size()));
    const auto* ptr = reinterpret_cast<const std::byte*>(value.data());
    out.insert(out.end(), ptr, ptr + value.size());
}

std::expected<std::string, WireError> read_string(std::span<const std::byte> bytes, std::size_t& offset)
{
    const auto length = read_plain<std::uint32_t>(bytes, offset);
    if (!length) {
        return std::unexpected(length.error());
    }
    if (*length > bytes.size() - offset) {
        return std::unexpected(WireError::truncated);
    }
    std::string value(reinterpret_cast<const char*>(bytes.data() + offset), *length);
    offset += *length;
    return value;
}

void append_money(std::vector<std::byte>& out, Money value)
{
    append_plain(out, static_cast<std::uint8_t>(value.value.has_value() ? 1 : 0));
    append_plain(out, value.value.value_or(0.0));
}

std::expected<Money, WireError> read_money(std::span<const std::byte> bytes, std::size_t& offset)
{
    const auto present = read_plain<std::uint8_t>(bytes, offset);
    if (!present) {
        return std::unexpected(present.error());
    }
    const auto value = read_plain<double>(bytes, offset);
    if (!value) {
        return std::unexpected(value.error());
    }
    if (*present > 1) {
        return std::unexpected(WireError::invalid_number);
    }
    return Money{*present == 1 ? std::optional<double>{*value} : std::nullopt};
}

#define READ_STRING_FIELD(target) \
    do { \
        auto field = read_string(payload, offset); \
        if (!field) { \
            return std::unexpected(field.error()); \
        } \
        target = std::move(*field); \
    } while (false)

#define READ_MONEY_FIELD(target) \
    do { \
        auto field = read_money(payload, offset); \
        if (!field) { \
            return std::unexpected(field.error()); \
        } \
        target = *field; \
    } while (false)

} // namespace

EncodedRecord encode_record(const RawFinancialRecord& record)
{
    std::vector<std::byte> out;
    out.reserve(256 + record.legal_name.size() + record.address.size());
    append_plain(out, record.sequence);
    append_string(out, record.company_id);
    append_string(out, record.legal_name);
    append_string(out, record.address);
    append_string(out, record.postal_code);
    append_string(out, record.country_code);
    append_string(out, record.currency_code);
    append_string(out, record.industry_code);
    append_string(out, record.reporting_period);
    append_money(out, record.revenue);
    append_money(out, record.operating_profit);
    append_money(out, record.net_income);
    append_money(out, record.current_assets);
    append_money(out, record.current_liabilities);
    append_money(out, record.total_assets);
    append_money(out, record.total_debt);
    append_money(out, record.shareholder_equity);
    append_money(out, record.previous_revenue);
    append_money(out, record.previous_profit);
    return EncodedRecord{std::move(out)};
}

std::expected<RawFinancialRecord, WireError> decode_record(std::span<const std::byte> payload)
{
    std::size_t offset = 0;
    RawFinancialRecord record;
    const auto sequence = read_plain<std::uint64_t>(payload, offset);
    if (!sequence) {
        return std::unexpected(sequence.error());
    }
    record.sequence = *sequence;
    READ_STRING_FIELD(record.company_id);
    READ_STRING_FIELD(record.legal_name);
    READ_STRING_FIELD(record.address);
    READ_STRING_FIELD(record.postal_code);
    READ_STRING_FIELD(record.country_code);
    READ_STRING_FIELD(record.currency_code);
    READ_STRING_FIELD(record.industry_code);
    READ_STRING_FIELD(record.reporting_period);
    READ_MONEY_FIELD(record.revenue);
    READ_MONEY_FIELD(record.operating_profit);
    READ_MONEY_FIELD(record.net_income);
    READ_MONEY_FIELD(record.current_assets);
    READ_MONEY_FIELD(record.current_liabilities);
    READ_MONEY_FIELD(record.total_assets);
    READ_MONEY_FIELD(record.total_debt);
    READ_MONEY_FIELD(record.shareholder_equity);
    READ_MONEY_FIELD(record.previous_revenue);
    READ_MONEY_FIELD(record.previous_profit);
    if (offset != payload.size()) {
        return std::unexpected(WireError::trailing_bytes);
    }
    return record;
}

std::vector<std::byte> frame_payload(std::span<const std::byte> payload)
{
    std::vector<std::byte> out;
    out.reserve(sizeof(std::uint32_t) + payload.size());
    append_plain(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::expected<std::vector<EncodedRecord>, WireError> decode_frames(
    std::span<const std::byte> bytes,
    std::uint32_t max_message_size)
{
    std::vector<EncodedRecord> records;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto length = read_plain<std::uint32_t>(bytes, offset);
        if (!length) {
            return std::unexpected(length.error());
        }
        if (*length > max_message_size) {
            return std::unexpected(WireError::invalid_length);
        }
        if (*length > bytes.size() - offset) {
            return std::unexpected(WireError::truncated);
        }
        auto* begin = bytes.data() + offset;
        records.push_back(EncodedRecord{{begin, begin + *length}});
        offset += *length;
    }
    return records;
}

std::string wire_error_name(WireError error)
{
    switch (error) {
    case WireError::truncated:
        return "truncated";
    case WireError::invalid_length:
        return "invalid_length";
    case WireError::invalid_number:
        return "invalid_number";
    case WireError::trailing_bytes:
        return "trailing_bytes";
    }
    return "unknown";
}

} // namespace financial::common

#include "financial/common/checksum.hpp"

#include <bit>

namespace financial::common {
namespace {

constexpr std::uint64_t fnv_offset = 14695981039346656037ull;
constexpr std::uint64_t fnv_prime = 1099511628211ull;

void append_byte(std::uint64_t& state, std::byte b) noexcept
{
    state ^= static_cast<std::uint8_t>(b);
    state *= fnv_prime;
}

} // namespace

void checksum_append(std::uint64_t& state, std::string_view text) noexcept
{
    checksum_append(state, static_cast<std::uint64_t>(text.size()));
    for (const char ch : text) {
        append_byte(state, static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}

void checksum_append(std::uint64_t& state, std::uint64_t value) noexcept
{
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        append_byte(state, static_cast<std::byte>((value >> (i * 8u)) & 0xffu));
    }
}

void checksum_append(std::uint64_t& state, std::int64_t value) noexcept
{
    checksum_append(state, std::bit_cast<std::uint64_t>(value));
}

std::uint64_t checksum_records(const std::vector<CanonicalRecord>& records)
{
    std::uint64_t state = fnv_offset;
    checksum_append(state, static_cast<std::uint64_t>(records.size()));
    for (const auto& record : records) {
        checksum_append(state, record.sequence);
        checksum_append(state, record.company_id);
        checksum_append(state, record.legal_name);
        checksum_append(state, record.address);
        checksum_append(state, record.postal_code);
        checksum_append(state, record.country_code);
        checksum_append(state, record.country_name);
        checksum_append(state, record.region);
        checksum_append(state, record.currency_code);
        checksum_append(state, static_cast<std::uint64_t>(record.currency_decimals));
        checksum_append(state, record.industry_code);
        checksum_append(state, record.industry_name);
        checksum_append(state, record.operating_margin_bp);
        checksum_append(state, record.net_margin_bp);
        checksum_append(state, record.current_ratio_bp);
        checksum_append(state, record.debt_to_equity_bp);
        checksum_append(state, record.return_on_assets_bp);
        checksum_append(state, record.revenue_growth_bp);
        checksum_append(state, record.profit_growth_bp);
        checksum_append(state, static_cast<std::uint64_t>(record.validation_flags));
        checksum_append(state, record.risk_class);
    }
    return state;
}

} // namespace financial::common

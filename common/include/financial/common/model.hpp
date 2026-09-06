#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace financial::common {

struct Money {
    std::optional<double> value;
};

struct RawFinancialRecord {
    std::uint64_t sequence{};
    std::string company_id;
    std::string legal_name;
    std::string address;
    std::string postal_code;
    std::string country_code;
    std::string currency_code;
    std::string industry_code;
    std::string reporting_period;
    Money revenue;
    Money operating_profit;
    Money net_income;
    Money current_assets;
    Money current_liabilities;
    Money total_assets;
    Money total_debt;
    Money shareholder_equity;
    Money previous_revenue;
    Money previous_profit;
};

struct EncodedRecord {
    std::vector<std::byte> payload;
};

struct CountryRef {
    std::string code;
    std::string name;
    std::string region;
    double risk{};
    std::string canonical_currency;
};

struct CurrencyRef {
    std::string code;
    std::uint8_t decimals{};
};

struct IndustryRef {
    std::string code;
    std::string name;
    double risk{};
};

struct ReferenceData {
    std::vector<CountryRef> countries;
    std::vector<CurrencyRef> currencies;
    std::vector<IndustryRef> industries;

    [[nodiscard]] static ReferenceData make_default();
    [[nodiscard]] const CountryRef* country(std::string_view code) const noexcept;
    [[nodiscard]] const CurrencyRef* currency(std::string_view code) const noexcept;
    [[nodiscard]] const IndustryRef* industry(std::string_view code) const noexcept;
};

enum class ValidationFlag : std::uint32_t {
    none = 0,
    missing_required_text = 1u << 0u,
    unknown_country = 1u << 1u,
    unknown_currency = 1u << 2u,
    unknown_industry = 1u << 3u,
    invalid_number = 1u << 4u,
    zero_denominator = 1u << 5u,
    invalid_accounting = 1u << 6u,
};

[[nodiscard]] constexpr std::uint32_t flag(ValidationFlag value) noexcept
{
    return static_cast<std::uint32_t>(value);
}

struct CanonicalRecord {
    std::uint64_t sequence{};
    std::string company_id;
    std::string legal_name;
    std::string address;
    std::string postal_code;
    std::string country_code;
    std::string country_name;
    std::string region;
    std::string currency_code;
    std::uint8_t currency_decimals{};
    std::string industry_code;
    std::string industry_name;
    std::int64_t operating_margin_bp{};
    std::int64_t net_margin_bp{};
    std::int64_t current_ratio_bp{};
    std::int64_t debt_to_equity_bp{};
    std::int64_t return_on_assets_bp{};
    std::int64_t revenue_growth_bp{};
    std::int64_t profit_growth_bp{};
    std::uint32_t validation_flags{};
    std::string risk_class;

    bool operator==(const CanonicalRecord&) const = default;
};

struct PipelineOutput {
    std::vector<CanonicalRecord> records;
    std::uint64_t checksum{};
    std::size_t input_bytes{};
    std::size_t output_bytes{};
};

struct WorkloadConfig {
    std::size_t records{1000};
    std::uint64_t seed{0x1234'5678ull};
    std::uint32_t missing_percent{4};
    std::uint32_t malformed_percent{2};
    bool skewed_distribution{true};
    bool large_text{false};
};

struct Workload {
    std::vector<EncodedRecord> records;
    std::size_t total_bytes{};
};

struct BenchmarkMetrics {
    std::string solution;
    std::size_t records{};
    std::size_t input_bytes{};
    std::size_t output_bytes{};
    double elapsed_seconds{};
    double records_per_second{};
    std::uint64_t checksum{};
    std::size_t allocations{};
    std::size_t allocated_bytes{};
    std::size_t worker_count{};
    std::size_t queue_capacity{};
    std::size_t batch_size{};
};

} // namespace financial::common

#include "financial/oracle/pipeline.hpp"

#include "financial/common/checksum.hpp"
#include "financial/common/wire.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>

namespace financial::oracle {
namespace {

using financial::common::CanonicalRecord;
using financial::common::RawFinancialRecord;
using financial::common::ReferenceData;
using financial::common::ValidationFlag;
using financial::common::flag;

struct DecodedRecord {
    RawFinancialRecord raw;
    bool malformed{};
};

struct NormalizedRecord {
    RawFinancialRecord raw;
    std::uint32_t flags{};
};

struct EnrichedRecord {
    NormalizedRecord normalized;
    const financial::common::CountryRef* country{};
    const financial::common::CurrencyRef* currency{};
    const financial::common::IndustryRef* industry{};
};

std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::ranges::find_if(value, not_space));
    value.erase(std::ranges::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string collapse_ws(std::string value)
{
    std::string out;
    out.reserve(value.size());
    bool previous_space = false;
    for (const unsigned char ch : value) {
        if (std::isspace(ch)) {
            if (!previous_space) {
                out.push_back(' ');
            }
            previous_space = true;
        } else {
            out.push_back(static_cast<char>(ch));
            previous_space = false;
        }
    }
    return trim_copy(std::move(out));
}

std::string upper_copy(std::string value)
{
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string title_copy(std::string value)
{
    value = collapse_ws(std::move(value));
    bool start_word = true;
    for (char& ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (std::isspace(uch)) {
            start_word = true;
        } else {
            ch = static_cast<char>(start_word ? std::toupper(uch) : std::tolower(uch));
            start_word = false;
        }
    }
    return value;
}

void replace_all(std::string& text, std::string_view from, std::string_view to)
{
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string normalize_address(std::string value)
{
    value = title_copy(std::move(value));
    replace_all(value, " St.", " Street");
    replace_all(value, " St,", " Street,");
    replace_all(value, " Rd.", " Road");
    replace_all(value, " Rd,", " Road,");
    replace_all(value, " Blvd", " Boulevard");
    return value;
}

std::string normalize_postal(std::string value)
{
    std::string out;
    for (const unsigned char ch : value) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::toupper(ch)));
        }
    }
    return out;
}

bool invalid_money(const financial::common::Money& money)
{
    return money.value.has_value() && !std::isfinite(*money.value);
}

bool negative(const financial::common::Money& money)
{
    return money.value.has_value() && *money.value < 0.0;
}

DecodedRecord decode(const financial::common::EncodedRecord& encoded)
{
    const auto decoded = financial::common::decode_record(encoded.payload);
    if (!decoded) {
        return {.malformed = true};
    }
    return {.raw = *decoded};
}

NormalizedRecord normalize(DecodedRecord decoded)
{
    NormalizedRecord out{.raw = std::move(decoded.raw)};
    if (decoded.malformed) {
        out.flags |= flag(ValidationFlag::missing_required_text);
        return out;
    }
    out.raw.legal_name = title_copy(std::move(out.raw.legal_name));
    out.raw.address = normalize_address(std::move(out.raw.address));
    out.raw.postal_code = normalize_postal(std::move(out.raw.postal_code));
    out.raw.country_code = upper_copy(collapse_ws(std::move(out.raw.country_code)));
    out.raw.currency_code = upper_copy(collapse_ws(std::move(out.raw.currency_code)));
    out.raw.industry_code = upper_copy(collapse_ws(std::move(out.raw.industry_code)));
    if (out.raw.company_id.empty() || out.raw.legal_name.empty() || out.raw.reporting_period.empty()) {
        out.flags |= flag(ValidationFlag::missing_required_text);
    }
    return out;
}

EnrichedRecord enrich(NormalizedRecord normalized, const ReferenceData& reference)
{
    EnrichedRecord out{.normalized = std::move(normalized)};
    out.country = reference.country(out.normalized.raw.country_code);
    out.currency = reference.currency(out.normalized.raw.currency_code);
    out.industry = reference.industry(out.normalized.raw.industry_code);
    if (out.country == nullptr) {
        out.normalized.flags |= flag(ValidationFlag::unknown_country);
    }
    if (out.currency == nullptr) {
        out.normalized.flags |= flag(ValidationFlag::unknown_currency);
    }
    if (out.industry == nullptr) {
        out.normalized.flags |= flag(ValidationFlag::unknown_industry);
    }
    return out;
}

std::int64_t bp(double numerator, double denominator, std::uint32_t& flags)
{
    if (!std::isfinite(numerator) || !std::isfinite(denominator)) {
        flags |= flag(ValidationFlag::invalid_number);
        return 0;
    }
    if (denominator == 0.0) {
        flags |= flag(ValidationFlag::zero_denominator);
        return 0;
    }
    return static_cast<std::int64_t>(std::llround((numerator / denominator) * 10'000.0));
}

double value_or_zero(const financial::common::Money& value)
{
    return value.value.value_or(0.0);
}

std::string classify(double score, std::uint32_t flags)
{
    if ((flags & (flag(ValidationFlag::invalid_accounting) | flag(ValidationFlag::invalid_number))) != 0) {
        return "review";
    }
    if (score < 0.35) {
        return "low";
    }
    if (score < 0.65) {
        return "medium";
    }
    return "high";
}

CanonicalRecord calculate(EnrichedRecord enriched)
{
    const auto& raw = enriched.normalized.raw;
    auto flags = enriched.normalized.flags;
    const std::array money_fields{
        raw.revenue, raw.operating_profit, raw.net_income, raw.current_assets, raw.current_liabilities,
        raw.total_assets, raw.total_debt, raw.shareholder_equity, raw.previous_revenue, raw.previous_profit};
    for (const auto& money : money_fields) {
        if (invalid_money(money)) {
            flags |= flag(ValidationFlag::invalid_number);
        }
    }
    if (negative(raw.revenue) || negative(raw.current_assets) || negative(raw.current_liabilities) ||
        negative(raw.total_assets) || negative(raw.total_debt) || negative(raw.shareholder_equity)) {
        flags |= flag(ValidationFlag::invalid_accounting);
    }

    const double revenue = value_or_zero(raw.revenue);
    const double operating = value_or_zero(raw.operating_profit);
    const double net = value_or_zero(raw.net_income);
    const double current_assets = value_or_zero(raw.current_assets);
    const double current_liabilities = value_or_zero(raw.current_liabilities);
    const double total_assets = value_or_zero(raw.total_assets);
    const double total_debt = value_or_zero(raw.total_debt);
    const double equity = value_or_zero(raw.shareholder_equity);
    const double previous_revenue = value_or_zero(raw.previous_revenue);
    const double previous_profit = value_or_zero(raw.previous_profit);

    const auto operating_margin = bp(operating, revenue, flags);
    const auto net_margin = bp(net, revenue, flags);
    const auto current_ratio = bp(current_assets, current_liabilities, flags);
    const auto debt_to_equity = bp(total_debt, equity, flags);
    const auto roa = bp(net, total_assets, flags);
    const auto revenue_growth = bp(revenue - previous_revenue, previous_revenue, flags);
    const auto profit_growth = bp(operating - previous_profit, previous_profit, flags);

    const double country_risk = enriched.country == nullptr ? 0.50 : enriched.country->risk;
    const double industry_risk = enriched.industry == nullptr ? 0.50 : enriched.industry->risk;
    double score = country_risk * 0.25 + industry_risk * 0.25;
    score += current_ratio < 10000 ? 0.18 : 0.04;
    score += debt_to_equity > 20000 ? 0.18 : 0.05;
    score += net_margin < 0 ? 0.10 : 0.02;
    score += revenue_growth < -500 ? 0.08 : 0.01;

    return {
        .sequence = raw.sequence,
        .company_id = raw.company_id,
        .legal_name = raw.legal_name,
        .address = raw.address,
        .postal_code = raw.postal_code,
        .country_code = raw.country_code,
        .country_name = enriched.country == nullptr ? "" : enriched.country->name,
        .region = enriched.country == nullptr ? "" : enriched.country->region,
        .currency_code = raw.currency_code,
        .currency_decimals = enriched.currency == nullptr ? static_cast<std::uint8_t>(0) : enriched.currency->decimals,
        .industry_code = raw.industry_code,
        .industry_name = enriched.industry == nullptr ? "" : enriched.industry->name,
        .operating_margin_bp = operating_margin,
        .net_margin_bp = net_margin,
        .current_ratio_bp = current_ratio,
        .debt_to_equity_bp = debt_to_equity,
        .return_on_assets_bp = roa,
        .revenue_growth_bp = revenue_growth,
        .profit_growth_bp = profit_growth,
        .validation_flags = flags,
        .risk_class = classify(score, flags),
    };
}

std::size_t encoded_size(const CanonicalRecord& record)
{
    return sizeof(record.sequence) + record.company_id.size() + record.legal_name.size() + record.address.size() +
        record.postal_code.size() + record.country_code.size() + record.country_name.size() + record.region.size() +
        record.currency_code.size() + record.industry_code.size() + record.industry_name.size() + record.risk_class.size() +
        8 * sizeof(std::int64_t) + sizeof(record.validation_flags);
}

} // namespace

financial::common::PipelineOutput process_records(
    std::span<const financial::common::EncodedRecord> input,
    const ReferenceData& reference)
{
    financial::common::PipelineOutput output;
    output.records.reserve(input.size());
    for (const auto& encoded : input) {
        output.input_bytes += encoded.payload.size();
        auto record = calculate(enrich(normalize(decode(encoded)), reference));
        output.output_bytes += encoded_size(record);
        output.records.push_back(std::move(record));
    }
    output.checksum = financial::common::checksum_records(output.records);
    return output;
}

} // namespace financial::oracle

#include "financial/batched/pipeline.hpp"

#include "financial/common/checksum.hpp"
#include "financial/common/wire.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cctype>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace financial::batched {
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

struct WorkItem {
    std::size_t output_start{};
    std::vector<financial::common::EncodedRecord> encoded_records;
};

struct SharedQueueMetrics {
    std::atomic<std::size_t> producer_waits{};
    std::atomic<std::size_t> consumer_waits{};
    std::atomic<std::int64_t> producer_wait_ns{};
    std::atomic<std::int64_t> consumer_wait_ns{};
    std::atomic<std::size_t> max_occupancy{};
};

class BoundedQueue {
public:
    BoundedQueue(std::size_t capacity, SharedQueueMetrics& metrics)
        : capacity_(std::max<std::size_t>(capacity, 1)), metrics_(metrics)
    {
    }

    void push(WorkItem item)
    {
        std::unique_lock lock(mutex_);
        if (items_.size() >= capacity_) {
            metrics_.producer_waits.fetch_add(1, std::memory_order_relaxed);
            const auto start = std::chrono::steady_clock::now();
            not_full_.wait(lock, [&] { return items_.size() < capacity_ || closed_; });
            metrics_.producer_wait_ns.fetch_add(
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count(),
                std::memory_order_relaxed);
        }
        if (closed_) {
            return;
        }
        items_.push_back(std::move(item));
        update_max_occupancy(items_.size());
        not_empty_.notify_one();
    }

    [[nodiscard]] std::optional<WorkItem> pop()
    {
        std::unique_lock lock(mutex_);
        if (items_.empty() && !closed_) {
            metrics_.consumer_waits.fetch_add(1, std::memory_order_relaxed);
            const auto start = std::chrono::steady_clock::now();
            not_empty_.wait(lock, [&] { return !items_.empty() || closed_; });
            metrics_.consumer_wait_ns.fetch_add(
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count(),
                std::memory_order_relaxed);
        }
        if (items_.empty()) {
            return std::nullopt;
        }
        WorkItem item = std::move(items_.front());
        items_.pop_front();
        not_full_.notify_one();
        return item;
    }

    void close()
    {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    void update_max_occupancy(std::size_t value)
    {
        auto observed = metrics_.max_occupancy.load(std::memory_order_relaxed);
        while (observed < value &&
               !metrics_.max_occupancy.compare_exchange_weak(observed, value, std::memory_order_relaxed)) {
        }
    }

    std::size_t capacity_{};
    SharedQueueMetrics& metrics_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<WorkItem> items_;
    bool closed_{};
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

CanonicalRecord process_one(const financial::common::EncodedRecord& encoded, const ReferenceData& reference)
{
    return calculate(enrich(normalize(decode(encoded)), reference));
}

QueueMetrics snapshot_metrics(const SharedQueueMetrics& metrics)
{
    return {
        .producer_waits = metrics.producer_waits.load(std::memory_order_relaxed),
        .consumer_waits = metrics.consumer_waits.load(std::memory_order_relaxed),
        .producer_wait_time = std::chrono::nanoseconds{metrics.producer_wait_ns.load(std::memory_order_relaxed)},
        .consumer_wait_time = std::chrono::nanoseconds{metrics.consumer_wait_ns.load(std::memory_order_relaxed)},
        .max_occupancy = metrics.max_occupancy.load(std::memory_order_relaxed),
    };
}

} // namespace

BatchedOutput process_records(
    std::span<const financial::common::EncodedRecord> input,
    const ReferenceData& reference,
    BatchedConfig config)
{
    if (config.worker_count == 0) {
        config.worker_count = 1;
    }
    if (config.queue_capacity == 0) {
        config.queue_capacity = 1;
    }
    if (config.batch_size == 0) {
        config.batch_size = 1;
    }

    SharedQueueMetrics queue_metrics;
    BoundedQueue queue(config.queue_capacity, queue_metrics);
    std::vector<CanonicalRecord> records(input.size());
    std::atomic<std::size_t> output_bytes{0};

    std::vector<std::jthread> workers;
    workers.reserve(config.worker_count);
    for (std::size_t worker = 0; worker < config.worker_count; ++worker) {
        workers.emplace_back([&] {
            while (auto item = queue.pop()) {
                for (std::size_t offset = 0; offset < item->encoded_records.size(); ++offset) {
                    auto record = process_one(item->encoded_records[offset], reference);
                    output_bytes.fetch_add(encoded_size(record), std::memory_order_relaxed);
                    records[item->output_start + offset] = std::move(record);
                }
            }
        });
    }

    std::size_t input_bytes = 0;
    for (std::size_t start = 0; start < input.size(); start += config.batch_size) {
        const auto count = std::min(config.batch_size, input.size() - start);
        WorkItem item;
        item.output_start = start;
        item.encoded_records.reserve(count);
        for (std::size_t offset = 0; offset < count; ++offset) {
            input_bytes += input[start + offset].payload.size();
            item.encoded_records.push_back(input[start + offset]);
        }
        queue.push(std::move(item));
    }
    queue.close();
    workers.clear();

    financial::common::PipelineOutput pipeline{
        .records = std::move(records),
        .checksum = 0,
        .input_bytes = input_bytes,
        .output_bytes = output_bytes.load(std::memory_order_relaxed),
    };
    pipeline.checksum = financial::common::checksum_records(pipeline.records);

    return {
        .pipeline = std::move(pipeline),
        .queue = snapshot_metrics(queue_metrics),
        .worker_count = config.worker_count,
        .queue_capacity = config.queue_capacity,
        .batch_size = config.batch_size,
    };
}

} // namespace financial::batched

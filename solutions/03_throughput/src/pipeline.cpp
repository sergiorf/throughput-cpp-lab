#include "financial/tp/pipeline.hpp"

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
#include <memory_resource>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace financial::tp {
namespace {

using financial::common::CanonicalRecord;
using financial::common::RawFinancialRecord;
using financial::common::ReferenceData;
using financial::common::ValidationFlag;
using financial::common::flag;

struct BatchItem {
    std::size_t start{};
    std::size_t count{};
};

struct SharedQueueMetrics {
    std::atomic<std::size_t> producer_waits{};
    std::atomic<std::size_t> consumer_waits{};
    std::atomic<std::int64_t> producer_wait_ns{};
    std::atomic<std::int64_t> consumer_wait_ns{};
    std::atomic<std::size_t> max_occupancy{};
};

class BoundedBatchQueue {
public:
    BoundedBatchQueue(std::size_t capacity, SharedQueueMetrics& metrics)
        : capacity_(std::max<std::size_t>(capacity, 1)), metrics_(metrics)
    {
    }

    void push(BatchItem item)
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
        items_.push_back(item);
        update_max_occupancy(items_.size());
        not_empty_.notify_one();
    }

    [[nodiscard]] std::optional<BatchItem> pop()
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
        const auto item = items_.front();
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
    std::deque<BatchItem> items_;
    bool closed_{};
};

struct DecodedRecord {
    RawFinancialRecord raw;
    bool malformed{};
};

struct BatchRecord {
    RawFinancialRecord raw;
    std::pmr::string company_id;
    std::pmr::string legal_name;
    std::pmr::string address;
    std::pmr::string postal_code;
    std::pmr::string country_code;
    std::pmr::string currency_code;
    std::pmr::string industry_code;
    std::uint32_t flags{};

    explicit BatchRecord(std::pmr::memory_resource* resource)
        : company_id(resource),
          legal_name(resource),
          address(resource),
          postal_code(resource),
          country_code(resource),
          currency_code(resource),
          industry_code(resource)
    {
    }
};

struct WorkerScratch {
    std::array<std::byte, 128 * 1024> initial_buffer{};
    std::pmr::monotonic_buffer_resource arena{initial_buffer.data(), initial_buffer.size()};
    std::pmr::memory_resource* ordinary{std::pmr::new_delete_resource()};

    [[nodiscard]] std::pmr::memory_resource* resource(AllocationMode mode) noexcept
    {
        return mode == AllocationMode::batch_arena ? &arena : ordinary;
    }

    void reset(AllocationMode mode)
    {
        if (mode == AllocationMode::batch_arena) {
            arena.release();
        }
    }
};

DecodedRecord decode(const financial::common::EncodedRecord& encoded)
{
    const auto decoded = financial::common::decode_record(encoded.payload);
    if (!decoded) {
        return {.malformed = true};
    }
    return {.raw = *decoded};
}

std::pmr::string trim_copy(std::pmr::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::ranges::find_if(value, not_space));
    value.erase(std::ranges::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::pmr::string collapse_ws(std::string_view value, std::pmr::memory_resource* resource)
{
    std::pmr::string out{resource};
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

std::pmr::string upper_copy(std::string_view value, std::pmr::memory_resource* resource)
{
    auto out = collapse_ws(value, resource);
    for (char& ch : out) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return out;
}

std::pmr::string title_copy(std::string_view value, std::pmr::memory_resource* resource)
{
    auto out = collapse_ws(value, resource);
    bool start_word = true;
    for (char& ch : out) {
        const auto uch = static_cast<unsigned char>(ch);
        if (std::isspace(uch)) {
            start_word = true;
        } else {
            ch = static_cast<char>(start_word ? std::toupper(uch) : std::tolower(uch));
            start_word = false;
        }
    }
    return out;
}

void replace_all(std::pmr::string& text, std::string_view from, std::string_view to)
{
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::pmr::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::pmr::string normalize_address(std::string_view value, std::pmr::memory_resource* resource)
{
    auto out = title_copy(value, resource);
    replace_all(out, " St.", " Street");
    replace_all(out, " St,", " Street,");
    replace_all(out, " Rd.", " Road");
    replace_all(out, " Rd,", " Road,");
    replace_all(out, " Blvd", " Boulevard");
    return out;
}

std::pmr::string normalize_postal(std::string_view value, std::pmr::memory_resource* resource)
{
    std::pmr::string out{resource};
    out.reserve(value.size());
    for (const unsigned char ch : value) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::toupper(ch)));
        }
    }
    return out;
}

BatchRecord normalize(DecodedRecord decoded, std::pmr::memory_resource* resource)
{
    BatchRecord out{resource};
    out.raw = std::move(decoded.raw);
    if (decoded.malformed) {
        out.flags |= flag(ValidationFlag::missing_required_text);
        return out;
    }
    out.company_id = std::pmr::string{out.raw.company_id, resource};
    out.legal_name = title_copy(out.raw.legal_name, resource);
    out.address = normalize_address(out.raw.address, resource);
    out.postal_code = normalize_postal(out.raw.postal_code, resource);
    out.country_code = upper_copy(out.raw.country_code, resource);
    out.currency_code = upper_copy(out.raw.currency_code, resource);
    out.industry_code = upper_copy(out.raw.industry_code, resource);
    if (out.company_id.empty() || out.legal_name.empty() || out.raw.reporting_period.empty()) {
        out.flags |= flag(ValidationFlag::missing_required_text);
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

double value_or_zero(const financial::common::Money& value)
{
    return value.value.value_or(0.0);
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

std::string own(std::string_view value)
{
    return {value.data(), value.size()};
}

CanonicalRecord calculate(BatchRecord& record, const ReferenceData& reference)
{
    const auto country = reference.country(record.country_code);
    const auto currency = reference.currency(record.currency_code);
    const auto industry = reference.industry(record.industry_code);
    if (country == nullptr) {
        record.flags |= flag(ValidationFlag::unknown_country);
    }
    if (currency == nullptr) {
        record.flags |= flag(ValidationFlag::unknown_currency);
    }
    if (industry == nullptr) {
        record.flags |= flag(ValidationFlag::unknown_industry);
    }

    const auto& raw = record.raw;
    auto flags = record.flags;
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

    const double country_risk = country == nullptr ? 0.50 : country->risk;
    const double industry_risk = industry == nullptr ? 0.50 : industry->risk;
    double score = country_risk * 0.25 + industry_risk * 0.25;
    score += current_ratio < 10000 ? 0.18 : 0.04;
    score += debt_to_equity > 20000 ? 0.18 : 0.05;
    score += net_margin < 0 ? 0.10 : 0.02;
    score += revenue_growth < -500 ? 0.08 : 0.01;

    return {
        .sequence = raw.sequence,
        .company_id = own(record.company_id),
        .legal_name = own(record.legal_name),
        .address = own(record.address),
        .postal_code = own(record.postal_code),
        .country_code = own(record.country_code),
        .country_name = country == nullptr ? "" : country->name,
        .region = country == nullptr ? "" : country->region,
        .currency_code = own(record.currency_code),
        .currency_decimals = currency == nullptr ? static_cast<std::uint8_t>(0) : currency->decimals,
        .industry_code = own(record.industry_code),
        .industry_name = industry == nullptr ? "" : industry->name,
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

void process_batch(
    BatchItem batch,
    std::span<const financial::common::EncodedRecord> input,
    const ReferenceData& reference,
    std::vector<CanonicalRecord>& records,
    std::atomic<std::size_t>& output_bytes,
    WorkerScratch& scratch,
    AllocationMode allocation_mode)
{
    scratch.reset(allocation_mode);
    auto* resource = scratch.resource(allocation_mode);
    for (std::size_t offset = 0; offset < batch.count; ++offset) {
        const auto output_index = batch.start + offset;
        auto normalized = normalize(decode(input[output_index]), resource);
        auto canonical = calculate(normalized, reference);
        output_bytes.fetch_add(encoded_size(canonical), std::memory_order_relaxed);
        records[output_index] = std::move(canonical);
    }
    scratch.reset(allocation_mode);
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

ThroughputOutput process_records(
    std::span<const financial::common::EncodedRecord> input,
    const ReferenceData& reference,
    ThroughputConfig config)
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
    BoundedBatchQueue queue(config.queue_capacity, queue_metrics);
    std::vector<CanonicalRecord> records(input.size());
    std::atomic<std::size_t> output_bytes{0};

    std::vector<std::jthread> workers;
    workers.reserve(config.worker_count);
    for (std::size_t worker = 0; worker < config.worker_count; ++worker) {
        workers.emplace_back([&, allocation_mode = config.allocation_mode] {
            WorkerScratch scratch;
            while (auto batch = queue.pop()) {
                process_batch(*batch, input, reference, records, output_bytes, scratch, allocation_mode);
            }
        });
    }

    std::size_t input_bytes = 0;
    for (std::size_t start = 0; start < input.size(); start += config.batch_size) {
        const auto count = std::min(config.batch_size, input.size() - start);
        for (std::size_t i = start; i < start + count; ++i) {
            input_bytes += input[i].payload.size();
        }
        queue.push({.start = start, .count = count});
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
        .allocation_mode = config.allocation_mode,
    };
}

} // namespace financial::tp

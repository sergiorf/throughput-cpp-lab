#include "financial/common/generator.hpp"

#include "financial/common/wire.hpp"

#include <array>
#include <random>

namespace financial::common {
namespace {

template <std::size_t N>
const char* pick(std::mt19937_64& rng, const std::array<const char*, N>& values)
{
    std::uniform_int_distribution<std::size_t> dist(0, values.size() - 1);
    return values[dist(rng)];
}

Money maybe_money(std::mt19937_64& rng, double min, double max, std::uint32_t missing_percent)
{
    std::uniform_int_distribution<std::uint32_t> missing(0, 99);
    if (missing(rng) < missing_percent) {
        return {};
    }
    std::uniform_real_distribution<double> dist(min, max);
    return Money{dist(rng)};
}

} // namespace

RawFinancialRecord make_edge_case_record(std::uint64_t sequence)
{
    return {
        .sequence = sequence,
        .company_id = "EDGE-0001",
        .legal_name = "  acme   holdings  ltd ",
        .address = "  10   main st.   suite 5  ",
        .postal_code = " 12 345 ",
        .country_code = "us",
        .currency_code = "usd",
        .industry_code = "TECH",
        .reporting_period = "2026-Q2",
        .revenue = Money{1000.0},
        .operating_profit = Money{125.0},
        .net_income = Money{90.0},
        .current_assets = Money{500.0},
        .current_liabilities = Money{250.0},
        .total_assets = Money{2000.0},
        .total_debt = Money{800.0},
        .shareholder_equity = Money{900.0},
        .previous_revenue = Money{900.0},
        .previous_profit = Money{80.0},
    };
}

Workload generate_workload(const WorkloadConfig& config)
{
    constexpr std::array countries{"US", "BR", "DE", "JP", "IN", "ZA"};
    constexpr std::array currencies{"USD", "BRL", "EUR", "JPY", "INR", "ZAR"};
    constexpr std::array industries{"TECH", "MFG", "RETL", "HLTH", "ENRG", "FIN"};
    constexpr std::array names{
        "Northwind Components Inc",
        "Fabrikam Industrial SA",
        "Contoso Retail Group",
        "Litware Energy Partners",
        "A Datum Medical Ltd",
        "Proseware Finance BV",
    };
    constexpr std::array streets{
        "101 Main Street",
        "44 Market Rd.",
        "900 Industrial Avenue",
        "17 Finance Blvd",
        "250 River Street",
        "8 Long Address Parkway",
    };

    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<double> revenue_dist(500'000.0, 250'000'000.0);
    std::uniform_int_distribution<int> malformed(0, 99);

    Workload workload;
    workload.records.reserve(config.records);
    for (std::size_t i = 0; i < config.records; ++i) {
        const bool skew = config.skewed_distribution && i % 3 != 0;
        const auto country = skew ? "US" : pick(rng, countries);
        const auto currency = skew ? "USD" : pick(rng, currencies);
        const double revenue = revenue_dist(rng);
        const double operating = revenue * std::uniform_real_distribution<double>(-0.05, 0.28)(rng);
        const double net = operating * std::uniform_real_distribution<double>(0.45, 0.92)(rng);
        const double current_assets = revenue * std::uniform_real_distribution<double>(0.10, 0.45)(rng);
        const double current_liabilities = revenue * std::uniform_real_distribution<double>(0.08, 0.38)(rng);
        const double total_assets = revenue * std::uniform_real_distribution<double>(0.50, 2.50)(rng);
        const double debt = total_assets * std::uniform_real_distribution<double>(0.05, 0.85)(rng);
        const double equity = total_assets - debt - std::uniform_real_distribution<double>(0.0, total_assets * 0.05)(rng);
        const double previous_revenue = revenue * std::uniform_real_distribution<double>(0.75, 1.25)(rng);
        const double previous_profit = operating * std::uniform_real_distribution<double>(0.70, 1.30)(rng);

        std::string address = std::string{pick(rng, streets)} + ", " + (skew ? "New York" : "Metro Center");
        if (config.large_text) {
            address += ", Building 14, Floor 22, International Business District";
        }

        RawFinancialRecord record{
            .sequence = static_cast<std::uint64_t>(i),
            .company_id = "C-" + std::to_string(1'000'000 + i),
            .legal_name = std::string{pick(rng, names)},
            .address = std::move(address),
            .postal_code = i % 2 == 0 ? " 12345-678 " : "ab 12 3cd",
            .country_code = country,
            .currency_code = currency,
            .industry_code = pick(rng, industries),
            .reporting_period = "2026-Q2",
            .revenue = maybe_money(rng, revenue, revenue, config.missing_percent),
            .operating_profit = maybe_money(rng, operating, operating, config.missing_percent),
            .net_income = maybe_money(rng, net, net, config.missing_percent),
            .current_assets = Money{current_assets},
            .current_liabilities = Money{current_liabilities},
            .total_assets = Money{total_assets},
            .total_debt = Money{debt},
            .shareholder_equity = Money{equity},
            .previous_revenue = Money{previous_revenue},
            .previous_profit = Money{previous_profit},
        };

        if (malformed(rng) < static_cast<int>(config.malformed_percent)) {
            record.country_code = "??";
            record.shareholder_equity = Money{-1.0};
        }

        auto encoded = encode_record(record);
        workload.total_bytes += encoded.payload.size();
        workload.records.push_back(std::move(encoded));
    }
    return workload;
}

} // namespace financial::common

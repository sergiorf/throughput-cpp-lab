#include "throughput/generator.hpp"

#include <array>
#include <charconv>
#include <sstream>

namespace throughput {
namespace {

class Rng {
public:
    explicit Rng(std::uint64_t seed)
        : state_(seed)
    {
    }

    std::uint32_t next()
    {
        state_ = state_ * 6364136223846793005ull + 1442695040888963407ull;
        return static_cast<std::uint32_t>(state_ >> 32);
    }

    std::uint32_t range(std::uint32_t min, std::uint32_t max)
    {
        return min + (next() % (max - min + 1));
    }

private:
    std::uint64_t state_;
};

std::string token(Rng& rng, std::string_view prefix, std::uint32_t min_len, std::uint32_t max_len)
{
    static constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string value(prefix);
    const auto len = rng.range(min_len, max_len);
    value.reserve(value.size() + len);
    for (std::uint32_t i = 0; i < len; ++i) {
        value.push_back(alphabet[rng.next() % alphabet.size()]);
    }
    return value;
}

std::uint32_t skewed_count(Rng& rng, std::uint32_t max_value)
{
    const auto roll = rng.range(0, 99);
    if (roll < 70) {
        return rng.range(0, max_value / 2 + 1);
    }
    if (roll < 95) {
        return rng.range(max_value / 2, max_value);
    }
    return max_value + rng.range(1, max_value * 4 + 1);
}

void append_list_sep(std::ostringstream& out, std::uint32_t index)
{
    if (index != 0) {
        out << ',';
    }
}

} // namespace

EncodedBatch generate_batch(const WorkloadConfig& config)
{
    static constexpr std::array<std::string_view, 6> jurisdictions{
        "US-DE", "US-CA", "BR-SP", "BR-RJ", "GB-LND", "DE-BE"
    };
    static constexpr std::array<std::string_view, 5> company_roots{
        "ACME HOLDINGS LIMITED",
        "NORTHWIND TRADING COMPANY",
        "CONTOSO INTERNATIONAL INCORPORATED",
        "FABRIKAM SERVICES",
        "ADVENTURE WORKS"
    };

    Rng rng(config.seed);
    EncodedBatch batch;
    batch.config = config;
    batch.events.reserve(config.records);

    for (std::size_t i = 0; i < config.records; ++i) {
        const bool oversized = rng.range(0, 99) < config.oversized_percent;
        const auto attr_count = config.skewed ? skewed_count(rng, config.max_attributes) : rng.range(1, config.max_attributes);
        const auto rel_count = config.skewed ? skewed_count(rng, config.max_relationships) : rng.range(0, config.max_relationships);
        const auto address_count = oversized ? rng.range(6, 12) : rng.range(1, 2);
        const auto payload_count = oversized ? rng.range(8, 20) : rng.range(1, 4);
        const auto payload_len = oversized ? rng.range(2048, 8192) : rng.range(32, 256);

        std::ostringstream out;
        out << (1'000'000 + i) << '|'
            << rng.range(1, 16) << '|'
            << (1'700'000'000'000'000'000ull + i * 1'000) << '|'
            << static_cast<unsigned>(event_type_from_index(rng.next())) << '|'
            << company_roots[rng.next() % company_roots.size()] << ' ' << token(rng, "", 2, 5) << '|'
            << jurisdictions[rng.next() % jurisdictions.size()] << '|';

        for (std::uint32_t id = 0; id < 2; ++id) {
            append_list_sep(out, id);
            out << "scheme" << id << '~' << token(rng, "ID", 8, 16);
        }
        out << '|';

        for (std::uint32_t address = 0; address < address_count; ++address) {
            append_list_sep(out, address);
            out << token(rng, "ST", 6, 14) << '~'
                << token(rng, "CITY", 4, 10) << '~'
                << jurisdictions[rng.next() % jurisdictions.size()] << '~'
                << rng.range(10000, 99999);
        }
        out << '|';

        for (std::uint32_t attr = 0; attr < attr_count; ++attr) {
            append_list_sep(out, attr);
            out << token(rng, "k", 3, 12) << '~' << token(rng, "v", 8, oversized ? 128 : 32);
        }
        out << '|';

        for (std::uint32_t rel = 0; rel < rel_count; ++rel) {
            append_list_sep(out, rel);
            out << "owns" << (rel % 3) << '~' << token(rng, "RID", 8, 18);
        }
        out << '|';

        for (std::uint32_t payload = 0; payload < payload_count; ++payload) {
            append_list_sep(out, payload);
            out << token(rng, "p", payload_len / 2, payload_len);
        }

        EncodedEvent event{out.str()};
        batch.total_bytes += event.bytes.size();
        batch.events.push_back(std::move(event));
    }

    return batch;
}

} // namespace throughput


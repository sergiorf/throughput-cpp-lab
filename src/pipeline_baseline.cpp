#include "throughput/pipeline_baseline.hpp"

#include "throughput/checksum.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <stdexcept>

namespace throughput::baseline {
namespace {

std::vector<std::string> split(std::string_view input, char delimiter)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= input.size()) {
        const auto end = input.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.emplace_back(input.substr(start));
            break;
        }
        parts.emplace_back(input.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

template <typename T>
T parse_integral(const std::string& value)
{
    T parsed{};
    const auto* first = value.data();
    const auto* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last) {
        throw std::runtime_error("invalid encoded integer");
    }
    return parsed;
}

std::string uppercase_trimmed(std::string value)
{
    const auto first = value.find_first_not_of(' ');
    const auto last = value.find_last_not_of(' ');
    if (first == std::string::npos) {
        value.clear();
        return value;
    }
    value = value.substr(first, last - first + 1);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

DecodedEvent decode(const EncodedEvent& encoded)
{
    const auto fields = split(encoded.bytes, '|');
    if (fields.size() != 11) {
        throw std::runtime_error("invalid encoded event field count");
    }

    DecodedEvent event;
    event.event_id = parse_integral<std::uint64_t>(fields[0]);
    event.source_id = parse_integral<std::uint32_t>(fields[1]);
    event.timestamp_ns = parse_integral<std::uint64_t>(fields[2]);
    event.event_type = event_type_from_index(parse_integral<std::uint32_t>(fields[3]));
    event.company_name = fields[4];
    event.jurisdiction = fields[5];

    for (const auto& item : split(fields[6], ',')) {
        const auto parts = split(item, '~');
        if (parts.size() == 2) {
            event.identifiers.push_back({parts[0], parts[1]});
        }
    }

    for (const auto& item : split(fields[7], ',')) {
        const auto parts = split(item, '~');
        if (parts.size() == 4) {
            event.addresses.push_back({parts[0], parts[1], parts[2], parts[3]});
        }
    }

    for (const auto& item : split(fields[8], ',')) {
        const auto parts = split(item, '~');
        if (parts.size() == 2) {
            event.attributes.push_back({parts[0], parts[1]});
        }
    }

    for (const auto& item : split(fields[9], ',')) {
        const auto parts = split(item, '~');
        if (parts.size() == 2) {
            event.relationships.push_back({parts[0], parts[1]});
        }
    }

    event.payload_fragments = split(fields[10], ',');
    return event;
}

void validate(const DecodedEvent& event, AuditRecord& audit)
{
    if (event.company_name.empty()) {
        audit.validation_errors.emplace_back("missing_company_name");
    }
    if (event.identifiers.empty()) {
        audit.validation_errors.emplace_back("missing_identifier");
    }
    if (event.jurisdiction.empty()) {
        audit.validation_errors.emplace_back("missing_jurisdiction");
    }
    audit.accepted = audit.validation_errors.empty();
}

std::uint64_t payload_digest(const DecodedEvent& event)
{
    Checksum checksum;
    for (const auto& fragment : event.payload_fragments) {
        checksum.add(fragment);
    }
    return checksum.value();
}

NormalizedRecord normalize(const DecodedEvent& event, const ReferenceData& reference, AuditRecord& audit)
{
    NormalizedRecord normalized;
    normalized.event_id = event.event_id;
    normalized.source_id = event.source_id;
    normalized.timestamp_ns = event.timestamp_ns;
    normalized.event_type = event.event_type;

    auto company = uppercase_trimmed(event.company_name);
    const auto alias = reference.alias_for(company);
    if (alias != company) {
        audit.normalization_actions.emplace_back("company_alias");
        company = std::string(alias);
        ++audit.reference_lookup_hits;
    }
    normalized.canonical_company_name = std::move(company);

    normalized.jurisdiction_id = reference.jurisdiction_id(event.jurisdiction);
    if (normalized.jurisdiction_id != 0) {
        ++audit.reference_lookup_hits;
    }

    normalized.category_code = reference.category_for_source(event.source_id);
    if (normalized.category_code != 0) {
        ++audit.reference_lookup_hits;
    }

    if (!event.identifiers.empty()) {
        normalized.primary_external_id = event.identifiers.front().scheme + ':' + event.identifiers.front().value;
    }

    normalized.normalized_addresses = event.addresses;
    normalized.normalized_attributes = event.attributes;
    normalized.relationship_refs = event.relationships;
    normalized.payload_digest = payload_digest(event);
    audit.payload_checksum = normalized.payload_digest;

    if (normalized.jurisdiction_id == 0) {
        normalized.validation_flags |= 1u;
        audit.validation_errors.emplace_back("unknown_jurisdiction");
        audit.accepted = false;
    }

    return normalized;
}

void add_result_checksum(Checksum& checksum, const PipelineResult& result)
{
    for (const auto& record : result.normalized) {
        checksum.add(record.event_id);
        checksum.add(record.source_id);
        checksum.add(record.timestamp_ns);
        checksum.add(static_cast<std::uint32_t>(record.event_type));
        checksum.add(record.canonical_company_name);
        checksum.add(static_cast<std::uint32_t>(record.jurisdiction_id));
        checksum.add(record.primary_external_id);
        checksum.add(static_cast<std::uint32_t>(record.category_code));
        checksum.add(record.payload_digest);
        checksum.add(record.validation_flags);
        checksum.add(static_cast<std::uint64_t>(record.normalized_attributes.size()));
        checksum.add(static_cast<std::uint64_t>(record.relationship_refs.size()));
    }

    for (const auto& audit : result.audit) {
        checksum.add(audit.event_id);
        checksum.add(audit.source_id);
        checksum.add(audit.accepted);
        checksum.add(static_cast<std::uint32_t>(audit.reference_lookup_hits));
        checksum.add(audit.payload_checksum);
        checksum.add(static_cast<std::uint64_t>(audit.validation_errors.size()));
        checksum.add(static_cast<std::uint64_t>(audit.normalization_actions.size()));
    }
}

} // namespace

PipelineResult process_batch(const EncodedBatch& batch, const ReferenceData& reference)
{
    PipelineResult result;
    result.normalized.reserve(batch.events.size());
    result.audit.reserve(batch.events.size());

    for (const auto& encoded : batch.events) {
        auto decoded = decode(encoded);

        AuditRecord audit;
        audit.event_id = decoded.event_id;
        audit.source_id = decoded.source_id;
        validate(decoded, audit);

        auto normalized = normalize(decoded, reference, audit);
        result.normalized.push_back(std::move(normalized));
        result.audit.push_back(std::move(audit));
    }

    Checksum checksum;
    add_result_checksum(checksum, result);
    result.checksum = checksum.value();
    return result;
}

} // namespace throughput::baseline

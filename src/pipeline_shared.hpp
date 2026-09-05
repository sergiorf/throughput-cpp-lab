#pragma once

#include "throughput/checksum.hpp"
#include "throughput/domain.hpp"
#include "throughput/generator.hpp"
#include "throughput/reference_data.hpp"
#include "throughput/semantic_result.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace throughput::pipeline_shared {

template <typename T>
using StdVector = std::vector<T>;

template <typename T>
using PmrVector = std::pmr::vector<T>;

template <typename String, template <typename> typename Vector>
struct ExternalIdentifierT {
    String scheme;
    String value;
};

template <typename String>
struct AddressT {
    String line1;
    String city;
    String jurisdiction;
    String postal_code;
};

template <typename String>
struct AttributeT {
    String key;
    String value;
};

template <typename String>
struct RelationshipT {
    String kind;
    String target_external_id;
};

template <typename String, template <typename> typename Vector>
struct DecodedEventT {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    std::uint64_t timestamp_ns{};
    EventType event_type{};
    String company_name;
    String jurisdiction;
    Vector<ExternalIdentifierT<String, Vector>> identifiers;
    Vector<AddressT<String>> addresses;
    Vector<AttributeT<String>> attributes;
    Vector<RelationshipT<String>> relationships;
    Vector<String> payload_fragments;
};

template <typename String, template <typename> typename Vector>
struct NormalizedRecordT {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    std::uint64_t timestamp_ns{};
    EventType event_type{};
    String canonical_company_name;
    std::uint16_t jurisdiction_id{};
    String primary_external_id;
    std::uint16_t category_code{};
    Vector<AddressT<String>> normalized_addresses;
    Vector<AttributeT<String>> normalized_attributes;
    Vector<RelationshipT<String>> relationship_refs;
    std::uint64_t payload_digest{};
    std::uint32_t validation_flags{};
};

template <typename String, template <typename> typename Vector>
struct AuditRecordT {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    bool accepted{};
    Vector<String> validation_errors;
    Vector<String> normalization_actions;
    std::uint16_t reference_lookup_hits{};
    std::uint64_t payload_checksum{};
};

template <typename String, template <typename> typename Vector>
struct PipelineResultT {
    Vector<NormalizedRecordT<String, Vector>> normalized;
    Vector<AuditRecordT<String, Vector>> audit;
    std::uint64_t checksum{};
};

struct PipelineOptions {
    bool reserve_nested{};
};

template <typename String>
std::string_view view(const String& value) noexcept
{
    return {value.data(), value.size()};
}

inline std::size_t count_items(std::string_view input, char delimiter)
{
    if (input.empty()) {
        return 1;
    }
    return static_cast<std::size_t>(std::count(input.begin(), input.end(), delimiter)) + 1u;
}

template <typename String, template <typename> typename Vector>
Vector<String> split(std::string_view input, char delimiter, bool reserve_parts)
{
    Vector<String> parts;
    if (input.empty()) {
        parts.emplace_back();
        return parts;
    }

    if (reserve_parts) {
        parts.reserve(count_items(input, delimiter));
    }

    std::size_t start = 0;
    while (start <= input.size()) {
        const auto end = input.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.emplace_back(input.data() + start, input.size() - start);
            break;
        }
        parts.emplace_back(input.data() + start, end - start);
        start = end + 1;
    }
    return parts;
}

template <typename T>
T parse_integral(std::string_view value)
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

template <typename String>
String uppercase_trimmed(String value)
{
    const auto first = value.find_first_not_of(' ');
    const auto last = value.find_last_not_of(' ');
    if (first == String::npos) {
        value.clear();
        return value;
    }
    value = value.substr(first, last - first + 1);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

template <typename String, template <typename> typename Vector>
DecodedEventT<String, Vector> decode(const EncodedEvent& encoded, PipelineOptions options)
{
    const auto fields = split<String, Vector>(encoded.bytes, '|', options.reserve_nested);
    if (fields.size() != 11) {
        throw std::runtime_error("invalid encoded event field count");
    }

    DecodedEventT<String, Vector> event;
    event.event_id = parse_integral<std::uint64_t>(view(fields[0]));
    event.source_id = parse_integral<std::uint32_t>(view(fields[1]));
    event.timestamp_ns = parse_integral<std::uint64_t>(view(fields[2]));
    event.event_type = event_type_from_index(parse_integral<std::uint32_t>(view(fields[3])));
    event.company_name = fields[4];
    event.jurisdiction = fields[5];

    if (options.reserve_nested) {
        event.identifiers.reserve(count_items(view(fields[6]), ','));
        event.addresses.reserve(count_items(view(fields[7]), ','));
        event.attributes.reserve(count_items(view(fields[8]), ','));
        event.relationships.reserve(count_items(view(fields[9]), ','));
        event.payload_fragments.reserve(count_items(view(fields[10]), ','));
    }

    for (const auto& item : split<String, Vector>(view(fields[6]), ',', options.reserve_nested)) {
        const auto parts = split<String, Vector>(view(item), '~', options.reserve_nested);
        if (parts.size() == 2) {
            event.identifiers.push_back({parts[0], parts[1]});
        }
    }

    for (const auto& item : split<String, Vector>(view(fields[7]), ',', options.reserve_nested)) {
        const auto parts = split<String, Vector>(view(item), '~', options.reserve_nested);
        if (parts.size() == 4) {
            event.addresses.push_back({parts[0], parts[1], parts[2], parts[3]});
        }
    }

    for (const auto& item : split<String, Vector>(view(fields[8]), ',', options.reserve_nested)) {
        const auto parts = split<String, Vector>(view(item), '~', options.reserve_nested);
        if (parts.size() == 2) {
            event.attributes.push_back({parts[0], parts[1]});
        }
    }

    for (const auto& item : split<String, Vector>(view(fields[9]), ',', options.reserve_nested)) {
        const auto parts = split<String, Vector>(view(item), '~', options.reserve_nested);
        if (parts.size() == 2) {
            event.relationships.push_back({parts[0], parts[1]});
        }
    }

    event.payload_fragments = split<String, Vector>(view(fields[10]), ',', options.reserve_nested);
    return event;
}

template <typename String, template <typename> typename Vector>
void validate(const DecodedEventT<String, Vector>& event, AuditRecordT<String, Vector>& audit)
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

template <typename String, template <typename> typename Vector>
std::uint64_t payload_digest(const DecodedEventT<String, Vector>& event)
{
    Checksum checksum;
    for (const auto& fragment : event.payload_fragments) {
        checksum.add(view(fragment));
    }
    return checksum.value();
}

template <typename String, template <typename> typename Vector>
NormalizedRecordT<String, Vector> normalize(
    const DecodedEventT<String, Vector>& event,
    const ReferenceData& reference,
    AuditRecordT<String, Vector>& audit)
{
    NormalizedRecordT<String, Vector> normalized;
    normalized.event_id = event.event_id;
    normalized.source_id = event.source_id;
    normalized.timestamp_ns = event.timestamp_ns;
    normalized.event_type = event.event_type;

    auto company = uppercase_trimmed(event.company_name);
    const auto alias = reference.alias_for(view(company));
    if (alias != view(company)) {
        audit.normalization_actions.emplace_back("company_alias");
        company = String(alias.data(), alias.size());
        ++audit.reference_lookup_hits;
    }
    normalized.canonical_company_name = std::move(company);

    normalized.jurisdiction_id = reference.jurisdiction_id(view(event.jurisdiction));
    if (normalized.jurisdiction_id != 0) {
        ++audit.reference_lookup_hits;
    }

    normalized.category_code = reference.category_for_source(event.source_id);
    if (normalized.category_code != 0) {
        ++audit.reference_lookup_hits;
    }

    if (!event.identifiers.empty()) {
        const auto& id = event.identifiers.front();
        normalized.primary_external_id.reserve(id.scheme.size() + id.value.size() + 1u);
        normalized.primary_external_id += id.scheme;
        normalized.primary_external_id.push_back(':');
        normalized.primary_external_id += id.value;
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

template <typename String, template <typename> typename Vector>
void add_result_checksum(Checksum& checksum, const PipelineResultT<String, Vector>& result)
{
    for (const auto& record : result.normalized) {
        checksum.add(record.event_id);
        checksum.add(record.source_id);
        checksum.add(record.timestamp_ns);
        checksum.add(static_cast<std::uint32_t>(record.event_type));
        checksum.add(view(record.canonical_company_name));
        checksum.add(static_cast<std::uint32_t>(record.jurisdiction_id));
        checksum.add(view(record.primary_external_id));
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

template <typename String, template <typename> typename Vector>
PipelineResultT<String, Vector> process_batch(
    const EncodedBatch& batch,
    const ReferenceData& reference,
    PipelineOptions options)
{
    PipelineResultT<String, Vector> result;
    result.normalized.reserve(batch.events.size());
    result.audit.reserve(batch.events.size());

    for (const auto& encoded : batch.events) {
        auto decoded = decode<String, Vector>(encoded, options);

        AuditRecordT<String, Vector> audit;
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

template <typename Result>
SemanticResult semantic_result(const Result& result)
{
    return {
        .normalized_records = result.normalized.size(),
        .audit_records = result.audit.size(),
        .checksum = result.checksum,
    };
}

} // namespace throughput::pipeline_shared

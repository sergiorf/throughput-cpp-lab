#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace throughput {

enum class EventType : std::uint8_t {
    company_registered = 0,
    address_changed = 1,
    identifier_added = 2,
    relationship_changed = 3,
    payload_observed = 4
};

struct ExternalIdentifier {
    std::string scheme;
    std::string value;
};

struct Address {
    std::string line1;
    std::string city;
    std::string jurisdiction;
    std::string postal_code;
};

struct Attribute {
    std::string key;
    std::string value;
};

struct Relationship {
    std::string kind;
    std::string target_external_id;
};

struct EncodedEvent {
    std::string bytes;
};

struct DecodedEvent {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    std::uint64_t timestamp_ns{};
    EventType event_type{};
    std::string company_name;
    std::string jurisdiction;
    std::vector<ExternalIdentifier> identifiers;
    std::vector<Address> addresses;
    std::vector<Attribute> attributes;
    std::vector<Relationship> relationships;
    std::vector<std::string> payload_fragments;
};

struct NormalizedRecord {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    std::uint64_t timestamp_ns{};
    EventType event_type{};
    std::string canonical_company_name;
    std::uint16_t jurisdiction_id{};
    std::string primary_external_id;
    std::uint16_t category_code{};
    std::vector<Address> normalized_addresses;
    std::vector<Attribute> normalized_attributes;
    std::vector<Relationship> relationship_refs;
    std::uint64_t payload_digest{};
    std::uint32_t validation_flags{};
};

struct AuditRecord {
    std::uint64_t event_id{};
    std::uint32_t source_id{};
    bool accepted{};
    std::vector<std::string> validation_errors;
    std::vector<std::string> normalization_actions;
    std::uint16_t reference_lookup_hits{};
    std::uint64_t payload_checksum{};
};

struct PipelineResult {
    std::vector<NormalizedRecord> normalized;
    std::vector<AuditRecord> audit;
    std::uint64_t checksum{};
};

const char* to_string(EventType type) noexcept;
EventType event_type_from_index(std::uint32_t index) noexcept;

} // namespace throughput


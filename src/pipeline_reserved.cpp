#include "throughput/pipeline_reserved.hpp"

#include "pipeline_shared.hpp"

#include <utility>

namespace throughput::reserved {
namespace {

Address convert_address(pipeline_shared::AddressT<std::string>& address)
{
    return {
        .line1 = std::move(address.line1),
        .city = std::move(address.city),
        .jurisdiction = std::move(address.jurisdiction),
        .postal_code = std::move(address.postal_code),
    };
}

Attribute convert_attribute(pipeline_shared::AttributeT<std::string>& attribute)
{
    return {
        .key = std::move(attribute.key),
        .value = std::move(attribute.value),
    };
}

Relationship convert_relationship(pipeline_shared::RelationshipT<std::string>& relationship)
{
    return {
        .kind = std::move(relationship.kind),
        .target_external_id = std::move(relationship.target_external_id),
    };
}

} // namespace

PipelineResult process_batch(const EncodedBatch& batch, const ReferenceData& reference)
{
    auto result = pipeline_shared::process_batch<
        std::string,
        pipeline_shared::StdVector>(batch, reference, {.reserve_nested = true});

    PipelineResult converted;
    converted.normalized.reserve(result.normalized.size());
    converted.audit.reserve(result.audit.size());

    for (auto& record : result.normalized) {
        NormalizedRecord out;
        out.event_id = record.event_id;
        out.source_id = record.source_id;
        out.timestamp_ns = record.timestamp_ns;
        out.event_type = record.event_type;
        out.canonical_company_name = std::move(record.canonical_company_name);
        out.jurisdiction_id = record.jurisdiction_id;
        out.primary_external_id = std::move(record.primary_external_id);
        out.category_code = record.category_code;
        out.normalized_addresses.reserve(record.normalized_addresses.size());
        for (auto& address : record.normalized_addresses) {
            out.normalized_addresses.push_back(convert_address(address));
        }
        out.normalized_attributes.reserve(record.normalized_attributes.size());
        for (auto& attribute : record.normalized_attributes) {
            out.normalized_attributes.push_back(convert_attribute(attribute));
        }
        out.relationship_refs.reserve(record.relationship_refs.size());
        for (auto& relationship : record.relationship_refs) {
            out.relationship_refs.push_back(convert_relationship(relationship));
        }
        out.payload_digest = record.payload_digest;
        out.validation_flags = record.validation_flags;
        converted.normalized.push_back(std::move(out));
    }

    for (auto& audit : result.audit) {
        AuditRecord out;
        out.event_id = audit.event_id;
        out.source_id = audit.source_id;
        out.accepted = audit.accepted;
        out.validation_errors = std::move(audit.validation_errors);
        out.normalization_actions = std::move(audit.normalization_actions);
        out.reference_lookup_hits = audit.reference_lookup_hits;
        out.payload_checksum = audit.payload_checksum;
        converted.audit.push_back(std::move(out));
    }

    converted.checksum = result.checksum;
    return converted;
}

} // namespace throughput::reserved

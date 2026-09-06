#include "financial/common/generator.hpp"
#include "financial/common/wire.hpp"
#include "financial/oracle/pipeline.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_wire_round_trip()
{
    const auto raw = financial::common::make_edge_case_record(7);
    const auto encoded = financial::common::encode_record(raw);
    const auto decoded = financial::common::decode_record(encoded.payload);
    require(decoded.has_value(), "edge record should decode");
    require(decoded->sequence == raw.sequence, "sequence round trip");
    require(decoded->legal_name == raw.legal_name, "legal name round trip");
}

void test_fragmented_frame_rejected()
{
    const auto raw = financial::common::make_edge_case_record(1);
    const auto encoded = financial::common::encode_record(raw);
    auto frame = financial::common::frame_payload(encoded.payload);
    frame.pop_back();
    const auto decoded = financial::common::decode_frames(frame, 4096);
    require(!decoded.has_value(), "truncated frame should fail");
}

void test_oracle_normalizes_and_classifies()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto encoded = financial::common::encode_record(financial::common::make_edge_case_record(0));
    const std::array<financial::common::EncodedRecord, 1> input{encoded};
    const auto output = financial::oracle::process_records(input, reference);
    require(output.records.size() == 1, "one record output");
    require(output.records[0].legal_name == "Acme Holdings Ltd", "name normalization");
    require(output.records[0].country_code == "US", "country normalization");
    require(output.records[0].operating_margin_bp == 1250, "operating margin");
    require(output.checksum != 0, "checksum");
}

} // namespace

int main()
{
    try {
        test_wire_round_trip();
        test_fragmented_frame_rejected();
        test_oracle_normalizes_and_classifies();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}

#include "throughput/reference_data.hpp"

namespace throughput {

ReferenceData ReferenceData::make_default()
{
    ReferenceData data;
    data.jurisdictions_.emplace("US-DE", 10);
    data.jurisdictions_.emplace("US-CA", 11);
    data.jurisdictions_.emplace("BR-SP", 20);
    data.jurisdictions_.emplace("BR-RJ", 21);
    data.jurisdictions_.emplace("GB-LND", 30);
    data.jurisdictions_.emplace("DE-BE", 40);

    for (std::uint32_t source = 1; source <= 16; ++source) {
        data.source_categories_.emplace(source, static_cast<std::uint16_t>(100 + (source % 7)));
    }

    data.aliases_.emplace("ACME HOLDINGS LIMITED", "ACME HOLDINGS LTD");
    data.aliases_.emplace("NORTHWIND TRADING COMPANY", "NORTHWIND TRADING CO");
    data.aliases_.emplace("CONTOSO INTERNATIONAL INCORPORATED", "CONTOSO INTERNATIONAL INC");
    return data;
}

std::uint16_t ReferenceData::jurisdiction_id(std::string_view code) const
{
    const auto found = jurisdictions_.find(std::string(code));
    if (found == jurisdictions_.end()) {
        return 0;
    }
    return found->second;
}

std::uint16_t ReferenceData::category_for_source(std::uint32_t source_id) const
{
    const auto found = source_categories_.find(source_id);
    if (found == source_categories_.end()) {
        return 0;
    }
    return found->second;
}

std::string_view ReferenceData::alias_for(std::string_view company_name) const
{
    const auto found = aliases_.find(std::string(company_name));
    if (found == aliases_.end()) {
        return company_name;
    }
    return found->second;
}

} // namespace throughput


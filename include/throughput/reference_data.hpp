#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace throughput {

class ReferenceData {
public:
    static ReferenceData make_default();

    [[nodiscard]] std::uint16_t jurisdiction_id(std::string_view code) const;
    [[nodiscard]] std::uint16_t category_for_source(std::uint32_t source_id) const;
    [[nodiscard]] std::string_view alias_for(std::string_view company_name) const;

private:
    std::unordered_map<std::string, std::uint16_t> jurisdictions_;
    std::unordered_map<std::uint32_t, std::uint16_t> source_categories_;
    std::unordered_map<std::string, std::string> aliases_;
};

} // namespace throughput


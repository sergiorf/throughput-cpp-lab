#include "financial/common/model.hpp"

#include <algorithm>

namespace financial::common {

ReferenceData ReferenceData::make_default()
{
    return {
        .countries = {
            {"US", "United States", "North America", 0.18, "USD"},
            {"BR", "Brazil", "Latin America", 0.34, "BRL"},
            {"DE", "Germany", "Europe", 0.14, "EUR"},
            {"JP", "Japan", "Asia Pacific", 0.16, "JPY"},
            {"IN", "India", "Asia Pacific", 0.29, "INR"},
            {"ZA", "South Africa", "Africa", 0.41, "ZAR"},
        },
        .currencies = {
            {"USD", 2},
            {"BRL", 2},
            {"EUR", 2},
            {"JPY", 0},
            {"INR", 2},
            {"ZAR", 2},
        },
        .industries = {
            {"TECH", "Technology", 0.18},
            {"MFG", "Manufacturing", 0.27},
            {"RETL", "Retail", 0.31},
            {"HLTH", "Healthcare", 0.20},
            {"ENRG", "Energy", 0.37},
            {"FIN", "Financial Services", 0.25},
        },
    };
}

const CountryRef* ReferenceData::country(std::string_view code) const noexcept
{
    const auto found = std::ranges::find_if(countries, [&](const CountryRef& item) {
        return item.code == code;
    });
    return found == countries.end() ? nullptr : &*found;
}

const CurrencyRef* ReferenceData::currency(std::string_view code) const noexcept
{
    const auto found = std::ranges::find_if(currencies, [&](const CurrencyRef& item) {
        return item.code == code;
    });
    return found == currencies.end() ? nullptr : &*found;
}

const IndustryRef* ReferenceData::industry(std::string_view code) const noexcept
{
    const auto found = std::ranges::find_if(industries, [&](const IndustryRef& item) {
        return item.code == code;
    });
    return found == industries.end() ? nullptr : &*found;
}

} // namespace financial::common

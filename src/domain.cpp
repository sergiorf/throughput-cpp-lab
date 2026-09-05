#include "throughput/domain.hpp"

namespace throughput {

const char* to_string(EventType type) noexcept
{
    switch (type) {
    case EventType::company_registered:
        return "company_registered";
    case EventType::address_changed:
        return "address_changed";
    case EventType::identifier_added:
        return "identifier_added";
    case EventType::relationship_changed:
        return "relationship_changed";
    case EventType::payload_observed:
        return "payload_observed";
    }
    return "unknown";
}

EventType event_type_from_index(std::uint32_t index) noexcept
{
    return static_cast<EventType>(index % 5u);
}

} // namespace throughput


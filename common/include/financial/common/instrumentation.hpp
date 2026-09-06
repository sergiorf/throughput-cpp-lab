#pragma once

#include <cstddef>

namespace financial::common {

struct AllocationSnapshot {
    std::size_t allocations{};
    std::size_t deallocations{};
    std::size_t allocated_bytes{};
    std::size_t deallocated_bytes{};
};

class AllocationScope {
public:
    AllocationScope();
    AllocationScope(const AllocationScope&) = delete;
    AllocationScope& operator=(const AllocationScope&) = delete;
    ~AllocationScope();

    [[nodiscard]] AllocationSnapshot snapshot() const noexcept;

private:
    AllocationSnapshot start_;
};

[[nodiscard]] AllocationSnapshot allocation_counters() noexcept;
void reset_allocation_counters() noexcept;

} // namespace financial::common

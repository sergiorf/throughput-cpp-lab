#include "financial/common/instrumentation.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace financial::common {
namespace {

std::atomic<std::size_t> allocation_count{0};
std::atomic<std::size_t> deallocation_count{0};
std::atomic<std::size_t> allocated_byte_count{0};
std::atomic<std::size_t> deallocated_byte_count{0};

} // namespace

void record_allocation(std::size_t size) noexcept
{
    allocation_count.fetch_add(1, std::memory_order_relaxed);
    allocated_byte_count.fetch_add(size, std::memory_order_relaxed);
}

void record_deallocation(std::size_t size) noexcept
{
    deallocation_count.fetch_add(1, std::memory_order_relaxed);
    deallocated_byte_count.fetch_add(size, std::memory_order_relaxed);
}

AllocationScope::AllocationScope()
    : start_(allocation_counters())
{
}

AllocationScope::~AllocationScope() = default;

AllocationSnapshot AllocationScope::snapshot() const noexcept
{
    const auto now = allocation_counters();
    return {
        now.allocations - start_.allocations,
        now.deallocations - start_.deallocations,
        now.allocated_bytes - start_.allocated_bytes,
        now.deallocated_bytes - start_.deallocated_bytes,
    };
}

AllocationSnapshot allocation_counters() noexcept
{
    return {
        allocation_count.load(std::memory_order_relaxed),
        deallocation_count.load(std::memory_order_relaxed),
        allocated_byte_count.load(std::memory_order_relaxed),
        deallocated_byte_count.load(std::memory_order_relaxed),
    };
}

void reset_allocation_counters() noexcept
{
    allocation_count.store(0, std::memory_order_relaxed);
    deallocation_count.store(0, std::memory_order_relaxed);
    allocated_byte_count.store(0, std::memory_order_relaxed);
    deallocated_byte_count.store(0, std::memory_order_relaxed);
}

} // namespace financial::common

void* operator new(std::size_t size)
{
    void* ptr = std::malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    financial::common::record_allocation(size);
    return ptr;
}

void* operator new[](std::size_t size)
{
    void* ptr = std::malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    financial::common::record_allocation(size);
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept
{
    financial::common::record_deallocation(size);
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
    financial::common::record_deallocation(size);
    std::free(ptr);
}

#include "throughput/stages.hpp"

#include "pipeline_shared.hpp"
#include "throughput/pipeline_baseline.hpp"
#include "throughput/pipeline_reserved.hpp"

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace throughput::stages {
namespace {

class DefaultResourceScope {
public:
    explicit DefaultResourceScope(std::pmr::memory_resource& resource)
        : previous_(std::pmr::set_default_resource(&resource))
    {
    }

    DefaultResourceScope(const DefaultResourceScope&) = delete;
    DefaultResourceScope& operator=(const DefaultResourceScope&) = delete;

    ~DefaultResourceScope()
    {
        std::pmr::set_default_resource(previous_);
    }

private:
    std::pmr::memory_resource* previous_;
};

class FixedMonotonicResource final : public std::pmr::memory_resource {
public:
    explicit FixedMonotonicResource(std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
        : upstream_(upstream)
    {
    }

    [[nodiscard]] std::size_t retained_bytes() const noexcept
    {
        return retained_bytes_;
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        const auto current = reinterpret_cast<std::uintptr_t>(buffer_.data() + offset_);
        const auto aligned = (current + alignment - 1u) & ~(alignment - 1u);
        const auto next_offset = static_cast<std::size_t>(aligned - reinterpret_cast<std::uintptr_t>(buffer_.data())) + bytes;
        if (next_offset <= buffer_.size()) {
            offset_ = next_offset;
            retained_bytes_ = std::max(retained_bytes_, offset_);
            return reinterpret_cast<void*>(aligned);
        }

        retained_bytes_ += bytes;
        return upstream_->allocate(bytes, alignment);
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override
    {
        const auto begin = buffer_.data();
        const auto end = begin + buffer_.size();
        const auto* value = static_cast<std::byte*>(pointer);
        if (value < begin || value >= end) {
            upstream_->deallocate(pointer, bytes, alignment);
        }
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }

    std::array<std::byte, 512 * 1024> buffer_{};
    std::size_t offset_{};
    std::size_t retained_bytes_{};
    std::pmr::memory_resource* upstream_;
};

SemanticResult run_pmr_resource(
    const EncodedBatch& batch,
    const ReferenceData& reference,
    std::pmr::memory_resource& resource)
{
    const DefaultResourceScope scope(resource);
    const auto result = pipeline_shared::process_batch<
        std::pmr::string,
        pipeline_shared::PmrVector>(batch, reference, {.reserve_nested = true});
    return pipeline_shared::semantic_result(result);
}

} // namespace

SemanticResult baseline(const EncodedBatch& batch, const ReferenceData& reference)
{
    return pipeline_shared::semantic_result(throughput::baseline::process_batch(batch, reference));
}

SemanticResult reserved(const EncodedBatch& batch, const ReferenceData& reference)
{
    return pipeline_shared::semantic_result(throughput::reserved::process_batch(batch, reference));
}

SemanticResult pmr(const EncodedBatch& batch, const ReferenceData& reference)
{
    std::pmr::monotonic_buffer_resource resource;
    return run_pmr_resource(batch, reference, resource);
}

SemanticResult arena(const EncodedBatch& batch, const ReferenceData& reference)
{
    FixedMonotonicResource resource;
    return run_pmr_resource(batch, reference, resource);
}

} // namespace throughput::stages

#pragma once

#include <cstdint>
#include <string_view>

namespace throughput {

class Checksum {
public:
    void add(std::string_view value) noexcept;
    void add(std::uint64_t value) noexcept;
    void add(std::uint32_t value) noexcept;
    void add(bool value) noexcept;

    [[nodiscard]] std::uint64_t value() const noexcept;

private:
    std::uint64_t state_{1469598103934665603ull};
};

std::uint64_t checksum_bytes(std::string_view value) noexcept;

} // namespace throughput


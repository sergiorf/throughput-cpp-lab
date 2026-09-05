#include "throughput/checksum.hpp"

#include <cstring>

namespace throughput {
namespace {

constexpr std::uint64_t fnv_prime = 1099511628211ull;

void add_byte(std::uint64_t& state, unsigned char byte) noexcept
{
    state ^= byte;
    state *= fnv_prime;
}

} // namespace

void Checksum::add(std::string_view value) noexcept
{
    for (unsigned char byte : value) {
        add_byte(state_, byte);
    }
    add_byte(state_, 0xffu);
}

void Checksum::add(std::uint64_t value) noexcept
{
    for (auto i = 0; i < 8; ++i) {
        add_byte(state_, static_cast<unsigned char>((value >> (i * 8)) & 0xffu));
    }
}

void Checksum::add(std::uint32_t value) noexcept
{
    add(static_cast<std::uint64_t>(value));
}

void Checksum::add(bool value) noexcept
{
    add_byte(state_, value ? 1u : 0u);
}

std::uint64_t Checksum::value() const noexcept
{
    return state_;
}

std::uint64_t checksum_bytes(std::string_view value) noexcept
{
    Checksum checksum;
    checksum.add(value);
    return checksum.value();
}

} // namespace throughput


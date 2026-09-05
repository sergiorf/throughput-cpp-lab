#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

void register_test(std::string_view name, std::function<void()> run);

struct RegisterTest {
    RegisterTest(std::string_view name, std::function<void()> run)
    {
        register_test(name, std::move(run));
    }
};

#define THROUGHPUT_TEST(name) \
    void name(); \
    static RegisterTest register_##name(#name, name); \
    void name()

#define THROUGHPUT_REQUIRE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream message; \
            message << "requirement failed: " #condition; \
            throw std::runtime_error(message.str()); \
        } \
    } while (false)

#define THROUGHPUT_REQUIRE_EQ(actual, expected) \
    do { \
        const auto actual_value = (actual); \
        const auto expected_value = (expected); \
        if (!(actual_value == expected_value)) { \
            std::ostringstream message; \
            message << "requirement failed: " #actual " == " #expected \
                    << " (actual=" << actual_value << ", expected=" << expected_value << ')'; \
            throw std::runtime_error(message.str()); \
        } \
    } while (false)

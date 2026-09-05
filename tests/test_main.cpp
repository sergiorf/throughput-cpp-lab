#include <exception>
#include <functional>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct TestCase {
    std::string_view name;
    std::function<void()> run;
};

std::vector<TestCase>& tests()
{
    static std::vector<TestCase> registered;
    return registered;
}

} // namespace

void register_test(std::string_view name, std::function<void()> run)
{
    tests().push_back({name, std::move(run)});
}

int main()
{
    int failures = 0;
    for (const auto& test : tests()) {
        try {
            test.run();
            std::cout << "[pass] " << test.name << '\n';
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << ex.what() << '\n';
        } catch (...) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": unknown exception\n";
        }
    }
    return failures == 0 ? 0 : 1;
}

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>

template <typename T>
inline std::string test_stringify(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

struct TestFailure {
    std::string condition;
    std::string file;
    int line;
    std::string message;
};

struct TestCase {
    std::string name;
    std::function<void()> body;
};

class TestRegistry {
    std::vector<TestCase> tests;
    std::vector<TestFailure> currentFailures;
    int passCount = 0;
    int failCount = 0;

public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& name, std::function<void()> body) {
        tests.push_back({name, body});
    }

    void reportFailure(const std::string& cond, const std::string& file, int line, const std::string& msg = "") {
        currentFailures.push_back({cond, file, line, msg});
    }

    int runAll() {
        std::cout << "========================================\n";
        std::cout << " Running CityBuilder Test Suite\n";
        std::cout << "========================================\n\n";

        passCount = 0;
        failCount = 0;

        for (const auto& test : tests) {
            currentFailures.clear();
            std::cout << "[ RUN      ] " << test.name << "\n";

            try {
                test.body();
            } catch (const std::exception& e) {
                currentFailures.push_back({"Exception caught", __FILE__, __LINE__, e.what()});
            } catch (...) {
                currentFailures.push_back({"Unknown exception caught", __FILE__, __LINE__, ""});
            }

            if (currentFailures.empty()) {
                std::cout << "[       OK ] " << test.name << "\n";
                passCount++;
            } else {
                std::cout << "[  FAILED  ] " << test.name << "\n";
                for (const auto& f : currentFailures) {
                    std::cout << "    " << f.file << ":" << f.line << ": Assertion failed: " << f.condition;
                    if (!f.message.empty()) std::cout << " (" << f.message << ")";
                    std::cout << "\n";
                }
                failCount++;
            }
        }

        std::cout << "\n----------------------------------------\n";
        std::cout << "Test Summary: " << passCount << " passed, " << failCount << " failed, " << tests.size() << " total\n";
        std::cout << "----------------------------------------\n";

        return (failCount == 0) ? 0 : 1;
    }
};

#define TEST_CASE(name) \
    static void name(); \
    namespace { \
        struct name##_registrar { \
            name##_registrar() { \
                TestRegistry::instance().addTest(#name, name); \
            } \
        } name##_registrar_instance; \
    } \
    static void name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            TestRegistry::instance().reportFailure(#cond, __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            TestRegistry::instance().reportFailure("!(" #cond ")", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            TestRegistry::instance().reportFailure(#a " == " #b, __FILE__, __LINE__, \
                test_stringify(a) + " != " + test_stringify(b)); \
            return; \
        } \
    } while (0)

#define ASSERT_NEAR(a, b, eps) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            TestRegistry::instance().reportFailure("|" #a " - " #b "| <= " #eps, __FILE__, __LINE__, \
                test_stringify(a) + " vs " + test_stringify(b)); \
            return; \
        } \
    } while (0)

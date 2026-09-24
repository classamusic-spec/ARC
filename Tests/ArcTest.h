#pragma once

// Minimal self-contained test harness for ARC.
// - TEST_CASE("group", "name") registers a test.
// - CHECK(expr) records a failure and continues; REQUIRE(expr) aborts the test.
// - MEASURE(key, value) records a numeric measurement (written to CSV by main).

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace arctest
{
struct TestAbort
{
};

struct Measurement
{
    std::string test;
    std::string key;
    double value;
};

struct Context
{
    std::string currentTest;
    int failures = 0;
    int checks = 0;
    std::vector<Measurement> measurements;
    std::vector<std::string> failureMessages;
};

inline Context& context()
{
    static Context c;
    return c;
}

struct TestInfo
{
    std::string group;
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestInfo>& registry()
{
    static std::vector<TestInfo> r;
    return r;
}

struct Registrar
{
    Registrar (const char* group, const char* name, std::function<void()> fn)
    {
        registry().push_back ({ group, name, std::move (fn) });
    }
};

inline void recordFailure (const char* file, int line, const std::string& what)
{
    auto& c = context();
    ++c.failures;
    std::ostringstream os;
    os << "  FAIL [" << c.currentTest << "] " << file << ":" << line << "  " << what;
    c.failureMessages.push_back (os.str());
    std::fprintf (stderr, "%s\n", os.str().c_str());
}

/** Timing checks are not meaningful in sanitizer builds (they are still measured). */
#if defined(ARC_SANITIZED)
inline constexpr bool timingChecksEnabled = false;
#else
inline constexpr bool timingChecksEnabled = true;
#endif

inline void measure (const std::string& key, double value)
{
    auto& c = context();
    c.measurements.push_back ({ c.currentTest, key, value });
    std::printf ("    %-44s %14.6g\n", key.c_str(), value);
}

/** Output directory for WAV/CSV artefacts produced by tests. */
std::string outputDir();

} // namespace arctest

#define ARC_CONCAT_INNER(a, b) a##b
#define ARC_CONCAT(a, b) ARC_CONCAT_INNER (a, b)

#define TEST_CASE(group, name)                                                                     \
    static void ARC_CONCAT (arcTestFn_, __LINE__)();                                               \
    static arctest::Registrar ARC_CONCAT (arcTestReg_, __LINE__) (group, name,                    \
                                                                  &ARC_CONCAT (arcTestFn_, __LINE__)); \
    static void ARC_CONCAT (arcTestFn_, __LINE__)()

#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        ++arctest::context().checks;                                                               \
        if (! (expr))                                                                              \
            arctest::recordFailure (__FILE__, __LINE__, "CHECK(" #expr ")");                       \
    } while (false)

#define CHECK_MSG(expr, msg)                                                                       \
    do                                                                                             \
    {                                                                                              \
        ++arctest::context().checks;                                                               \
        if (! (expr))                                                                              \
        {                                                                                          \
            std::ostringstream arcOs_;                                                             \
            arcOs_ << "CHECK(" #expr ") " << msg;                                                  \
            arctest::recordFailure (__FILE__, __LINE__, arcOs_.str());                             \
        }                                                                                          \
    } while (false)

#define REQUIRE(expr)                                                                              \
    do                                                                                             \
    {                                                                                              \
        ++arctest::context().checks;                                                               \
        if (! (expr))                                                                              \
        {                                                                                          \
            arctest::recordFailure (__FILE__, __LINE__, "REQUIRE(" #expr ")");                     \
            throw arctest::TestAbort {};                                                           \
        }                                                                                          \
    } while (false)

#define MEASURE(key, value) arctest::measure (key, static_cast<double> (value))

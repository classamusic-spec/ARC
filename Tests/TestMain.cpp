#include "ArcTest.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace arctest
{
std::string outputDir()
{
    static const std::string dir = []
    {
        std::filesystem::path p = std::filesystem::path (ARC_SOURCE_DIR) / "Tests" / "output";
        std::filesystem::create_directories (p);
        return p.string();
    }();
    return dir;
}
} // namespace arctest

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    // Match the plugin's audio thread (ScopedNoDenormals in processBlock).
    juce::FloatVectorOperations::disableDenormalisedNumberSupport();

    std::vector<std::string> filters;
    bool listOnly = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--list")
            listOnly = true;
        else if (a == "--all")
            continue;
        else
            filters.push_back (a);
    }

    auto matches = [&] (const arctest::TestInfo& t)
    {
        if (filters.empty())
            return true;
        for (auto& f : filters)
            if (t.group == f || (t.group + "/" + t.name).find (f) != std::string::npos)
                return true;
        return false;
    };

    auto& reg = arctest::registry();
    if (listOnly)
    {
        for (auto& t : reg)
            std::printf ("%s/%s\n", t.group.c_str(), t.name.c_str());
        return 0;
    }

    int run = 0, failedTests = 0;
    const auto start = std::chrono::steady_clock::now();
    for (auto& t : reg)
    {
        if (! matches (t))
            continue;

        auto& c = arctest::context();
        c.currentTest = t.group + "/" + t.name;
        const int failuresBefore = c.failures;
        std::printf ("[ RUN  ] %s\n", c.currentTest.c_str());
        std::fflush (stdout);

        const auto t0 = std::chrono::steady_clock::now();
        try
        {
            t.fn();
        }
        catch (const arctest::TestAbort&)
        {
        }
        catch (const std::exception& e)
        {
            arctest::recordFailure (__FILE__, __LINE__, std::string ("exception: ") + e.what());
        }
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();

        ++run;
        const bool ok = c.failures == failuresBefore;
        if (! ok)
            ++failedTests;
        std::printf ("[ %s ] %s (%.1f ms)\n", ok ? " OK " : "FAIL", c.currentTest.c_str(), ms);
        std::fflush (stdout);
    }

    const double total = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();

    auto& c = arctest::context();
    {
        std::ofstream csv (arctest::outputDir() + "/measurements.csv");
        csv << "test,key,value\n";
        for (auto& m : c.measurements)
            csv << m.test << "," << m.key << "," << m.value << "\n";
    }

    std::printf ("\n==== %d tests, %d failed, %d checks, %d check failures (%.1f s) ====\n",
                 run, failedTests, c.checks, c.failures, total);
    if (! c.failureMessages.empty())
    {
        std::printf ("Failures:\n");
        for (auto& f : c.failureMessages)
            std::printf ("%s\n", f.c_str());
    }
    return failedTests == 0 ? 0 : 1;
}

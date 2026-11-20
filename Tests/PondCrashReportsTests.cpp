// PondCrashReportsTests is its own binary with its own main: the tests that
// crash relaunch it as an app that starts crash reports, so the crash, the
// handler process and the report are all real.
#include "PondCrashReportsTestApp.h"

#include <PondCrashReports/CrashReports.h>

#include <NanoTest/NanoTest.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace nano;

namespace
{
std::vector<std::string> removeFlag(std::vector<std::string> arguments)
{
    auto argumentPointers = std::vector<char*> {};

    for (auto& argument: arguments)
        argumentPointers.push_back(argument.data());

    const auto argumentCount = static_cast<int>(argumentPointers.size());
    return pondcrashreports::getArgumentsWithoutCrashForTesting(argumentCount,
                                                                argumentPointers.data());
}
} // namespace

auto tFlagAndFaultAreRemoved =
    test("PondCrashReports/--crash-for-testing and its fault leave the command line") = []
{
    check(removeFlag({"Grazer", "--crash-for-testing", "abort", "--focus-tab", "3"})
          == std::vector<std::string> {"Grazer", "--focus-tab", "3"});
};

auto tFlagAtTheEndIsRemoved =
    test("PondCrashReports/--crash-for-testing with no fault after it leaves the command line") = []
{
    check(removeFlag({"Grazer", "--crash-for-testing"}) == std::vector<std::string> {"Grazer"});
};

auto tPlainCommandLineIsKept =
    test("PondCrashReports/a command line without the flag is unchanged") = []
{
    check(removeFlag({"Grazer", "https://example.com"})
          == std::vector<std::string> {"Grazer", "https://example.com"});
};

auto tReportsAreInThePondFolder =
    test("PondCrashReports/reports go to pond/<App>/Crashes/<program>") = []
{
    auto directory = pondcrashreports::getCrashReportsDirectory("Grazer");

    check(directory.filename() == "PondCrashReportsTests");
    check(directory.parent_path().filename() == "Crashes");
    check(directory.parent_path().parent_path().filename() == "Grazer");
    check(directory.parent_path().parent_path().parent_path().filename() == "pond");
};

int main(int argc, char* argv[])
{
    if (auto exitCode = pondcrashreports::runCrashHandlerIfRequested(argc, argv))
        return *exitCode;

    if (auto exitCode = pondcrashreportstests::runTestAppIfRequested(argc, argv))
        return *exitCode;

    pondcrashreportstests::rememberTestExecutable(argv[0]);
    return nano::run(argc, argv);
}

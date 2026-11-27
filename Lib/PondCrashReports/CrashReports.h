#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Crash reports for a pond app, written by Crashout. main() starts with
//
//   if (auto exitCode = pondcrashreports::runCrashHandlerIfRequested(argc, argv))
//       return *exitCode;
//
// and calls startCrashReports once it knows this process is the one that
// keeps running: after a second launch has handed over to the running one.
// On Linux, where Crashout has no handler, both do nothing.
namespace pondcrashreports
{
constexpr auto crashForTestingFlag = std::string_view {"--crash-for-testing"};

// `<app> --crash-for-testing <fault>` with an unknown fault prints the
// fault names and ends the process with this code.
constexpr auto exitCodeUnknownFault = 2;

// The crash handler process is this executable relaunched: its exit code when
// this process is that handler, nullopt otherwise.
std::optional<int> runCrashHandlerIfRequested(int argc, char* argv[]);

struct CrashReportsSettings
{
    // The app's pond data folder name (pondapp::getPondAppDataDirectory).
    std::string appName;

    // Written into every report as its release: the app's pond::getVersionString().
    std::string release;
};

// `<app> crashes` with no subcommand or an unknown one prints the usage and
// ends the process with this code.
constexpr auto exitCodeCrashesUsage = 2;

struct CrashesCommandSettings
{
    // The app's pond data folder name (pondapp::getPondAppDataDirectory).
    std::string appName;

    // Other programs of the app whose reports from before PondDev are in
    // <app support>/<program>/Crashes, such as CowTermDaemon for CowTerm.
    std::vector<std::string> otherProgramNames;
};

// `<app> crashes list` prints this machine's crash reports nobody has marked
// fixed, grouped by where they happened, newest group first, and returns its
// exit code. nullopt for any other command line. Needs no crash handler, so
// main() can call it straight after runCrashHandlerIfRequested.
std::optional<int> runCrashesCommandIfRequested(
    const CrashesCommandSettings& settings, int argc, char* argv[]);

// Moves the reports this program wrote to <app support>/<program>/Crashes
// before PondDev into getCrashReportsDirectory, once; starts the crash handler
// process; then, when the command line holds `--crash-for-testing <fault>`,
// raises that Crashout fault on the calling thread.
void startCrashReports(const CrashReportsSettings& settings, int argc, char* argv[]);

// <pond data folder>/<appName>/Crashes/<program>, where program is the name
// this executable had its own app support folder under (Rudder, CowTermDaemon).
std::filesystem::path getCrashReportsDirectory(std::string_view appName);

// The command line without `--crash-for-testing <fault>`, for an app that
// parses its own arguments.
std::vector<std::string> getArgumentsWithoutCrashForTesting(int argc, char* argv[]);
} // namespace pondcrashreports

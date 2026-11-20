#pragma once

#include <eacp/Core/Process/Process.h>

#include <optional>
#include <string>

// PondCrashReportsTests relaunched as a small app that starts crash reports
// the way a pond app does, then exits.
namespace pondcrashreportstests
{
constexpr auto testRelease = std::string_view {"1.2.3+test"};

// Its exit code when argv asks for the test app, nullopt otherwise.
std::optional<int> runTestAppIfRequested(int argc, char* argv[]);

void rememberTestExecutable(const char* argumentZero);

// A pond app name no other test run uses.
std::string makeUniqueTestAppName();

// Runs the test app as appName, with extraArguments after it on its command line.
eacp::Processes::ProcessResult launchTestApp(const std::string& appName,
                                             const eacp::Vector<std::string>& extraArguments);

// Deletes the app's pond data folder, its reports with it.
void removeTestAppData(const std::string& appName);
} // namespace pondcrashreportstests

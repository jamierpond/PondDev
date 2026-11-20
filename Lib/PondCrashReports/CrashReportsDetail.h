#pragma once

#include <filesystem>
#include <optional>
#include <string>

// Shared by the Crashout and the no-handler builds of CrashReports.h.
namespace pondcrashreports::detail
{
// Where Crashout put this program's reports before PondDev:
// <app support>/<program>/Crashes, its getDefaultDatabaseDirectory().
std::filesystem::path getOldCrashReportsDirectory();

// The fault named after the first `--crash-for-testing`, if any.
std::optional<std::string> findFaultToCrashWithForTesting(int argc, char* argv[]);
} // namespace pondcrashreports::detail

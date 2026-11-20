#include "CrashReports.h"

// Crashout has no handler process on this platform, so nothing is reported.
namespace pondcrashreports
{
std::optional<int> runCrashHandlerIfRequested(int, char*[])
{
    return std::nullopt;
}

void startCrashReports(const CrashReportsSettings&, int, char*[]) {}
} // namespace pondcrashreports

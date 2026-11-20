#include "CrashReports.h"
#include "CrashReportsDetail.h"

#include <PondApp/DataDirectory.h>

#include <eacp/Core/Utils/FilePath.h>
#include <eacp/CrashReporting/CrashReporting.h>
#include <eacp/CrashReporting/Faults/Faults.h>

#include <cstdio>
#include <cstdlib>

namespace pondcrashreports
{
namespace
{
using namespace eacp::CrashReporting;

void moveOldCrashReports(std::string_view appName)
{
    auto move = pondapp::moveOldDataDirectory(detail::getOldCrashReportsDirectory(),
                                              getCrashReportsDirectory(appName));

    if (move && !move->error.empty())
    {
        std::fprintf(stderr,
                     "crash reports: could not move %s to %s: %s\n",
                     eacp::FilePath {move->from}.str().c_str(),
                     eacp::FilePath {move->to}.str().c_str(),
                     move->error.c_str());
    }
}

void printFaultNames()
{
    std::fprintf(stderr, "faults here:");

    for (const auto& fault: Faults::listFaults())
    {
        if (fault.isAvailable)
            std::fprintf(stderr, " %s", std::string {fault.name}.c_str());
    }

    std::fprintf(stderr, "\n");
}

void crashIfAskedToForTesting(int argc, char* argv[])
{
    auto fault = detail::findFaultToCrashWithForTesting(argc, argv);

    if (!fault)
        return;

    if (Faults::triggerFault(*fault, Faults::ThreadChoice::main))
        return;

    std::fprintf(stderr, "%s: no fault named %s\n", crashForTestingFlag.data(), fault->c_str());
    printFaultNames();
    std::exit(exitCodeUnknownFault);
}
} // namespace

std::optional<int> runCrashHandlerIfRequested(int argc, char* argv[])
{
    return runHandlerIfRequested(argc, argv);
}

void startCrashReports(const CrashReportsSettings& settings, int argc, char* argv[])
{
    moveOldCrashReports(settings.appName);

    auto crashReportingOptions = Options {};
    crashReportingOptions.databaseDirectory =
        eacp::FilePath {getCrashReportsDirectory(settings.appName)};
    crashReportingOptions.release = settings.release;

    if (!start(crashReportingOptions))
        std::fprintf(stderr, "crash reports: the handler process did not start, so a crash now goes unreported\n");

    crashIfAskedToForTesting(argc, argv);
}
} // namespace pondcrashreports

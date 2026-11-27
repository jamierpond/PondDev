#pragma once

#include <eacp/CrashReporting/CrashReport.h>

#include <string>
#include <vector>

// Crash reports grouped the way Sentry groups issues: by where they happened,
// not by the version or the exact addresses.
namespace pondcrashreports
{
constexpr auto groupingFrameCount = 3;

struct ProgramCrashReport
{
    // The executable that wrote the report, such as CowTermDaemon.
    std::string programName;
    eacp::CrashReporting::CrashReport report;
};

struct CrashReportGroup
{
    std::string programName;
    eacp::CrashReporting::ReportKind kind = eacp::CrashReporting::ReportKind::crash;

    // The frames every report in the group shares, as symbols without `+ offset`.
    std::vector<std::string> groupingFrames;

    // Newest first.
    std::vector<eacp::CrashReporting::CrashReport> reports;
};

// Two reports are one group when they share the program, the kind and the
// first groupingFrameCount frames in the program's own executable, or, when no
// frame is in it, the first groupingFrameCount frames of any module. Groups
// come back newest report first.
std::vector<CrashReportGroup>
    groupCrashReportsByStack(const std::vector<ProgramCrashReport>& reports);
} // namespace pondcrashreports

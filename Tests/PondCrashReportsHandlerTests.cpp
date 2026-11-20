// The tests that need Crashout's handler process, so macOS and Windows only.
#include "PondCrashReportsTestApp.h"

#include <PondCrashReports/CrashReports.h>
#include <PondCrashReports/CrashReportsDetail.h>

#include <eacp/CrashReporting/ReportDatabase.h>

#include <NanoTest/NanoTest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace nano;
using namespace pondcrashreportstests;

namespace
{
constexpr auto reportWaitTimeout = std::chrono::seconds {20};
constexpr auto reportPollInterval = std::chrono::milliseconds {50};
constexpr auto oldReportId = std::string_view {"00000000-0000-4000-8000-000000000000"};

eacp::Vector<eacp::CrashReporting::CrashReport> readPendingReports(const std::string& appName)
{
    auto directory = eacp::FilePath {pondcrashreports::getCrashReportsDirectory(appName)};
    return eacp::CrashReporting::ReportDatabase {directory}.readPendingReports();
}

std::optional<eacp::CrashReporting::CrashReport> waitForOneReport(const std::string& appName)
{
    const auto giveUpAt = std::chrono::steady_clock::now() + reportWaitTimeout;

    while (std::chrono::steady_clock::now() < giveUpAt)
    {
        auto reports = readPendingReports(appName);

        if (reports.size() == 1 && !reports[0].reason.empty())
            return reports[0];

        std::this_thread::sleep_for(reportPollInterval);
    }

    return std::nullopt;
}
} // namespace

auto tCrashForTestingWritesAReport =
    test("PondCrashReports/--crash-for-testing abort crashes and leaves a report in the pond folder") = []
{
    auto appName = makeUniqueTestAppName();
    auto result = launchTestApp(appName, {"--crash-for-testing", "abort"});

    check(result.exitCode != 0);

    auto report = waitForOneReport(appName);
    check(report.has_value());

    if (report)
    {
        check(report->kind == eacp::CrashReporting::ReportKind::crash);
        check(report->metadata["release"] == testRelease);
    }

    removeTestAppData(appName);
};

auto tUnknownFaultExitsWithoutCrashing =
    test("PondCrashReports/--crash-for-testing with an unknown fault exits with a usage error") = []
{
    auto appName = makeUniqueTestAppName();
    auto result = launchTestApp(appName, {"--crash-for-testing", "no-such-fault"});

    check(result.exitCode == pondcrashreports::exitCodeUnknownFault);
    check(result.errorOutput.find("no fault named no-such-fault") != std::string::npos);
    check(readPendingReports(appName).empty());

    removeTestAppData(appName);
};

auto tCleanRunLeavesNoReport =
    test("PondCrashReports/a run that exits normally leaves no report") = []
{
    auto appName = makeUniqueTestAppName();
    auto result = launchTestApp(appName, {});

    check(result.exitCode == 0);
    check(readPendingReports(appName).empty());

    removeTestAppData(appName);
};

auto tOldReportsMoveToThePondFolder =
    test("PondCrashReports/reports from before PondDev move from <app support>/<program>/Crashes") = []
{
    auto appName = makeUniqueTestAppName();
    auto oldDirectory = pondcrashreports::detail::getOldCrashReportsDirectory();
    auto oldReportFolder = oldDirectory / "pending" / oldReportId;
    std::filesystem::create_directories(oldReportFolder);
    std::ofstream {oldReportFolder / "kept.txt"} << "kept";

    check(launchTestApp(appName, {}).exitCode == 0);

    auto movedFile = pondcrashreports::getCrashReportsDirectory(appName) / "pending"
                     / oldReportId / "kept.txt";
    check(std::filesystem::exists(movedFile));
    check(!std::filesystem::exists(oldDirectory));

    auto error = std::error_code {};
    std::filesystem::remove_all(oldDirectory.parent_path(), error);
    removeTestAppData(appName);
};

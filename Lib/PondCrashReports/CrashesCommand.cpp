#include "CrashReportGroups.h"
#include "CrashReports.h"
#include "CrashReportsDetail.h"

#include <eacp/Core/Utils/Console.h>
#include <eacp/Core/Utils/File.h>
#include <eacp/Core/Utils/Files.h>
#include <eacp/Core/Utils/Format.h>
#include <eacp/CrashReporting/ReportDatabase.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <ranges>
#include <set>
#include <utility>

namespace pondcrashreports
{
namespace
{
using namespace eacp::CrashReporting;

constexpr auto crashesCommandName = std::string_view {"crashes"};
constexpr auto listSubcommandName = std::string_view {"list"};
constexpr auto commandArgumentIndex = 1;
constexpr auto subcommandArgumentIndex = 2;
constexpr auto newestReportTopFrameCount = 12;
constexpr auto moduleColumnWidth = 28;
constexpr auto unknownVersion = std::string_view {"?"};
constexpr auto versionMetadataKeys =
    std::to_array<std::string_view>({"release", "appVersion"});

struct CrashesFolder
{
    std::string programName;
    eacp::FilePath directory;
};

eacp::FilePath getOldCrashReportsDirectoryOfProgram(const std::string& programName)
{
    auto ownOldDirectory = eacp::FilePath {detail::getOldCrashReportsDirectory()};
    return ownOldDirectory.parentDirectory().sibling(programName)
           / ownOldDirectory.fileName();
}

std::vector<CrashesFolder> findCrashesFolders(const CrashesCommandSettings& settings)
{
    auto folders = std::vector<CrashesFolder> {};
    auto appCrashesDirectory =
        eacp::FilePath {getCrashReportsDirectory(settings.appName).parent_path()};

    for (const auto& entry: eacp::Files::listDirectory(appCrashesDirectory))
    {
        if (entry.kind == eacp::Files::EntryKind::directory)
            folders.push_back({entry.path.fileName(), entry.path});
    }

    auto programNames = settings.otherProgramNames;
    programNames.push_back(detail::getProgramName());

    for (const auto& programName: programNames)
        folders.push_back(
            {programName, getOldCrashReportsDirectoryOfProgram(programName)});

    return folders;
}

CrashReport makeUnreadableReport(const eacp::FilePath& folder)
{
    auto report = CrashReport {};
    report.id = folder.fileName();
    report.reason = incompleteReason;
    report.capturedBy = CaptureSource::incomplete;
    report.timestamp = eacp::File {folder}.modificationTime();
    report.folder = folder;
    return report;
}

// Read without ReportDatabase::readPendingReports, which writes an incomplete
// report.json over one it cannot read.
std::vector<ProgramCrashReport>
    readPendingCrashReports(const std::vector<CrashesFolder>& folders)
{
    auto reports = std::vector<ProgramCrashReport> {};
    auto readFolders = std::set<std::string> {};

    for (const auto& crashesFolder: folders)
    {
        auto database = ReportDatabase {crashesFolder.directory};

        for (const auto& entry:
             eacp::Files::listDirectory(database.getPendingDirectory()))
        {
            if (entry.kind != eacp::Files::EntryKind::directory)
                continue;

            if (!readFolders.insert(entry.path.str()).second)
                continue;

            auto report = database.readReport(entry.path)
                              .value_or(makeUnreadableReport(entry.path));
            reports.push_back({crashesFolder.programName, std::move(report)});
        }
    }

    return reports;
}

std::string formatReportTime(const CrashReport& report)
{
    using namespace std::chrono;

    auto moment = sys_time<milliseconds> {milliseconds {report.timestamp}};
    auto day = floor<days>(moment);
    auto date = year_month_day {day};
    auto timeOfDay = hh_mm_ss {floor<seconds>(moment - day)};
    auto year = static_cast<int>(date.year());
    auto month = static_cast<unsigned>(date.month());
    auto dayOfMonth = static_cast<unsigned>(date.day());

    return eacp::Strings::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC",
                                 year,
                                 month,
                                 dayOfMonth,
                                 timeOfDay.hours().count(),
                                 timeOfDay.minutes().count(),
                                 timeOfDay.seconds().count());
}

std::string getReportVersion(const CrashReport& report)
{
    for (auto key: versionMetadataKeys)
    {
        auto value = report.metadata.find(std::string {key});

        if (value != report.metadata.end() && !value->second.empty())
            return value->second;
    }

    return std::string {unknownVersion};
}

std::string formatTimesSeen(const CrashReportGroup& group)
{
    auto newest = formatReportTime(group.reports.front());

    if (group.reports.size() == 1)
        return "seen " + newest;

    return "first seen " + formatReportTime(group.reports.back()) + ", last seen "
           + newest;
}

std::string formatVersionsSeen(const CrashReportGroup& group)
{
    auto versions = std::vector<std::string> {};

    for (const auto& report: group.reports | std::views::reverse)
    {
        auto version = getReportVersion(report);

        if (std::ranges::find(versions, version) == versions.end())
            versions.push_back(version);
    }

    auto text = std::string {};

    for (const auto& version: versions)
        text += (text.empty() ? "" : ", ") + version;

    return text;
}

std::string formatReportIds(const CrashReportGroup& group)
{
    auto text = std::string {};

    for (const auto& report: group.reports)
        text += (text.empty() ? "" : ", ") + report.id;

    return text;
}

std::string formatWhatHappened(const CrashReport& report)
{
    auto text = std::string {getReportKindName(report.kind)} + ": " + report.reason;

    if (!report.message.empty())
        text += ": " + report.message;

    return text;
}

std::string formatStackFrame(int index, const StackFrame& frame)
{
    auto symbol =
        frame.symbol.empty() ? "+" + formatHexAddress(frame.offset) : frame.symbol;
    return eacp::Strings::format("    #{:<2} {:<{}} {}",
                                 index,
                                 eacp::FilePath {frame.modulePath}.fileName(),
                                 moduleColumnWidth,
                                 symbol);
}

std::vector<std::string> formatCrashReportGroup(const CrashReportGroup& group)
{
    const auto& newest = group.reports.front();
    const auto& frames = newest.crashingThread.frames;
    auto topFrameCount = std::min<int>(newestReportTopFrameCount, frames.size());

    auto lines = std::vector<std::string> {
        eacp::Strings::format("{} {}, {} report{}, {}",
                              group.programName,
                              getReportKindName(group.kind),
                              group.reports.size(),
                              group.reports.size() == 1 ? "" : "s",
                              formatTimesSeen(group)),
        "  versions: " + formatVersionsSeen(group),
        "  newest: " + formatWhatHappened(newest),
        "  grouped by these frames:"};

    for (const auto& frame: group.groupingFrames)
        lines.push_back("    " + frame);

    lines.push_back(
        eacp::Strings::format("  newest report's top {} frames:", topFrameCount));

    for (auto index = 0; index < topFrameCount; ++index)
        lines.push_back(formatStackFrame(index, frames[index]));

    lines.push_back("  report: " + (newest.folder / "report.json").str());
    lines.push_back("  reports: " + formatReportIds(group));
    lines.push_back("  mark one fixed: just crash-reports mark-fixed local <id>");
    return lines;
}

int listCrashReportGroups(const CrashesCommandSettings& settings)
{
    auto groups = groupCrashReportsByStack(
        readPendingCrashReports(findCrashesFolders(settings)));
    auto isFirstGroup = true;

    for (const auto& group: groups)
    {
        if (!std::exchange(isFirstGroup, false))
            eacp::Console::print("");

        for (const auto& line: formatCrashReportGroup(group))
            eacp::Console::print(line);
    }

    return 0;
}

int printCrashesUsage(const eacp::FilePath& executable)
{
    auto name = executable.fileName();
    eacp::Console::printError(eacp::Strings::format("usage: {} crashes list", name));
    eacp::Console::printError(
        "  list  print this machine's crash reports nobody has marked fixed, grouped by where "
        "they happened, newest first");
    return exitCodeCrashesUsage;
}
} // namespace

std::optional<int> runCrashesCommandIfRequested(
    const CrashesCommandSettings& settings, int argc, char* argv[])
{
    if (argc <= commandArgumentIndex
        || argv[commandArgumentIndex] != crashesCommandName)
        return std::nullopt;

    if (argc <= subcommandArgumentIndex
        || argv[subcommandArgumentIndex] != listSubcommandName)
        return printCrashesUsage(eacp::FilePath {argv[0]});

    return listCrashReportGroups(settings);
}
} // namespace pondcrashreports

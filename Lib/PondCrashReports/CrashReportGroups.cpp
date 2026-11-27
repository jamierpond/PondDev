#include "CrashReportGroups.h"

#include <eacp/Core/Utils/Strings.h>

#include <algorithm>
#include <map>
#include <ranges>

namespace pondcrashreports
{
namespace
{
using eacp::CrashReporting::CrashReport;
using eacp::CrashReporting::StackFrame;

constexpr auto symbolOffsetSeparator = std::string_view {" + "};
constexpr auto groupKeySeparator = '\n';

std::string getSymbolWithoutOffset(const StackFrame& frame)
{
    if (frame.symbol.empty())
    {
        return eacp::FilePath {frame.modulePath}.fileName() + " "
               + eacp::CrashReporting::formatHexAddress(frame.offset);
    }

    auto separator = frame.symbol.rfind(symbolOffsetSeparator);

    if (separator == std::string::npos)
        return frame.symbol;

    auto offsetText = std::string_view {frame.symbol}.substr(
        separator + symbolOffsetSeparator.size());

    if (!eacp::Strings::tryParse<std::uint64_t>(offsetText))
        return frame.symbol;

    return frame.symbol.substr(0, separator);
}

std::vector<std::string> getGroupingFrames(const ProgramCrashReport& programReport)
{
    const auto& frames = programReport.report.crashingThread.frames;
    auto isProgramFrame = [&](const StackFrame& frame)
    {
        return eacp::FilePath {frame.modulePath}.stem() == programReport.programName;
    };
    auto hasProgramFrame = std::ranges::any_of(frames, isProgramFrame);

    auto groupingFrames = std::vector<std::string> {};

    for (const auto& frame: frames)
    {
        if (std::ssize(groupingFrames) == groupingFrameCount)
            break;

        if (!hasProgramFrame || isProgramFrame(frame))
            groupingFrames.push_back(getSymbolWithoutOffset(frame));
    }

    return groupingFrames;
}

std::string makeGroupKey(const CrashReportGroup& group)
{
    auto key = group.programName + groupKeySeparator
               + std::string {eacp::CrashReporting::getReportKindName(group.kind)};

    for (const auto& frame: group.groupingFrames)
        key += groupKeySeparator + frame;

    return key;
}

bool isNewer(const CrashReport& report, const CrashReport& other)
{
    return report.timestamp > other.timestamp;
}
} // namespace

std::vector<CrashReportGroup>
    groupCrashReportsByStack(const std::vector<ProgramCrashReport>& reports)
{
    auto groups = std::vector<CrashReportGroup> {};
    auto groupIndexByKey = std::map<std::string, std::size_t> {};

    for (const auto& programReport: reports)
    {
        auto candidate =
            CrashReportGroup {.programName = programReport.programName,
                              .kind = programReport.report.kind,
                              .groupingFrames = getGroupingFrames(programReport)};
        auto [entry, isNewGroup] =
            groupIndexByKey.try_emplace(makeGroupKey(candidate), groups.size());

        if (isNewGroup)
            groups.push_back(std::move(candidate));

        groups[entry->second].reports.push_back(programReport.report);
    }

    for (auto& group: groups)
        std::ranges::stable_sort(group.reports, isNewer);

    std::ranges::stable_sort(
        groups,
        [](const CrashReportGroup& group, const CrashReportGroup& other)
        { return isNewer(group.reports.front(), other.reports.front()); });

    return groups;
}
} // namespace pondcrashreports

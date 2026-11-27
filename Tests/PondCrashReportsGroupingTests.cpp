#include <PondCrashReports/CrashReportGroups.h>

#include <NanoTest/NanoTest.h>

#include <string>
#include <vector>

using namespace nano;
using namespace pondcrashreports;
using eacp::CrashReporting::CrashReport;
using eacp::CrashReporting::ReportKind;
using eacp::CrashReporting::StackFrame;

namespace
{
constexpr auto daemonPath = "/Applications/CowTerm.app/Contents/MacOS/CowTermDaemon";
constexpr auto terminalPath = "/Applications/CowTerm.app/Contents/MacOS/CowTerm";
constexpr auto kernelPath = "/usr/lib/system/libsystem_kernel.dylib";
constexpr auto libcPath = "/usr/lib/system/libsystem_c.dylib";
constexpr auto libcxxPath = "/usr/lib/libc++.1.dylib";
constexpr auto dispatchPath = "/usr/lib/system/libdispatch.dylib";
constexpr auto quartzPath =
    "/System/Library/Frameworks/QuartzCore.framework/QuartzCore";
constexpr auto ioSurfacePath =
    "/System/Library/Frameworks/IOSurface.framework/IOSurface";
constexpr auto appKitPath = "/System/Library/Frameworks/AppKit.framework/AppKit";

constexpr auto foregroundProcess = "term::Pty::foregroundProcess() const";
constexpr auto handleInfo =
    "(anonymous namespace)::Daemon::handleInfo(eacp::IPC::Messenger&, term::proto::Message const&)";
constexpr auto adoptClient = "(anonymous namespace)::Daemon::adoptClient lambda";
constexpr auto findWatchedFolder =
    "term::filechanges::findWatchedFolderOfChange(path const&)";
constexpr auto projectFileChanges = "ProjectFileChanges::ProjectFileChanges()::$_0";
constexpr auto changeGatherer = "pond::Files::detail::ChangeGatherer::add::$_0";
constexpr auto frameDestructor = "eacp::GPU::Frame::~Frame()";
constexpr auto renderFrame = "eacp::GPU::GPUView::renderFrame(std::function const&)";
constexpr auto paint = "eacp::GPU::GPUView::paint(eacp::Graphics::Context&)";

StackFrame makeFrame(const char* modulePath, const std::string& symbol)
{
    return {.modulePath = modulePath, .symbol = symbol};
}

std::string withOffset(const std::string& symbol, int offset)
{
    return symbol + " + " + std::to_string(offset);
}

struct ReportDescription
{
    std::string programName;
    ReportKind kind = ReportKind::hang;
    std::string id;
    std::int64_t timestamp = 0;
    eacp::Vector<StackFrame> frames;
};

ProgramCrashReport makeReport(const ReportDescription& description)
{
    auto report = CrashReport {};
    report.id = description.id;
    report.kind = description.kind;
    report.timestamp = description.timestamp;
    report.crashingThread.frames = description.frames;
    return {.programName = description.programName, .report = report};
}

eacp::Vector<StackFrame> makeForegroundProcessHang(int offset,
                                                   const std::string& readerSymbol)
{
    return {makeFrame(kernelPath, "__ioctl + 8"),
            makeFrame(libcPath, "tcgetpgrp + 60"),
            makeFrame(daemonPath, withOffset(foregroundProcess, offset)),
            makeFrame(daemonPath, withOffset(handleInfo, offset)),
            makeFrame(daemonPath, withOffset(adoptClient, offset)),
            makeFrame(daemonPath, withOffset(readerSymbol, offset))};
}

std::vector<std::string> getGroupIds(const CrashReportGroup& group)
{
    auto ids = std::vector<std::string> {};

    for (const auto& report: group.reports)
        ids.push_back(report.id);

    return ids;
}
} // namespace

auto tHangsAcrossVersionsAreOneGroup = test(
    "PondCrashReports/grouping: daemon hangs under handleInfo are one group across versions") =
    []
{
    constexpr auto olderOffset = 56;
    constexpr auto newerOffset = 72;
    auto groups = groupCrashReportsByStack({
        makeReport(
            {.programName = "CowTermDaemon",
             .kind = ReportKind::hang,
             .id = "a",
             .timestamp = 1,
             .frames = makeForegroundProcessHang(
                 olderOffset, "eacp::IPC::Messenger::Impl::readUntilGone()::$_0")}),
        makeReport(
            {.programName = "CowTermDaemon",
             .kind = ReportKind::hang,
             .id = "b",
             .timestamp = 2,
             .frames = makeForegroundProcessHang(
                 olderOffset, "eacp::IPC::Messenger::Impl::readUntilGone()::$_0")}),
        makeReport(
            {.programName = "CowTermDaemon",
             .kind = ReportKind::hang,
             .id = "c",
             .timestamp = 3,
             .frames = makeForegroundProcessHang(
                 olderOffset, "eacp::IPC::Messenger::Impl::readUntilGone()::$_0")}),
        makeReport({.programName = "CowTermDaemon",
                    .kind = ReportKind::hang,
                    .id = "d",
                    .timestamp = 4,
                    .frames = makeForegroundProcessHang(
                        newerOffset, "eacp::IPC::Messenger::readUntilGone()::$_0")}),
    });

    check(groups.size() == 1);
    check(groups[0].groupingFrames
          == std::vector<std::string> {foregroundProcess, handleInfo, adoptClient});
    check(getGroupIds(groups[0]) == std::vector<std::string> {"d", "c", "b", "a"});
};

auto tDifferentLibraryFramesAboveAreOneGroup = test(
    "PondCrashReports/grouping: findWatchedFolderOfChange hangs with different libc++ frames are one group") =
    []
{
    auto groups = groupCrashReportsByStack({
        makeReport(
            {.programName = "CowTermDaemon",
             .kind = ReportKind::hang,
             .id = "a",
             .timestamp = 1,
             .frames =
                 {makeFrame(libcxxPath, "std::__fs::filesystem::path::append + 124"),
                  makeFrame(libcxxPath,
                            "std::__fs::filesystem::path::lexically_relative + 988"),
                  makeFrame(daemonPath, withOffset(findWatchedFolder, 240)),
                  makeFrame(daemonPath, withOffset(projectFileChanges, 216)),
                  makeFrame(daemonPath, withOffset(changeGatherer, 164))}}),
        makeReport(
            {.programName = "CowTermDaemon",
             .kind = ReportKind::hang,
             .id = "b",
             .timestamp = 2,
             .frames =
                 {makeFrame(
                      libcxxPath,
                      "std::__fs::filesystem::parser::PathParser::increment() + 432"),
                  makeFrame(daemonPath, withOffset(findWatchedFolder, 240)),
                  makeFrame(daemonPath, withOffset(projectFileChanges, 216)),
                  makeFrame(daemonPath, withOffset(changeGatherer, 164))}}),
    });

    check(groups.size() == 1);
    check(groups[0].groupingFrames
          == std::vector<std::string> {
              findWatchedFolder, projectFileChanges, changeGatherer});
};

auto tDifferentSystemFramesAboveAreOneGroup = test(
    "PondCrashReports/grouping: Frame destructor hangs with different system frames are one group") =
    []
{
    auto groups = groupCrashReportsByStack({
        makeReport(
            {.programName = "CowTerm",
             .kind = ReportKind::hang,
             .id = "a",
             .timestamp = 1,
             .frames = {makeFrame(kernelPath, "semaphore_timedwait_trap + 8"),
                        makeFrame(dispatchPath,
                                  "_dispatch_semaphore_wait_slow + 76"),
                        makeFrame(terminalPath, withOffset(frameDestructor, 204)),
                        makeFrame(terminalPath, withOffset(renderFrame, 288)),
                        makeFrame(terminalPath, withOffset(paint, 108))}}),
        makeReport(
            {.programName = "CowTerm",
             .kind = ReportKind::hang,
             .id = "b",
             .timestamp = 2,
             .frames = {makeFrame(ioSurfacePath, "IOSurfaceGetID + 20"),
                        makeFrame(quartzPath, "-[CAMetalDrawable present] + 156"),
                        makeFrame(terminalPath, withOffset(frameDestructor, 220)),
                        makeFrame(terminalPath, withOffset(renderFrame, 288)),
                        makeFrame(terminalPath, withOffset(paint, 108))}}),
    });

    check(groups.size() == 1);
    check(groups[0].groupingFrames
          == std::vector<std::string> {frameDestructor, renderFrame, paint});
};

auto tKindAndProgramSplitGroups = test(
    "PondCrashReports/grouping: the same frames in another program or as a crash are another group") =
    []
{
    constexpr auto offset = 56;
    auto frames = makeForegroundProcessHang(offset, "reader");
    auto groups = groupCrashReportsByStack({
        makeReport({.programName = "CowTermDaemon",
                    .kind = ReportKind::hang,
                    .id = "a",
                    .timestamp = 1,
                    .frames = frames}),
        makeReport({.programName = "CowTermDaemon",
                    .kind = ReportKind::crash,
                    .id = "b",
                    .timestamp = 2,
                    .frames = frames}),
        makeReport({.programName = "CowTerm",
                    .kind = ReportKind::hang,
                    .id = "c",
                    .timestamp = 3,
                    .frames = frames}),
    });

    check(groups.size() == 3);
};

auto tNoAppFrameGroupsByAnyModule = test(
    "PondCrashReports/grouping: a report with no frame in the program groups by its first frames of any module") =
    []
{
    auto spacesHang = eacp::Vector<StackFrame> {
        makeFrame(kernelPath, "mach_msg2_trap + 8"),
        makeFrame(kernelPath, "mach_msg + 24"),
        makeFrame(appKitPath, "_NSCGSGetSpaces + 32"),
        makeFrame(appKitPath, "NSAccessibilityGetObjectValueForAttribute + 1688")};
    auto commitHang = eacp::Vector<StackFrame> {
        makeFrame(kernelPath, "mach_msg2_trap + 8"),
        makeFrame(kernelPath, "mach_msg + 24"),
        makeFrame(quartzPath, "CA::Transaction::commit() + 652")};
    auto groups = groupCrashReportsByStack({
        makeReport({.programName = "Grazer",
                    .kind = ReportKind::hang,
                    .id = "a",
                    .timestamp = 1,
                    .frames = spacesHang}),
        makeReport({.programName = "Grazer",
                    .kind = ReportKind::hang,
                    .id = "b",
                    .timestamp = 2,
                    .frames = commitHang}),
        makeReport({.programName = "Grazer",
                    .kind = ReportKind::hang,
                    .id = "c",
                    .timestamp = 3,
                    .frames = spacesHang}),
    });

    check(groups.size() == 2);
    check(groups[0].groupingFrames
          == std::vector<std::string> {
              "mach_msg2_trap", "mach_msg", "_NSCGSGetSpaces"});
    check(getGroupIds(groups[0]) == std::vector<std::string> {"c", "a"});
};

auto tGroupsAreNewestFirst = test(
    "PondCrashReports/grouping: groups come newest report first, and so do their reports") =
    []
{
    auto groups = groupCrashReportsByStack({
        makeReport({.programName = "CowTerm",
                    .kind = ReportKind::crash,
                    .id = "old-crash",
                    .timestamp = 10,
                    .frames = {makeFrame(terminalPath, "first + 1")}}),
        makeReport({.programName = "CowTerm",
                    .kind = ReportKind::crash,
                    .id = "newest-other",
                    .timestamp = 30,
                    .frames = {makeFrame(terminalPath, "second + 1")}}),
        makeReport({.programName = "CowTerm",
                    .kind = ReportKind::crash,
                    .id = "new-crash",
                    .timestamp = 20,
                    .frames = {makeFrame(terminalPath, "first + 2")}}),
        makeReport({.programName = "CowTerm",
                    .kind = ReportKind::crash,
                    .id = "middle-other",
                    .timestamp = 15,
                    .frames = {makeFrame(terminalPath, "second + 1")}}),
    });

    check(groups.size() == 2);
    check(getGroupIds(groups[0])
          == std::vector<std::string> {"newest-other", "middle-other"});
    check(getGroupIds(groups[1])
          == std::vector<std::string> {"new-crash", "old-crash"});
};

auto tNoReportsIsNoGroups =
    test("PondCrashReports/grouping: no reports is no groups") = []
{ check(groupCrashReportsByStack({}).empty()); };

#include "PondCrashReportsTestApp.h"

#include <PondApp/DataDirectory.h>
#include <PondCrashReports/CrashReports.h>

#include <filesystem>
#include <random>

namespace pondcrashreportstests
{
namespace
{
constexpr auto testAppMode = std::string_view {"--test-app"};
// The same process without crash reports, for scripts/measure-crash-reports-start-time.sh.
constexpr auto testAppWithoutCrashReportsMode = std::string_view {"--test-app-without-crash-reports"};
constexpr auto modeArgumentCount = 2;
constexpr auto testAppArgumentCount = 3;
constexpr auto testAppNameIndex = 2;

std::string testExecutablePath;
} // namespace

std::optional<int> runTestAppIfRequested(int argc, char* argv[])
{
    if (argc >= modeArgumentCount && argv[1] == testAppWithoutCrashReportsMode)
        return 0;

    if (argc < testAppArgumentCount || argv[1] != testAppMode)
        return std::nullopt;

    pondcrashreports::startCrashReports(
        {.appName = argv[testAppNameIndex], .release = std::string {testRelease}}, argc, argv);
    return 0;
}

void rememberTestExecutable(const char* argumentZero)
{
    testExecutablePath = std::filesystem::absolute(argumentZero).string();
}

std::string makeUniqueTestAppName()
{
    auto generator = std::random_device {};
    return "PondCrashReportsTest" + std::to_string(generator());
}

eacp::Processes::ProcessResult launchTestApp(const std::string& appName,
                                             const eacp::Vector<std::string>& extraArguments)
{
    auto arguments = eacp::Vector<std::string> {std::string {testAppMode}, appName};

    for (const auto& argument: extraArguments)
        arguments.add(argument);

    return eacp::Processes::run(testExecutablePath, arguments);
}

void removeTestAppData(const std::string& appName)
{
    auto error = std::error_code {};
    std::filesystem::remove_all(pondapp::getPondAppDataDirectory(appName), error);
}
} // namespace pondcrashreportstests

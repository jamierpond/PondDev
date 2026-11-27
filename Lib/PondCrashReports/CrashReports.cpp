#include "CrashReports.h"
#include "CrashReportsDetail.h"

#include <PondApp/DataDirectory.h>

#include <eacp/Core/Utils/FilePath.h>
#include <eacp/Core/Utils/StdPath.h>

#include <algorithm>
#include <iterator>

namespace pondcrashreports
{
namespace
{
constexpr auto crashesFolderName = std::string_view {"Crashes"};
} // namespace

namespace detail
{
std::filesystem::path getOldCrashReportsDirectory()
{
    return eacp::toStdPath(eacp::FilePath::appSupportDirectory()) / crashesFolderName;
}

std::string getProgramName()
{
    return getOldCrashReportsDirectory().parent_path().filename().string();
}

std::optional<std::string> findFaultToCrashWithForTesting(int argc, char* argv[])
{
    for (auto index = 1; index + 1 < argc; ++index)
    {
        if (argv[index] == crashForTestingFlag)
            return std::string {argv[index + 1]};
    }

    return std::nullopt;
}
} // namespace detail

std::filesystem::path getCrashReportsDirectory(std::string_view appName)
{
    return pondapp::getPondAppDataDirectory(appName) / crashesFolderName
           / detail::getProgramName();
}

std::vector<std::string> getArgumentsWithoutCrashForTesting(int argc, char* argv[])
{
    auto arguments = std::vector<std::string> {argv, argv + argc};
    auto flag = std::ranges::find(arguments, crashForTestingFlag);

    if (flag != arguments.end())
    {
        auto argumentsAfterFlag = std::distance(std::next(flag), arguments.end());
        auto flagAndFaultCount = 1 + std::min<std::ptrdiff_t>(1, argumentsAfterFlag);
        arguments.erase(flag, std::next(flag, flagAndFaultCount));
    }

    return arguments;
}
} // namespace pondcrashreports

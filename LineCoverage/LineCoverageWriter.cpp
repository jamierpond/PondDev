#include <PondApp/DataDirectory.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

// clang's coverage runtime, linked in by -fprofile-instr-generate.
extern "C" void __llvm_profile_set_filename(const char* filenamePattern);
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_reset_counters();

namespace pondapp
{
namespace
{
constexpr auto countsWriteInterval = std::chrono::minutes {1};

// POND_LINE_COVERAGE_APP_NAME and POND_LINE_COVERAGE_INSTANCE_VARIABLE come from pond_write_line_coverage.
constexpr auto appName = std::string_view {POND_LINE_COVERAGE_APP_NAME};
constexpr auto coverageFolderName = std::string_view {"coverage"};

// %m: every run of one executable merges into one file, so each program keeps one.
constexpr auto countsFileName = "%m.profraw";

std::string getInstanceName()
{
#ifdef POND_LINE_COVERAGE_INSTANCE_VARIABLE
    const auto* name = std::getenv(POND_LINE_COVERAGE_INSTANCE_VARIABLE);
    return name != nullptr ? std::string {name} : std::string {};
#else
    return {};
#endif
}

// A named instance counts into coverage-<instance>, beside the normal instance's coverage.
std::filesystem::path getLineCoverageDirectory()
{
    const auto instance = getInstanceName();
    auto folderName = std::string {coverageFolderName};

    if (!instance.empty())
        folderName += "-" + instance;

    return getPondAppDataDirectory(appName) / folderName;
}

void writeCountsEveryInterval()
{
    while (true)
    {
        std::this_thread::sleep_for(countsWriteInterval);

        // The counts are added to the file, so clearing them keeps the next write from adding them twice.
        if (__llvm_profile_write_file() == 0)
            __llvm_profile_reset_counters();
    }
}

struct LineCoverageWriter
{
    LineCoverageWriter()
    {
        const auto directory = getLineCoverageDirectory();
        auto error = std::error_code {};
        std::filesystem::create_directories(directory, error);

        __llvm_profile_set_filename((directory / countsFileName).string().c_str());
        std::thread {writeCountsEveryInterval}.detach();
    }
};

const auto lineCoverageWriter = LineCoverageWriter {};
} // namespace
} // namespace pondapp

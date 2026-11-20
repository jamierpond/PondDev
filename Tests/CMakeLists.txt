# The tests of the libraries that need no event loop or second process.
add_executable(PondDevTests
        FrecencySearchTests.cpp
        SearchKeysTests.cpp
        SessionLogTests.cpp)
target_link_libraries(PondDevTests PRIVATE NanoTest NanoTestMain FrecencySearch SearchKeys SessionLog)

# The line coverage pipeline's own check (just coverage-check) counts these tests.
pond_write_line_coverage(PondDevTests APP_NAME PondDevLineCoverageCheck)

nano_discover_tests(PondDevTests)

# Its own binary with its own main: the single-instance tests relaunch it as a
# running instance and as a second launch.
add_executable(PondAppTests PondAppTests.cpp)
target_link_libraries(PondAppTests PRIVATE NanoTest PondApp)
nano_discover_tests(PondAppTests)

# The privacy-grants model behind every Permissions… panel; pure, so on every platform.
add_executable(PermissionsTests PermissionsTests.cpp)
target_link_libraries(PermissionsTests PRIVATE NanoTest NanoTestMain PermissionsPanel)
nano_discover_tests(PermissionsTests)

# Its own binary with its own main: the crash tests relaunch it as an app that
# starts crash reports, and Crashout relaunches that as its handler process.
add_executable(PondCrashReportsTests PondCrashReportsTests.cpp PondCrashReportsTestApp.cpp)
target_link_libraries(PondCrashReportsTests PRIVATE NanoTest PondApp eacp-crash-report)
pond_enable_crash_reports(PondCrashReportsTests)

if (TARGET eacp-crash-reporting)
    target_sources(PondCrashReportsTests PRIVATE PondCrashReportsHandlerTests.cpp)
endif ()

nano_discover_tests(PondCrashReportsTests)

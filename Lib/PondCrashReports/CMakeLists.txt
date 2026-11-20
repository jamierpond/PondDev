# A pond app's crash reports, written by Crashout's handler process into
# <pond data folder>/<App>/Crashes/<program>. Crashout has a handler on macOS
# and Windows only; elsewhere the same calls build over CrashReports-None.cpp
# and do nothing.
add_library(PondCrashReports STATIC CrashReports.cpp)

if (TARGET eacp-crash-reporting)
    target_sources(PondCrashReports PRIVATE CrashReports-Crashout.cpp)
    target_link_libraries(PondCrashReports PRIVATE eacp-crash-faults)
else ()
    target_sources(PondCrashReports PRIVATE CrashReports-None.cpp)
endif ()

target_include_directories(PondCrashReports PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(PondCrashReports PRIVATE PondApp)
target_compile_features(PondCrashReports PUBLIC cxx_std_20)

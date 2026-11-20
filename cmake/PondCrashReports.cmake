# Crash reports for a pond app's executable:
#
#   pond_enable_crash_reports(MyApp)
#
# links PondCrashReports and, where Crashout has a handler (macOS, Windows),
# gives the executable Crashout's symbols in <build>/symbols and, on Windows,
# its Windows Error Reporting helper beside the executable.
#
# Configure copies crash-reports.py into <build>/PondCrashReports, where the
# app's justfile runs it.
#
# PondDev includes this file, so every app that adds PondDev with lockstep gets
# the function.

file(COPY "${CMAKE_CURRENT_LIST_DIR}/../Lib/PondCrashReports/crash-reports.py"
        DESTINATION "${CMAKE_BINARY_DIR}/PondCrashReports")

function(pond_enable_crash_reports target)
    target_link_libraries(${target} PRIVATE PondCrashReports)

    if (TARGET eacp-crash-reporting)
        eacp_enable_crash_reporting(${target} SYMBOLS_DIR ${PROJECT_BINARY_DIR}/symbols)
    endif ()
endfunction()

# Counts which lines of an app's own code run, so code nobody uses can be found.
# Configure with -DPOND_LINE_COVERAGE=ON and clang, then:
#
#   pond_add_line_coverage_counters()  # in the app's top CMakeLists, after its packages
#   pond_write_line_coverage(ct APP_NAME CowTerm INSTANCE_VARIABLE COWTERM_INSTANCE)
#
# Each program given to pond_write_line_coverage adds its counts every minute
# and at exit to <pond data folder>/<APP_NAME>/coverage/<id>.profraw, or
# coverage-<instance> when the INSTANCE_VARIABLE environment variable names one.
# With the option on, configure also copies line-coverage.py and the check
# scripts into <build>/PondLineCoverage, where the app's justfile runs them.
#
# PondDev includes this file, so every app that adds PondDev with lockstep gets
# the option and the functions.

option(POND_LINE_COVERAGE "Count which lines of the app's code run (clang only)" OFF)

if (POND_LINE_COVERAGE)
    file(COPY
            "${CMAKE_CURRENT_LIST_DIR}/../LineCoverage/line-coverage.py"
            "${CMAKE_CURRENT_LIST_DIR}/../LineCoverage/check-line-coverage.sh"
            "${CMAKE_CURRENT_LIST_DIR}/../LineCoverage/check-line-coverage-on-host.sh"
            "${CMAKE_CURRENT_LIST_DIR}/../LineCoverage/check-line-coverage-across-builds.sh"
            DESTINATION "${CMAKE_BINARY_DIR}/PondLineCoverage")
endif ()

# Set after the app's packages, so only the app's own code is counted.
function(pond_add_line_coverage_counters)
    if (NOT POND_LINE_COVERAGE)
        return()
    endif ()

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "POND_LINE_COVERAGE needs clang: configure with -DCMAKE_CXX_COMPILER=clang++")
    endif ()

    # With a plain default name every program start creates an empty default.profraw in its working
    # folder; a %m name makes nothing until LineCoverageWriter.cpp has set the real path.
    add_compile_options(-fprofile-instr-generate=%m.profraw -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate=%m.profraw)
endfunction()

function(pond_write_line_coverage target)
    cmake_parse_arguments(PARSE_ARGV 1 lineCoverage "" "APP_NAME;INSTANCE_VARIABLE" "")

    if (NOT lineCoverage_APP_NAME)
        message(FATAL_ERROR "pond_write_line_coverage(${target}) needs APP_NAME, the app's pond data folder name")
    endif ()

    if (NOT POND_LINE_COVERAGE)
        return()
    endif ()

    target_sources(${target} PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../LineCoverage/LineCoverageWriter.cpp")
    target_compile_definitions(${target} PRIVATE "POND_LINE_COVERAGE_APP_NAME=\"${lineCoverage_APP_NAME}\"")

    if (lineCoverage_INSTANCE_VARIABLE)
        target_compile_definitions(${target} PRIVATE
                "POND_LINE_COVERAGE_INSTANCE_VARIABLE=\"${lineCoverage_INSTANCE_VARIABLE}\"")
    endif ()

    target_link_libraries(${target} PRIVATE PondApp)
endfunction()

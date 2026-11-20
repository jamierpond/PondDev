#!/bin/sh
# Shows the line coverage pipeline works end to end on PondDev's own tests:
# builds PondDevTests with POND_LINE_COVERAGE=ON, runs it, keeps the program and
# prints the report. Counts go to app PondDevLineCoverageCheck, never an app's own.
#
#   scripts/check-line-coverage.sh <build directory>
set -eu
cd "$(dirname "$0")/.."
build_directory="$1"
coverage_arguments="--app PondDevLineCoverageCheck --program PondDevTests --own-code-folder Lib"

cmake -S . -B "$build_directory" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DPOND_LINE_COVERAGE=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ >/dev/null
cmake --build "$build_directory" --target PondDevTests >/dev/null

line_coverage="$build_directory/PondLineCoverage/line-coverage.py"
# shellcheck disable=SC2086 # the app settings are one word each
"$line_coverage" $coverage_arguments reset
"$build_directory/Tests/PondDevTests" >/dev/null
# shellcheck disable=SC2086
"$line_coverage" $coverage_arguments keep --programs "$build_directory/Tests"
# shellcheck disable=SC2086
"$line_coverage" $coverage_arguments report
echo "[coverage-check] done"

#!/bin/sh
# How much starting crash reports adds to a program's start: runs
# PondCrashReportsTests as a program that exits at once, with and without
# startCrashReports, through hyperfine.
set -eu

build_directory=${1:-build}
tests="$build_directory/Tests/PondCrashReportsTests"
app_name=PondCrashReportsStartTime

hyperfine --warmup 5 --runs 100 --shell none \
    --command-name "without crash reports" "$tests --test-app-without-crash-reports" \
    --command-name "with crash reports" "$tests --test-app $app_name"

case "$(uname -s)" in
    Darwin) rm -rf "$HOME/Library/Application Support/pond/$app_name" ;;
    *) rm -rf "${XDG_DATA_HOME:-$HOME/.local/share}/pond/$app_name" ;;
esac

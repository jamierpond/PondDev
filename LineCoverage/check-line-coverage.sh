#!/bin/sh
# Shows that line-coverage counts survive a program that never exits cleanly, on
# a throwaway instance (<instance variable>=coverage-check), never the user's own:
# builds the instrumented targets, copies each built program into
# <build directory>/coverage-check under its installed name, runs the start
# command, waits past the program's one-minute write, kills the program with
# SIGKILL, then prints the report from what it wrote. Run from the app's checkout.
#
#   check-line-coverage.sh --coverage-arguments "<line-coverage.py app settings>"
#       --instance-variable NAME --build-directory FOLDER
#       --target NAME [--target NAME] --program <path in build>=<name> [--program ...]
#       --start "<command; \$1 is the folder of copied programs>" --kill <program name>
#       [-- cmake arguments...]
#
# CowTerm's: --instance-variable COWTERM_INSTANCE --target CowTermDaemon --target CowTermCli
#   --program Daemon/CowTermDaemon=cowtermd --program Remote/ct=ct
#   --start 'scripts/probe-bridge.py --local "$1/ct" --keep' --kill cowtermd
set -eu
line_coverage="$(dirname "$0")/line-coverage.py"
targets=""
program_copies=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --coverage-arguments) coverage_arguments="$2"; shift 2 ;;
        --instance-variable) instance_variable="$2"; shift 2 ;;
        --build-directory) build_directory="$2"; shift 2 ;;
        --target) targets="$targets $2"; shift 2 ;;
        --program) program_copies="$program_copies $2"; shift 2 ;;
        --start) start_command="$2"; shift 2 ;;
        --kill) killed_program="$2"; shift 2 ;;
        --) shift; break ;;
        *) echo "check-line-coverage.sh: unknown argument $1" >&2; exit 2 ;;
    esac
done
seconds_past_first_write=75
instance=coverage-check
export "$instance_variable=$instance"

cmake -S . -B "$build_directory" -G Ninja -DCMAKE_BUILD_TYPE=Release "$@" >/dev/null
# shellcheck disable=SC2086 # one word per target
cmake --build "$build_directory" --target $targets >/dev/null
mkdir -p "$build_directory/coverage-check"
programs="$(cd "$build_directory/coverage-check" && pwd)"
for copy in $program_copies; do
    cp "$build_directory/${copy%%=*}" "$programs/${copy#*=}"
done

# shellcheck disable=SC2086 # the app settings are one word each
"$line_coverage" $coverage_arguments reset --instance "$instance"
sh -c "$start_command" check-line-coverage "$programs"
echo "[coverage-check] waiting ${seconds_past_first_write}s for the program's timed write"
sleep "$seconds_past_first_write"
pkill -KILL -f "$programs/$killed_program"
echo "[coverage-check] $killed_program killed with SIGKILL at $(date '+%H:%M:%S')"
# shellcheck disable=SC2086
"$line_coverage" $coverage_arguments keep --instance "$instance" --programs "$programs"
# shellcheck disable=SC2086
"$line_coverage" $coverage_arguments report --instance "$instance"
echo "[coverage-check] done"

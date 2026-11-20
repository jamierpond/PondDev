#!/bin/sh
# Shows that coverage-report combines builds: builds one instrumented program
# twice from the app checkout's HEAD, the second with a function changed by a
# perl substitution, as instance coverage-builds (never the user's own counts).
# The first build runs the first command, which reaches the function; the second
# runs only the second command, which does not. With only the second build kept,
# the function is listed as never run; with both kept, it is not. Run from the
# app's checkout.
#
#   check-line-coverage-across-builds.sh --coverage-arguments "<line-coverage.py app settings>"
#       --instance-variable NAME --target NAME --program <path in build>
#       --source <file> --substitution <perl s///> --marker <text the substitution adds>
#       --function <name> --first-run "<arguments>" --second-run "<arguments>"
#       [-- cmake arguments...]
#
# CowTerm's: --instance-variable COWTERM_INSTANCE --target CowTermCli --program Remote/ct
#   --source Remote/CtMain.cpp --function printVersion --marker '"second build"'
#   --substitution 's/int printVersion\(const Arguments&\)\n\{\n/int printVersion(const Arguments& arguments)\n{\n    if (arguments.size() > 1)\n        std::puts("second build");\n/'
#   --first-run version --second-run help -- -DCOWTERM_HEADLESS=ON
set -eu
line_coverage="$(dirname "$0")/line-coverage.py"
while [ "$#" -gt 0 ]; do
    case "$1" in
        --coverage-arguments) coverage_arguments="$2"; shift 2 ;;
        --instance-variable) instance_variable="$2"; shift 2 ;;
        --target) target="$2"; shift 2 ;;
        --program) program="$2"; shift 2 ;;
        --source) source_file="$2"; shift 2 ;;
        --substitution) substitution="$2"; shift 2 ;;
        --marker) marker="$2"; shift 2 ;;
        --function) function_name="$2"; shift 2 ;;
        --first-run) first_run="$2"; shift 2 ;;
        --second-run) second_run="$2"; shift 2 ;;
        --) shift; break ;;
        *) echo "check-line-coverage-across-builds.sh: unknown argument $1" >&2; exit 2 ;;
    esac
done
instance=coverage-builds
export "$instance_variable=$instance"
root="$PWD/build-coverage-builds"

coverage() {
    # shellcheck disable=SC2086 # the app settings are one word each
    "$line_coverage" $coverage_arguments "$@"
}

build_program() {
    name="$1"
    shift
    rm -rf "$root/$name-src"
    mkdir -p "$root/$name-src"
    git archive --format=tar HEAD | tar -x -C "$root/$name-src"
    if [ "$name" = second ]; then
        perl -0pi -e "$substitution" "$root/$name-src/$source_file"
        grep -q "$marker" "$root/$name-src/$source_file"
    fi
    cmake -S "$root/$name-src" -B "$root/$name-build" -G Ninja -DCMAKE_BUILD_TYPE=Release "$@" >/dev/null
    cmake --build "$root/$name-build" --target "$target" >/dev/null
}

is_function_never_run() {
    coverage report --instance "$instance" > "$root/report.txt"
    grep -E "^\[coverage\]|$function_name" "$root/report.txt"
    grep -q "$function_name" "$root/report.txt"
}

build_program first "$@"
build_program second "$@"
coverage reset --instance "$instance"
# shellcheck disable=SC2086 # one word per argument
"$root/first-build/$program" $first_run
# shellcheck disable=SC2086
"$root/second-build/$program" $second_run > /dev/null

program_folder="$(dirname "$program")"
echo "[coverage-builds] only the second build kept:"
coverage keep --instance "$instance" --programs "$root/second-build/$program_folder"
is_function_never_run || { echo "[coverage-builds] failed: $function_name should be never run"; exit 1; }

echo "[coverage-builds] both builds kept:"
coverage keep --instance "$instance" --programs "$root/first-build/$program_folder"
if is_function_never_run; then
    echo "[coverage-builds] failed: $function_name ran in the first build but is listed as never run"
    exit 1
fi
coverage reset --instance "$instance"
echo "[coverage-builds] passed: a function run only under the first build is reported as run"

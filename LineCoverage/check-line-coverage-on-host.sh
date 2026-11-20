#!/bin/sh
# Runs check-line-coverage.sh on the app checkout's HEAD on <host>, detached
# there so a dropped ssh link does not stop it, and prints its log once it says
# done or failed. Then copies the host's counts and programs here and prints the
# report. Run from the app's checkout. <wrapper> runs the check on the host (CowTerm's
# scripts/with-linux-deps.sh); "" for none. The check arguments are
# check-line-coverage.sh's, without --coverage-arguments and --build-directory.
#
#   check-line-coverage-on-host.sh <host> <app name> <wrapper> "<line-coverage.py app settings>" [check arguments...]
set -eu
here="$(cd "$(dirname "$0")" && pwd)"
host="$1"
app_name="$2"
wrapper="$3"
coverage_arguments="$4"
shift 4
folder="\$HOME/tmp/$(printf '%s' "$app_name" | tr '[:upper:]' '[:lower:]')-coverage-check"
poll_seconds=20

# One argument as a single-quoted shell word.
quote() {
    printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

check_command="$wrapper ../PondLineCoverage/check-line-coverage.sh --build-directory ../build"
check_command="$check_command --coverage-arguments $(quote "$coverage_arguments")"
for argument in "$@"; do
    check_command="$check_command $(quote "$argument")"
done

git archive --format=tar HEAD | ssh -o BatchMode=yes "$host" \
    "rm -rf $folder && mkdir -p $folder/src $folder/PondLineCoverage && tar -x -C $folder/src"
tar -c -C "$here" . | ssh -o BatchMode=yes "$host" "tar -x -C $folder/PondLineCoverage"
ssh -o BatchMode=yes "$host" "cd $folder/src && PATH=\$HOME/.nix-profile/bin:\$PATH nohup sh -c \
    $(quote "$check_command || echo '[coverage-check] failed'") \
    > $folder/check.log 2>&1 < /dev/null &"

until ssh -o BatchMode=yes -o ConnectTimeout=10 "$host" \
        "grep -q '^\[coverage-check\] \(done\|failed\)' $folder/check.log" 2>/dev/null; do
    sleep "$poll_seconds"
done
ssh -o BatchMode=yes "$host" "cat $folder/check.log"
ssh -o BatchMode=yes "$host" "grep -q '^\[coverage-check\] done' $folder/check.log"

# shellcheck disable=SC2086 # the app settings are one word each
"$here/line-coverage.py" $coverage_arguments collect "$host" --instance coverage-check
# shellcheck disable=SC2086
"$here/line-coverage.py" $coverage_arguments report --instance coverage-check

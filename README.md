# PondDev

The libraries Jamie's pond dev apps share. They used to live inside Rudder, so
every other app fetched all of Rudder to get them; now an app fetches PondDev,
which holds only these.

## The libraries

- `PondApp` (`<PondApp/DataDirectory.h>`, `<PondApp/SingleInstance.h>`, namespace `pondapp`): an
  app's data folder under `pond/<App>`, moving an old data folder there once,
  and the claim that only one copy of the app runs.
- `SessionLog` (`<SessionLog/SessionLog.h>`, `<SessionLog/EventLine.h>`): one log file per
  launch, oldest pruned, written as `<event> key=value ...` lines so every app's
  usage reads the same way.
- `FrecencySearch` (`<FrecencySearch/MatchLevel.h>`, `LearnedPicks.h`, `RowOrder.h`): how
  well a query matches a row, and the order of a search list from how often and
  how recently each row was picked. Plain functions, no storage.
- `SearchKeys` (`<SearchKeys/SearchKeys.h>`): the one-line vim search box over a list of
  rows, so every app's picker has the same keys. Built on text-buffer's `VimEditor`.
- `PermissionsPanel` (`<PermissionsPanel/PermissionsPanel.h>`): the Permissions… panel
  showing the macOS privacy grants an app holds, with deep links into System
  Settings. Windows and Linux report every grant as given. Only built when the
  app has `eacp-ui`.
- `pond_add_version_library` (`cmake/PondVersion.cmake`): the app's version string,
  `<semver>+<commit>`, with `.dirty` when tracked files have changed.
- Line coverage (`cmake/PondLineCoverage.cmake`, `LineCoverage/`): counts which lines of an
  app's own code run in normal use, to find features nobody uses.
- `PondCrashReports` (`<PondCrashReports/CrashReports.h>`, namespace `pondcrashreports`,
  `cmake/PondCrashReports.cmake`): crash reports written by
  [Crashout](https://github.com/jamierpond/Crashout) into the app's pond data folder,
  the `--crash-for-testing <fault>` flag, and `crash-reports.py`, which lists the
  reports on this machine and others.

## Adding PondDev to an app

Add eacp first if the app pins its own; PondDev only fetches eacp (and
text-buffer) when no copy is there yet.

```cmake
# Work against a local checkout with:
#   cmake -B build -DLOCKSTEP_PondDev_SOURCE=$HOME/projects/PondDev
lockstep_add_package(
        NAME PondDev
        GITHUB_REPOSITORY jamierpond/PondDev
        GIT_TAG main
        SYSTEM YES)

pond_add_version_library(MyAppVersion)
target_link_libraries(MyApp PRIVATE PondApp SessionLog SearchKeys MyAppVersion)
```

A fetched PondDev builds only the libraries. Built on its own, it also builds
and runs their tests:

```sh
just build   # configure + build in build/
just test    # build, then run every test
just format  # clang-format every tracked source
```

## Line coverage

Configure the app with clang and `-DPOND_LINE_COVERAGE=ON`, then in its top
`CMakeLists.txt`, after its packages so only its own code is counted:

```cmake
pond_add_line_coverage_counters()
pond_write_line_coverage(MyApp APP_NAME MyApp)
```

Call `pond_write_line_coverage` once for each program to count. `APP_NAME` is
the app's pond data folder name; an app with named instances adds
`INSTANCE_VARIABLE <environment variable>` so each instance counts separately.
Each program adds its counts every minute and at exit to
`<pond data folder>/<APP_NAME>/coverage/` (`coverage-<instance>` for a named
instance): `~/Library/Application Support/pond/<APP_NAME>` on macOS,
`~/.local/share/pond/<APP_NAME>` on Linux.

Configure copies `line-coverage.py` and the check scripts into
`<build>/PondLineCoverage`. Counts map to lines only through the programs that
wrote them, so keep each build's programs before an install replaces them, and
read the report from the app's checkout:

```sh
coverage="build-coverage/PondLineCoverage/line-coverage.py --app MyApp --program MyApp \
    --installed-in ~/.local/libexec/myapp --own-code-folder Source --own-code-folder Lib \
    --not-own-code-folder Tests"
$coverage keep            # before and after installing the instrumented build
$coverage report          # never-run files and functions, largest first, and an HTML per-line view
$coverage collect user@host   # copy another machine's counts here; report then includes them
$coverage reset           # delete the counts and kept programs
```

`just coverage-check` here builds PondDev's own tests with coverage and prints
their report, which shows the pipeline works.

## Crash reports

When a program crashes or hangs, Crashout's handler process, which is the
program's own executable started again, writes a report to
`<pond data folder>/<App>/Crashes/<program>/pending/<id>/`. The program is the
executable's own name, so an app with a daemon (CowTerm and CowTermDaemon) keeps
one folder each. Reports from before PondDev, in `<app support>/<program>/Crashes`,
move there on the first start. Crashout has a handler on macOS and Windows; on
Linux the same calls build and do nothing.

In the app's `CMakeLists.txt`, for each executable that reports crashes:

```cmake
pond_enable_crash_reports(MyApp)
```

This links `PondCrashReports` and, on macOS and Windows, copies the symbols
into `<build>/symbols` and, on Windows, Crashout's Windows Error Reporting
helper beside the executable. PondDev fetches Crashout unless the app already
added it.

In `main`, the first call returns at once unless this process is the crash
handler, whose whole life it is. The second starts the handler; call it once
this process is known to be the one that keeps running, after a second launch
has handed over to the first and after any quick command that prints and exits:

```cpp
#include <PondCrashReports/CrashReports.h>
#include <PondVersion.h>

int main(int argc, char* argv[])
{
    if (auto exitCode = pondcrashreports::runCrashHandlerIfRequested(argc, argv))
        return *exitCode;

    // ... a second launch hands over and returns here ...

    pondcrashreports::startCrashReports(
        {.appName = "MyApp", .release = pond::getVersionString()}, argc, argv);
    // ...
}
```

`startCrashReports` also handles `MyApp --crash-for-testing <fault>`, which
crashes on purpose with one of Crashout's faults (`nullptr`, `abort`,
`stackoverflow`, ...) to check a report end to end. An unknown fault prints the
fault names and exits with code 2. An app that parses its own arguments takes
the flag out with `getArgumentsWithoutCrashForTesting`.

Starting crash reports adds about 10 ms to a program's start on an M-series Mac
(`just measure-crash-reports-start-time` measures it with a small program; a
larger executable takes longer, since the handler is that executable started
again). The handler ends with the program, so a command whose output is piped
or captured with `$(...)` returns as soon as the program does.

Configure copies `crash-reports.py` into `<build>/PondCrashReports`. The app's
justfile gives it a `crash-reports` recipe, the name the script's mark-fixed
lines use, reading it from the build folder `just test` configures (Rudder and
Grazer use `build-test`):

```just
[doc('List the crash reports nobody has marked fixed here and on other machines (list [HOST ...]), or mark one fixed (mark-fixed HOST ID)')]
crash-reports *arguments:
    python3 build-test/PondCrashReports/crash-reports.py --app MyApp {{arguments}}
```

`list big-cow jamiepond@tamby` reads each host over ssh (`local` is this
machine, the default). An app whose programs wrote reports before PondDev under
other names passes each with `--program` (`--program CowTerm --program
CowTermDaemon`), so those are still found on machines that have not updated yet.

## Apps that use it

- [Rudder](https://github.com/jamierpond/Rudder): an fzf-style app switcher.
- [Grazer](https://github.com/jamierpond/Grazer): a keyboard-driven web browser with no tab strip or address bar.
- [CowTerm](https://github.com/jamierpond/CowTerm): a GPU-drawn terminal emulator and session manager.
- [iac](https://github.com/jamierpond/iac): the chat room the agents on Jamie's machines share.
- [DiffView](https://github.com/jamierpond/DiffView): a live diff viewer for reviewing what agents change in a repo.

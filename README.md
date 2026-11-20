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

## Apps that use it

- [Rudder](https://github.com/jamierpond/Rudder): an fzf-style app switcher.
- [Grazer](https://github.com/jamierpond/Grazer): a keyboard-driven web browser with no tab strip or address bar.
- [CowTerm](https://github.com/jamierpond/CowTerm): a GPU-drawn terminal emulator and session manager.
- [iac](https://github.com/jamierpond/iac): the chat room the agents on Jamie's machines share.
- [DiffView](https://github.com/jamierpond/DiffView): a live diff viewer for reviewing what agents change in a repo.

# PondDev dev commands. Recipes are a line or two of cmake; anything longer is a
# script in scripts/.

set windows-shell := ["cmd.exe", "/c"]

build_dir := "build"
build_type := "Debug"

# Ninja drives cl directly on Windows, so it needs the Visual Studio environment.
msvc := if os() == "windows" { "powershell -NoProfile -ExecutionPolicy Bypass -File scripts/with-msvc.ps1" } else { "" }

# Show the recipe list by default.
default:
    @just --list

[doc('Configure + build the libraries and their tests; extra arguments go to the configure step')]
build *cmake_args:
    {{msvc}} cmake -S . -B {{build_dir}} -G Ninja -DCMAKE_BUILD_TYPE={{build_type}} {{cmake_args}}
    {{msvc}} cmake --build {{build_dir}}

[doc('Build, then run every test')]
test: build
    ctest --test-dir {{build_dir}} --output-on-failure

[doc('Move branch-pinned dependencies (eacp, text-buffer, NanoTest) to their branch heads and rewrite lockstep.lock')]
update-deps:
    {{msvc}} cmake -S . -B {{build_dir}} -G Ninja -DLOCKSTEP_MODE=update
    {{msvc}} cmake -S . -B {{build_dir}} -G Ninja -DLOCKSTEP_MODE=locked

[doc('clang-format every tracked source in place')]
[unix]
format:
    git ls-files '*.h' '*.cpp' '*.mm' | xargs clang-format -i

[doc("Build PondDevTests with line coverage, run it and print the report: shows the writer, the CMake functions and line-coverage.py work together")]
[unix]
coverage-check:
    scripts/check-line-coverage.sh build-coverage

[doc('Delete the build directories')]
clean:
    cmake -E rm -rf {{build_dir}} build-coverage

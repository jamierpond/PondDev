# Working in PondDev

PondDev holds the libraries Rudder, Grazer, CowTerm, iac and DiffView share.
Each app fetches it from GitHub `main` with lockstep, so a change here reaches
an app only once it is pushed and the app's `lockstep.lock` moves to it.

## Ship every change at once

After each finished step: `just test`, `git pull --rebase`, `git add -A`,
commit, `git push origin main`. Never ask whether to commit or push.

- No AI or agent is credited on a commit: no `Co-Authored-By` trailer, no
  "Generated with" footer.
- A change an app needs is not finished until that app moves its pin
  (`just update-deps` there) and is rebuilt and installed.
- Target names, namespaces and include paths are what the apps use: renaming
  one breaks five apps.

## Code style

- Long, verbose, English, JUCE-style names. Functions are verbs that say what
  they do (`getFrameRateLabel`, not `frameRateLabel`). Predicates read as
  questions: `isX()`, `hasX()`.
- No magic numbers. Every literal that tunes behaviour is a named
  `constexpr auto`, in call sites and tests too.
- No comments by default. A comment earns its place only for what code cannot
  say: a non-obvious why, a workaround and its cause, an invariant or a unit.
  One line.
- No nested or multi-line ternaries, no nullable raw pointers (use
  `std::optional` or a reference), braces on any loop or if whose body or
  condition spans lines.
- A header exposes only what callers use.
- Library code takes values as parameters; only an app reads the environment.
- Run `just format` before committing.

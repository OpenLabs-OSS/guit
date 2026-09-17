# Guit — Architecture (Milestone 1)

## Layers

```text
UI (src/ui, src/pages, src/dialogs)
 ↓ calls slots, reacts to signals
Controller (src/controllers)
 ↓ calls repository operations
GitRepository (src/git)
 ↓ calls clean C++ Git APIs
GitClient (src/git)
 ↓ runs processes
GitProcess (src/git)
 ↓ spawns
git.exe (the user's installed Git)
```

Widgets must never execute Git directly. Git logic is testable without
the UI: `guit_core` links to all tests, and no test constructs a widget.

## Key decisions

### The user's Git executable, located dynamically

`GitLocator::findGitExecutable()` searches an explicit settings override,
then `GUIT_GIT_EXECUTABLE`, then `PATH`. Hard-coded install paths are
forbidden. An explicit-but-broken path is authoritative: Guit reports
"Git not found" instead of silently falling back to a different Git.

### No shell, ever

`GitProcess::run()` passes the executable and argument list separately to
`QProcess`. There is no shell string to quote or inject into.
`GitCommandFormatter` renders the equivalent `git ...` display string for
transparency; that string is never parsed or executed.

### Machine-readable Git output

- Status: `git status --porcelain=v1` (position-sensitive XY columns —
  the parser deliberately does not trim leading whitespace).
- Commits: `git log --format=` with `%x1f` field / `%x1e` record separators.
- Branches: `git for-each-ref --format=` with **`%1f` separators, not
  `%x1f`**. Unlike `git log`, `for-each-ref` decodes `%NN`
  percent-escapes and emits `%x1f` literally (verified against real Git).

### Error handling without exceptions

Every operation returns a result object (`GitProcessResult`,
`GitClient::RepositoryProbe`, `bool` + `openFailed()` signal). Failures
carry the `GitError` category, a user-facing message, and the raw Git
output for the details view. Nothing Git-related throws.

### Async from the start

`AsyncGitProcess` wraps `QProcess` with `finished`/`progress` signals,
timeout handling, and `cancel()`. Milestone 1 uses synchronous calls for
fast local operations; clone/fetch/pull/push will use this class.

### Settings, themes, logging

- `AppSettings` wraps `QSettings` (theme, Git override, recent
  repositories capped at 10, window geometry).
- `ThemeManager` supports system/light/dark: platform style via
  `QStyleHints::setColorScheme` (Qt 6.5+) plus an explicit dark palette.
- `Logging::initialize()` installs a timestamped console handler and an
  optional log file. No credentials ever flow through Git operations, so
  there is nothing sensitive to leak into logs.

## Tests

Qt Test binaries in `tests/`, run via CTest. Fixtures (`TestTempRepo.h`)
create deterministic temporary repositories (fixed author/committer
identity and timestamps, `main` branch, no autocrlf surprises) and are
destroyed automatically. Real user repositories are never touched.
Windows note: CTest prepends the Qt/MinGW runtime directories to `PATH`
so test executables resolve their DLLs (`ENVIRONMENT` property with
escaped `\;` separators — `$ENV{PATH}` must be re-escaped or CMake
splits it into multiple entries).

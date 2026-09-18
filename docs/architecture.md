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
- Remotes: `git remote -v` (tab-separated name/URL/kind).
- Tags: `git for-each-ref refs/tags` with `%1f` separators (peeled
  `%(*objectname)` distinguishes annotated from lightweight tags).
- Stash: `git stash list` parsed with a strict `stash@{N}:` regex.
- Commit files: `git diff-tree --name-status` needs explicit
  `--find-renames` — plumbing does not detect renames by itself.

### Error handling without exceptions

Every operation returns a result object (`GitProcessResult`,
`GitClient::RepositoryProbe`, `bool` + `openFailed()` signal). Failures
carry the `GitError` category, a user-facing message, and the raw Git
output for the details view. Nothing Git-related throws.

### Async network operations

`AsyncGitProcess` wraps `QProcess` with `finished`/`progress` signals,
timeout handling, and `cancel()`. Fast local operations use synchronous
`GitProcess::run`; fetch/pull/push/clone run through `AsyncGitProcess`
owned by `GitRepository` (network) or `RepositoryController` (clone), so
the UI stays responsive with progress and cancel. Progress handlers
consume the pipes as data arrives, therefore output is accumulated into
buffers — failure diagnostics survive in the final result.

### Operation state and conflicts

`GitRepository::operationState()` reads `.git` directly (`MERGE_HEAD`,
`rebase-merge/`, `CHERRY_PICK_HEAD`, `REVERT_HEAD`), so operations
started outside Guit are detected too. `OperationResult::conflict`
marks "stopped at conflicts" distinctly from errors: controllers route
it to conflict resolution (resolve -> stage -> continue, or abort)
instead of an error box. All `--continue` flows pin
`-c core.editor=true` so no interactive editor can hang the UI.

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

# Development Guide

This document describes how to set up a development environment, build Guit, run tests, and contribute code.

## Prerequisites

- **OS**: Windows 10/11 (primary), Linux, macOS (experimental)
- **Compiler**: MinGW-w64 GCC 13+ (Windows), GCC 11+/Clang 14+ (Linux/macOS)
- **CMake**: 3.21+
- **Build System**: Ninja
- **Qt**: 6.5+ (MinGW build on Windows)
- **Git**: 2.30+ (for development and testing)
- **C++**: C++20 compatible compiler

### Windows Setup

1. Install **MinGW-w64** (via MSYS2 or standalone)
   ```bash
   # Via MSYS2
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake
   ```
2. Install **Qt 6** (MinGW build) via Qt Online Installer
3. Add to PATH:
   ```
   C:\Qt\6.x.x\mingw_64\bin
   C:\Qt\Tools\mingw1310_64\bin
   C:\Qt\Tools\Ninja
   C:\Program Files\CMake\bin
   ```

### Linux Setup (Ubuntu/Debian)
```bash
sudo apt install cmake ninja-build g++ git qt6-base-dev qt6-tools-dev
```

### macOS Setup
```bash
brew install cmake ninja qt6 git
```

## Building

### Configure
```bash
# Debug build (default)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Build
```bash
# Debug
cmake --build build

# Release
cmake --build build-release
```

### Build Options
- `GUIT_BUILD_TESTS=ON/OFF` (default: ON) - Build test suite

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGUIT_BUILD_TESTS=OFF
```

### Running
```bash
# From build directory
./build/src/guit

# With repository path
./build/src/guit /path/to/repo
```

## Testing

### Running Tests
```bash
# All tests
ctest --test-dir build --output-on-failure

# Specific test suite
ctest --test-dir build -R tst_gitclient --output-on-failure

# Verbose
ctest --test-dir build -VV -R tst_gitclient
```

### Test Structure
- `tst_gitprocess` - Git process execution
- `tst_gitversion` - Version parsing
- `tst_gitclient` - GitClient API
- `tst_models` - Model parsing (status, log, branches)
- `tst_gitrepository` - Repository operations
- `tst_commandformatter` - Command formatting
- `tst_diff` - Diff parsing
- `tst_changes` - Staging/commit workflows
- `tst_history` - History/log operations
- `tst_branches` - Branch operations
- `tst_remotes` - Remote operations
- `tst_tags` - Tag operations
- `tst_stash` - Stash operations
- `tst_merge` - Merge operations
- `tst_rebase` - Rebase operations
- `tst_reset` - Reset/revert/cherry-pick
- `tst_graph` - Graph lane assignment
- `tst_insight` - Repository insight (LFS, submodules, worktrees)
- `tst_appsettings` - Settings persistence
- `tst_theme` - Theme system
- `tst_async` - Async controller behavior
- `tst_gitignore_crash` - Gitignore crash regression

### Test Philosophy
- **Never** touch real repositories
- Use `TestTempRepo` for isolated test repositories
- Deterministic test data (fixed timestamps, authors)
- Clean up automatically (RAII via `QTemporaryDir`)
- Test both success and failure paths

## Architecture

```
src/
├── app/              # Application entry, settings, theme
├── git/              # Git layer (Client, Repository, Models, Process)
├── controllers/      # Business logic (AsyncController + domain controllers)
├── ui/               # Widgets, pages, dialogs, delegates
├── utils/            # Utilities (Logger, TerminalLauncher, etc.)
└── app/main.cpp      # Entry point
```

### Layer Responsibilities

| Layer | Responsibility |
|-------|----------------|
| UI | Display, input, navigation, no Git logic |
| Controllers | Coordinate UI ↔ Repository, async via QtConcurrent |
| GitRepository | Repository-level operations (status, log, branches, etc.) |
| GitClient | Clean C++ API over Git commands |
| GitProcess | Low-level QProcess wrapper |

### Key Principles
- **UI never calls Git directly** - always through Controllers → Repository → Client → Process
- **Thread safety** - GitClient thread-safe; background queue via QThreadPool (1 thread)
- **Async by default** - Long operations (fetch, log, diff) run off GUI thread
- **Generation-based cancellation** - Rapid refreshes don't queue stale results
- **Immutable snapshots** - Repository state captured via `RepoLocation` snapshots

## Coding Standards

### C++ Style
- **C++20** - Use modern features (`std::optional`, `std::variant`, structured bindings)
- **RAII** - No raw owning pointers, prefer `std::unique_ptr`, `std::shared_ptr`
- **Const correctness** - Mark everything `const` by default
- **No raw owning pointers** - Use smart pointers or value semantics
- **Small focused classes** - Single responsibility
- **Clear ownership** - Parent-child for Qt objects, unique_ptr for C++

### Qt Patterns
- Parent-child ownership for QObjects
- Signals/slots for async communication
- Model/View for lists (QStandardItemModel + QListView)
- Q_PROPERTY for bindable properties
- No business logic in widgets

### Naming Conventions
| Element | Convention |
|---------|------------|
| Classes | PascalCase (`GitRepository`, `ChangesController`) |
| Methods | camelCase (`refreshStatus`, `stagePaths`) |
| Variables | camelCase (`m_repository`, `currentStatus`) |
| Constants | SCREAMING_SNAKE (`MAX_RECENT_REPOS`) |
| Enums | PascalCase (`PendingOperation`, `ResetMode`) |
| Signals | past tense (`statusChanged`, `operationFailed`) |
| Slots | present tense (`refresh`, `stage`) |

### File Organization
- One class per header/source pair
- Header guards: `#pragma once`
- Includes: system → Qt → project (alphabetical within groups)
- Forward declarations where possible

### Error Handling
- No exceptions across module boundaries
- `OperationResult` for mutating operations (ok, conflict, message, command)
- `GitProcessResult` for process execution (exit code, stdout, stderr, error)
- Signals for async completion (`finished`, `operationFailed`, `loadingChanged`)
- Never silently ignore errors

### Testing Patterns
```cpp
// TempRepo fixture - deterministic, isolated
GuitTest::TempRepo repo;
repo.writeFile("file.txt", "content");
repo.commit("message");

// GitRepository for testing
GitRepository repo;
QVERIFY(repo.open(path));
QVERIFY(repo.status().valid);

// Signal spies for async
QSignalSpy spy(&controller, &Controller::statusChanged);
controller.refresh();
QVERIFY(spy.wait(5000));
```

## Build System

### CMake
- CMake 3.21+
- Ninja generator
- C++20
- Qt6 (Core, Widgets, Concurrent)
- MinGW-w64 on Windows
- Ninja generator required

### Build Types
- Debug: `-DCMAKE_BUILD_TYPE=Debug` (default)
- Release: `-DCMAKE_BUILD_TYPE=Release`

### Targets
- `guit_core` - Static library (all non-main code)
- `guit` - Executable
- `tst_*` - Test executables (via `add_guit_test`)
- `deploy` - Windows deployment (Windows only)

### Adding New Files
1. Add `.h`/`.cpp` to `src/CMakeLists.txt` in appropriate section
2. Run `cmake` to regenerate
3. Rebuild

### Dependencies
- **Qt6**: Core, Widgets, Concurrent (Test for tests)
- **MinGW**: Runtime (libgcc, libstdc++, libwinpthread)
- **Git**: External dependency (runtime only)

## Debugging

### Logging
```cpp
Q_LOGGING_CATEGORY(guitMyFeature, "guit.myfeature")
qCDebug(guitMyFeature) << "Debug message";
qCInfo(guitMyFeature) << "Info";
qCWarning(guitMyFeature) << "Warning";
qCCritical(guitMyFeature) << "Error";
```

Enable categories at runtime:
```bash
QT_LOGGING_RULES="guit.myfeature.debug=true" ./guit
```

### Debug Build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
gdb ./build/src/guit
```

### Common Issues
- **DLL not found**: Run `windeployqt` via `cmake --build build --target deploy`
- **Qt plugins missing**: Ensure `platforms/qwindows.dll` is deployed
- **Git not found**: Ensure Git is in PATH or configure in Settings
- **Test failures**: Check `TestTempRepo` fixture setup

## Performance Profiling

### Qt Creator
- Analyze → Profiler (Valgrind/perf on Linux, Very Sleepy on Windows)

### Built-in
```cpp
QElapsedTimer timer;
timer.start();
// ... operation ...
qCDebug(log) << "Took" << timer.elapsed() << "ms";
```

## Contributing Workflow

1. Fork repository
2. Create feature branch: `git checkout -b feat/my-feature`
3. Make changes with tests
4. Run full test suite: `ctest --test-dir build --output-on-failure`
5. Format code (clang-format if configured)
6. Commit with conventional message
6. Push and create PR

### Commit Message Format
```
type(scope): brief description

Longer explanation if needed.

Fixes #123
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `build`, `ci`

Example:
```
feat(changes): add status badge colors

Each file status now shows a colored badge (M/A/D/R/??/conflict)
for immediate visual feedback.

Fixes #456
```

## Release Process

1. Update version in `CMakeLists.txt` and `src/app/Version.h.in`
2. Update CHANGELOG.md
3. Create release branch: `git checkout -b release/v0.2.0`
4. Build Release: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`
5. Run full test suite: `ctest --test-dir build-release`
6. Deploy: `cmake --build build-release --target deploy`
7. Create zip: `Compress-Archive build-release/deploy/* Guit-<version>-windows-x64.zip`
8. Generate checksums: `sha256sum *.zip *.exe > checksums.txt`
8. Tag release: `git tag -a v0.2.0 -m "Release 0.2.0"`
9. Push tag: `git push origin v0.2.0`
10. Create GitHub Release with artifacts
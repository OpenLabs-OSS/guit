# Guit — Agent Instructions

## 1. Project

Guit is a modern desktop Git client written in C++.

Its primary audience is beginner and intermediate Git users.

The core philosophy is:

> Make Git easier to use and understand without hiding Git itself.

Guit should provide a graphical interface for Git while showing users what Git is actually doing.

For important operations, Guit should be able to display the equivalent Git command.

Example:

UI action:

Create branch `feature/login`

Equivalent command:

```text
git switch -c feature/login
```

---

# 2. Technology Requirements

Use:

- C++20
- Qt 6 Widgets
- CMake
- Ninja
- MinGW-w64
- Git executable

Target platform initially:

- Windows

Architecture should avoid unnecessary platform-specific code so Linux and macOS can be supported later.

---

# 3. Hard Constraints

## Git

Guit MUST use the user's installed Git executable.

Do NOT reimplement Git.

Do NOT create a custom Git implementation.

Do NOT use libgit2 unless explicitly requested by the user.

Use Qt's process APIs, primarily `QProcess`.

Avoid executing Git through:

- `cmd.exe`
- PowerShell
- shell scripts

unless there is a specific technical reason.

Pass arguments directly to `QProcess`.

Never construct unsafe shell command strings.

Do not hard-code paths such as:

```text
C:\Program Files\Git\bin\git.exe
```

Detect Git dynamically.

Respect Git's existing configuration and credential mechanisms.

Guit must never store Git passwords, access tokens, or credentials.

---

# 4. Architecture

Use a layered architecture.

Preferred flow:

```text
UI
 ↓
Controller
 ↓
GitRepository
 ↓
GitClient
 ↓
GitProcess
 ↓
git.exe
```

Responsibilities:

### UI

Responsible for:

- displaying information
- collecting user input
- user interaction
- navigation
- visual state

UI classes must NOT directly execute Git commands.

### Controllers

Responsible for:

- coordinating UI actions
- calling repository/business logic
- updating models
- handling operation results

### GitRepository

Responsible for repository-level operations.

Examples:

- repository detection
- status
- commits
- branches
- remotes
- tags
- stashes
- merge
- rebase
- reset
- revert
- cherry-pick

### GitClient

Responsible for higher-level interaction with Git.

It should provide clean C++ APIs over Git operations.

### GitProcess

Responsible for the low-level Git process.

It should handle:

- executable path
- arguments
- working directory
- stdout
- stderr
- exit code
- process errors
- cancellation where appropriate

---

# 5. Machine-Readable Git Output

Prefer machine-readable Git output whenever Git provides it.

Examples:

```text
git status --porcelain=v1
```

Use explicit `git log --format=...` formats.

Use explicit branch formatting.

Use NUL-delimited output when appropriate.

Do NOT depend unnecessarily on human-readable Git output because it may change between versions or configurations.

---

# 6. C++ Guidelines

Use modern C++20.

Prefer:

- RAII
- const correctness
- scoped enums
- `std::optional`
- `std::variant` where appropriate
- smart pointers when ownership requires them
- value semantics where appropriate
- clear ownership
- small focused classes
- strong types where useful

Avoid:

- raw owning pointers
- global mutable state
- giant classes
- giant functions
- unnecessary inheritance
- excessive macros
- duplicated logic
- magic numbers
- unnecessary templates
- clever code that is difficult for a beginner to understand

Do not use advanced C++ merely to demonstrate that advanced C++ exists.

The code should be understandable to a developer learning C++.

---

# 7. Qt Guidelines

Use Qt 6 Widgets.

Prefer normal Qt patterns:

- `QObject`
- signals/slots
- models/views where appropriate
- layouts
- `QProcess`
- `QSettings`
- Qt resource system

Do not put the entire application into `main.cpp`.

Avoid putting business logic inside widgets.

Prefer reusable widgets when a UI component has meaningful independent behavior.

Use Qt's model/view architecture for large lists and tables where appropriate.

---

# 8. Error Handling

Never silently ignore errors.

Every Git operation should be able to communicate:

- success
- Git command failure
- invalid repository
- invalid input
- Git unavailable
- process failure
- cancellation

Preserve useful Git output.

User-facing errors should be understandable.

Technical details should be available when useful, such as through an expandable details/output area.

Do not expose confusing raw errors as the only UI.

---

# 9. Asynchronous Operations

Never freeze the UI for long-running operations.

Operations such as:

- clone
- fetch
- pull
- push
- large history loading
- large diff generation

should run asynchronously where appropriate.

Show useful progress/status information.

Provide cancellation when practical.

---

# 10. UI Principles

Guit should feel like a modern native desktop application.

Use:

- clear visual hierarchy
- consistent spacing
- readable typography
- sensible layouts
- clear buttons
- useful empty states
- useful loading states
- useful error states
- keyboard accessibility

Support:

- Light theme
- Dark theme
- System theme

Do not overuse:

- gradients
- animations
- excessive rounded containers
- decorative effects

Do not clone another Git application's UI.

Guit should have its own visual identity.

---

# 11. Beginner Experience

The educational aspect is a core feature.

Where appropriate, show the equivalent Git command.

Explain concepts such as:

- working tree
- staging
- commit
- branch
- remote
- merge
- rebase
- stash
- reset
- revert
- cherry-pick
- HEAD

Explanations should be concise and contextual.

Do not turn the application into a textbook.

Provide additional information through tooltips, help panels, or expandable explanations.

---

# 12. Dangerous Operations

Operations that can cause data loss must have appropriate confirmation.

Examples:

- discard changes
- hard reset
- force push
- branch deletion
- stash deletion
- destructive cleanup

The UI should clearly explain consequences.

Do not silently perform destructive operations.

---

# 13. Testing

Tests must never modify the user's real repositories.

Use temporary repositories created specifically for tests.

Tests should cover:

- Git detection
- Git process execution
- exit codes
- status parsing
- branch parsing
- commit parsing
- remote parsing
- tag parsing
- stash parsing
- diff parsing
- error handling
- repository operations

Use deterministic test repositories.

Tests must clean up temporary repositories.

---

# 14. Build System

Use CMake.

Use Ninja as the build generator.

Use C++20.

The project must support a clean out-of-source build.

Example:

```text
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Do not use:

- MSVC
- Visual Studio generators
- NMake

unless the user explicitly requests them.

The primary Windows compiler is MinGW-w64.

---

# 15. Dependencies

Do not add dependencies casually.

Before introducing a third-party dependency:

1. Determine whether Qt/C++ already provides the required functionality.
2. Determine whether the dependency is actually necessary.
3. Consider maintenance and licensing.
4. Keep the dependency justified and documented.

Prefer a small dependency footprint.

---

# 16. Security

Never commit:

- passwords
- API keys
- access tokens
- credentials
- private certificates
- personal authentication data

Never log credentials.

Never store Git credentials.

Use Git credential helpers where applicable.

Do not execute arbitrary shell strings.

Validate paths and user input where necessary.

---

# 17. Git Commands and User Transparency

Important operations should expose their Git equivalent.

Examples:

```text
git status
git add <file>
git restore --staged <file>
git commit -m "message"
git switch -c feature/name
git merge feature/name
git rebase main
git push origin main
```

Commands displayed to users should accurately represent the operation.

Never display a fake command.

---

# 18. Code Organization

Do not create giant source files.

Use clear modules.

Suggested organization:

```text
src/
├── app/
├── git/
├── models/
├── controllers/
├── ui/
├── pages/
├── dialogs/
└── utils/
```

The exact structure may evolve if there is a good architectural reason.

Do not create empty abstractions just because a folder exists.

Create files when functionality actually requires them.

---

# 19. Development Strategy

Development is divided into five large milestones.

The agent should work autonomously within the current milestone.

Do NOT stop after every small feature.

For each major subsystem:

1. Inspect existing code.
2. Implement it.
3. Build.
4. Run relevant tests.
5. Fix problems.
6. Continue.

Only stop when the current milestone is complete or when a serious blocker requires user input.

---

# 20. Milestones

## Milestone 1 — Foundation

Implement:

- CMake project
- C++20 configuration
- Qt 6 Widgets
- application entry point
- main window
- basic application architecture
- sidebar/navigation
- GitProcess
- GitClient
- Git detection
- repository detection
- basic repository abstraction
- basic models
- logging
- error handling
- initial theme infrastructure

---

## Milestone 2 — Core Git

Implement:

- repository opening
- recent repositories
- repository status
- staged changes
- unstaged changes
- stage
- unstage
- stage all
- unstage all
- discard
- diff viewer
- commit
- amend
- history
- commit details
- branches
- create branch
- switch branch
- rename branch
- delete branch
- branch comparison

---

## Milestone 3 — Advanced Git

Implement:

- remotes
- add/remove/edit remotes
- fetch
- pull
- push
- tags
- stashes
- merge
- merge conflict detection
- conflict resolution
- rebase
- rebase conflict handling
- reset
- revert
- cherry-pick

---

## Milestone 4 — Complete UX

Implement:

- Git graph
- commit search
- reflog
- Git command viewer
- beginner explanations
- beginner/advanced mode
- settings
- keyboard shortcuts
- notifications
- terminal integration
- Git LFS detection/basic integration
- submodules
- worktrees
- `.gitignore` helper
- repository information
- polished loading states
- polished empty states
- polished error states

---

**## Milestone 5 — Release Engineering and Public Release**

Milestone 5 transforms the completed Guit application into a reproducible, distributable, production-quality open-source Windows application.

The goal is not to add large new Git features. The goal is to make the existing application reliable, testable, buildable, distributable, and ready for public GitHub release.

The agent should work autonomously through the entire milestone and must not stop after individual tasks. Build and test after each major subsystem, fix problems, and continue.

---

### 5.1 Release Baseline

Before making changes:

1. Inspect the complete repository.
2. Inspect the current Git status and diff.
3. Review the existing build configuration.
4. Review existing tests.
5. Review the existing deployment configuration.
6. Build the current Release configuration.
7. Run the complete existing test suite.
8. Record any existing failures or warnings.

Do not discard working functionality.

Do not rewrite working architecture merely to make the release system look different.

The manually verified deployment is the baseline for the automated deployment system.

The current known-good Windows deployment contains:

```text
guit.exe
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
platforms/
└── qwindows.dll
```

The automated deployment must reproduce an equivalent working package.

---

### 5.2 Release Build Configuration

Create a reliable Windows Release build using:

- C++20
- Qt 6
- CMake
- Ninja
- MinGW-w64

The documented Release workflow should work from a clean checkout.

Example:

```text
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

Do not require MSVC, Visual Studio, or NMake.

Do not hard-code the user's local Qt installation path into the repository.

If Qt discovery requires an environment variable or documented CMake configuration, document it clearly.

---

### 5.3 Automated Qt and MinGW Deployment

Fix and improve the existing CMake deployment target.

The final project must not require manually copying DLLs for every release.

A command such as:

```text
cmake --build build-release --target deploy
```

should create a complete runnable deployment directory.

The deployment system must:

1. Copy the Guit executable.
2. Deploy required Qt runtime DLLs.
3. Deploy required MinGW runtime DLLs.
4. Deploy required Qt plugins.
5. Deploy the Windows platform plugin:

```text
platforms/qwindows.dll
```

6. Preserve the directory structure required by Qt.
7. Fail clearly if required deployment files cannot be found.
8. Never silently produce an incomplete deployment.

The previously discovered `windeployqt` platform-plugin discovery problem must be handled robustly.

Do not rely on a development machine's PATH after deployment.

Do not copy unrelated Qt plugins unnecessarily.

The resulting deployment must run on a machine that does not have the Guit development environment installed.

---

### 5.4 Deployment Validation

Add a deployment validation step.

It should verify at minimum:

```text
guit.exe
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
platforms/qwindows.dll
```

If a required dependency is missing, deployment validation must fail rather than producing a package that appears successful but cannot launch.

Where practical, validate the executable's runtime dependencies rather than relying only on filenames.

---

### 5.5 Portable Windows Package

Create a reproducible portable package.

The package should have a predictable name such as:

```text
Guit-<version>-windows-x64.zip
```

It must contain only the files necessary to run Guit.

It must not contain:

- source code
- build files
- CMake cache files
- test repositories
- development logs
- compiler intermediate files
- personal paths
- IDE files

The ZIP must be tested after extraction into a clean directory.

---

### 5.6 Windows Installer

Add a Windows installer.

Use a lightweight and well-established Windows installer technology such as Inno Setup unless an existing project constraint makes another solution more appropriate.

The installer should:

- install Guit
- install all required runtime files
- create Start Menu shortcuts
- optionally create a desktop shortcut
- register an uninstaller
- support upgrading a previous Guit installation
- preserve appropriate user configuration when upgrading
- allow the user to choose the installation directory
- launch Guit optionally after installation
- uninstall cleanly

Do not bundle Git itself unless explicitly required.

Guit must continue to use the user's installed Git executable.

The installer must not store Git credentials.

---

### 5.7 Versioning

Introduce a single authoritative application version.

The version should be used consistently by:

- the application
- CMake
- package names
- installer
- GitHub release artifacts
- About dialog where applicable

Do not duplicate version strings unnecessarily.

Prepare the project for semantic-style releases such as:

```text
v0.1.0
v0.2.0
v1.0.0
```

Do not claim API stability merely because semantic versioning is used.

---

### 5.8 Automated Tests

Expand the existing test suite where useful.

Tests must cover important existing functionality and edge cases.

At minimum consider:

#### Git

- Git executable detection
- Git unavailable
- process startup failure
- non-zero exit codes
- stdout/stderr handling
- cancellation where supported

#### Repositories

- valid repository
- invalid directory
- empty repository
- repository with no commits
- repository with many commits
- repository with uncommitted changes
- repository with staged changes
- repository with deleted files
- repository with renamed files
- repository with unusual filenames

#### Branches

- branch creation
- switching
- rename
- deletion
- branch comparison
- detached HEAD

#### History

- normal history
- multiple branches
- merge commits
- root commit
- empty history
- graph boundaries

#### Remotes

- no remotes
- one remote
- multiple remotes
- invalid remote
- fetch/pull/push failures

#### Stashes

- no stash
- one stash
- multiple stashes
- stash creation
- stash application
- stash deletion

#### Merge/Rebase

- successful operation
- conflict
- aborted operation
- invalid operation

#### UI/state

- opening repositories
- switching pages
- loading states
- empty states
- error states
- `.gitignore` handling
- startup behavior

Tests must never modify the user's real repositories.

Use temporary deterministic repositories.

---

### 5.9 Compiler Warnings

Release builds should be reviewed for compiler warnings.

Fix warnings introduced by Guit's code whenever practical.

Do not suppress warnings globally merely to make the build appear clean.

Warnings that are intentionally retained must have a documented reason.

---

### 5.10 Performance and Responsiveness

Perform a final performance audit.

Pay particular attention to:

- repository opening
- large repositories
- history loading
- graph generation
- large diffs
- status loading
- branch loading
- remote operations
- filesystem scanning
- page navigation

No normal user action should unnecessarily freeze the GUI.

Do not introduce unnecessary background threads.

Do not access QWidget objects from worker threads.

Preserve the existing asynchronous architecture.

---

### 5.11 Stability and Crash Audit

Perform a final crash/stability audit.

Test at minimum:

- application startup
- opening valid repository
- opening invalid directory
- opening `.git` directory
- switching pages repeatedly
- rapidly switching pages while loading
- closing during a Git operation
- `.gitignore` dialog
- empty repository
- repository with no `.gitignore`
- repository with an empty `.gitignore`
- repository with a large `.gitignore`
- missing Git executable
- failed Git commands
- deleted/moved repository
- inaccessible repository

Fix root causes rather than hiding crashes with generic exception handling or excessive null checks.

---

### 5.12 GitHub Actions CI

Create GitHub Actions CI.

At minimum CI should:

1. Check out the repository.
2. Configure the project.
3. Build it.
4. Run tests.
5. Report failures clearly.

The Windows CI environment must use the project's supported toolchain.

Do not introduce MSVC if MinGW-w64 is the supported Windows compiler.

Keep CI configuration maintainable.

If Qt installation/setup is required, use a stable documented approach.

---

### 5.13 Release Workflow

Create a GitHub Actions release workflow capable of producing release artifacts.

The intended flow is:

```text
Git tag
   ↓
Release workflow
   ↓
Configure
   ↓
Build Release
   ↓
Run tests
   ↓
Deploy Qt + MinGW
   ↓
Validate deployment
   ↓
Create portable ZIP
   ↓
Build Windows installer
   ↓
Generate checksums
   ↓
Upload release artifacts
```

Do not automatically publish a real public release during development unless explicitly requested by the user.

The workflow should be safe to test without publishing a release.

---

### 5.14 Checksums

Generate SHA-256 checksums for release artifacts.

For example:

```text
Guit-0.1.0-windows-x64.zip
Guit-0.1.0-windows-x64-Setup.exe
```

Produce a checksum file or equivalent release metadata.

Do not claim cryptographic signing unless signing is actually implemented.

---

### 5.15 Documentation

Prepare the repository for public developers and users.

At minimum provide:

```text
README.md
CONTRIBUTING.md
CODE_OF_CONDUCT.md
SECURITY.md
```

Also maintain appropriate documentation for:

- architecture
- building from source
- development setup
- testing
- Git concepts
- deployment/release process

README should explain:

- what Guit is
- who it is for
- major features
- screenshots or visual examples where appropriate
- supported platform
- requirements
- installation
- building from source
- testing
- contributing
- license
- project status

Do not claim features that do not actually work.

---

### 5.16 Public Repository Audit

Before declaring the milestone complete, inspect the entire repository for:

- secrets
- credentials
- API keys
- personal paths
- generated binaries
- build directories
- test repositories
- temporary files
- IDE files
- unnecessary generated files
- accidental large files

Verify `.gitignore`.

Review:

```text
git status
git diff
git ls-files
```

Search source and documentation for accidental local machine paths.

Do not commit release binaries unless the release process specifically requires them.

---

### 5.17 Clean-Machine Verification

Perform a final clean-environment test.

The portable deployment must run without requiring:

- Qt installation
- MinGW installation
- CMake
- Ninja
- Qt Creator
- Visual Studio
- project source tree

Git may remain an external prerequisite because Guit intentionally uses the user's installed Git executable.

Verify that Guit produces a clear error when Git is unavailable.

---

### 5.18 Final Release Checklist

Before completing Milestone 5, verify:

- [ ] Clean Release configuration succeeds.
- [ ] Release compilation succeeds.
- [ ] All tests pass.
- [ ] No known reproducible crash remains.
- [ ] No known normal-use UI freeze remains.
- [ ] Deployment is automated.
- [ ] Deployment validation succeeds.
- [ ] Portable ZIP works.
- [ ] Windows installer works.
- [ ] Installer uninstall works.
- [ ] Upgrade installation works.
- [ ] Git remains an external dependency.
- [ ] CI passes.
- [ ] Release workflow is functional.
- [ ] SHA-256 checksums are generated.
- [ ] README is complete.
- [ ] CONTRIBUTING exists.
- [ ] CODE_OF_CONDUCT exists.
- [ ] SECURITY exists.
- [ ] Architecture documentation is current.
- [ ] Development documentation is current.
- [ ] No secrets are present.
- [ ] No personal development paths are present.
- [ ] No generated build artifacts are accidentally tracked.
- [ ] Git working tree is clean after intended commits.
- [ ] Release artifacts have been tested outside the development environment.

Only after all applicable items are satisfied should the milestone be considered complete.

---

### 5.19 Public Release Readiness

Milestone 5 completion means Guit is technically prepared for a public GitHub release.

Do not publish a GitHub release automatically unless explicitly requested.

At completion, provide the user with:

1. Build status.
2. Test results.
3. CI status.
4. Deployment status.
5. Installer status.
6. Portable package status.
7. Documentation status.
8. Known limitations.
9. Recommended version number.
10. Exact remaining steps, if any, before the first public release.

Do not declare Guit production-ready if important checklist items remain incomplete.

Do not hide known problems merely to complete the milestone.

# 21. Git Commit Strategy

Create logical commits while working.

Do not create one enormous commit for an entire milestone.

Good examples:

```text
chore: initialize Qt project

feat(git): add Git process abstraction

feat(git): detect installed Git

feat(repository): add repository detection

feat(status): implement status parsing

feat(changes): add staging workflow

feat(diff): add diff viewer

feat(commit): add commit workflow

feat(history): add commit history

feat(branch): add branch management

feat(remote): add remote operations

feat(merge): add merge workflow
```

Commits should represent coherent changes.

Do not commit unrelated work together.

---

# 22. Before Commits

Before creating commits:

```text
git status
git diff
```

Review the changes.

Do not commit:

- build directories
- generated binaries
- IDE temporary files
- personal paths
- test repositories
- temporary files
- secrets

---

# 23. Documentation

Important architectural decisions should be documented.

When introducing a significant C++ or Qt concept, documentation should briefly explain:

1. What it is.
2. Why Guit uses it.
3. What problem it solves.

Keep documentation concise.

---

# 24. Scope Control

Work quickly, but do not create technical debt deliberately.

Do not:

- implement fake features
- create dead buttons
- create fake Git operations
- add unrelated features
- add unnecessary dependencies
- rewrite working systems without reason
- prematurely optimize
- over-engineer simple functionality

If a feature is not ready, do not pretend it is.

---

# 25. Blockers

If a problem is straightforward:

Solve it and continue.

If there is a significant architectural decision:

Stop and explain:

- the problem
- why it matters
- possible approaches
- the recommended approach
- affected parts of the project

Do not silently make major architectural changes.

---

# 26. Final Quality Standard

The final application must be:

- buildable
- testable
- maintainable
- secure
- responsive
- documented
- usable
- genuinely functional

Every implemented feature must actually work.

No fake functionality.

No dead buttons.

No intentionally broken intermediate state.

The project should be something a developer could reasonably publish as an open-source GitHub project.

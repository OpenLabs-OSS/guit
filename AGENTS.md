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

## Milestone 5 — Release

Implement:

- comprehensive tests
- edge-case testing
- performance improvements
- accessibility improvements
- GitHub Actions CI
- Windows build pipeline
- packaging
- release configuration
- README
- CONTRIBUTING
- CODE_OF_CONDUCT
- SECURITY
- architecture documentation
- development documentation
- Git concepts documentation

Perform a final production-readiness review.

---

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

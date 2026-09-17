# Guit — Project Specification

## 1. Overview

Guit is a modern desktop Git client written in C++ and Qt.

It is designed for users who want a graphical Git workflow without losing visibility into how Git actually works.

Guit's main differentiator is educational transparency.

The application should make Git easier without hiding Git.

---

# 2. Core Philosophy

Guit should answer two questions simultaneously:

1. "How do I accomplish this?"
2. "What is Git actually doing?"

For important operations, the user should be able to see the equivalent Git command.

Example:

```text
Create branch: feature/login

Git command:
git switch -c feature/login
```

The displayed command must correspond to the actual operation.

---

# 3. Platform

Initial target:

- Windows

Future target:

- Linux
- macOS

The architecture should minimize unnecessary Windows-specific assumptions.

---

# 4. Technology

- C++20
- Qt 6 Widgets
- CMake
- Ninja
- MinGW-w64
- Git executable

No Electron.

No JavaScript/TypeScript application layer.

No custom Git implementation.

---

# 5. Main Application Layout

The main window should generally contain:

```text
┌──────────────────────────────────────────────────────────┐
│ Repository / current context                         ⋯ │
├───────────────┬──────────────────────────────────────────┤
│               │                                          │
│ Changes       │                                          │
│ History       │               Main Content               │
│ Branches      │                                          │
│ Tags          │                                          │
│ Stashes       │                                          │
│ Remotes       │                                          │
│               │                                          │
│               │                                          │
├───────────────┴──────────────────────────────────────────┤
│ Branch: main          Clean          Git 2.x.x            │
└──────────────────────────────────────────────────────────┘
```

The exact design can evolve.

The UI should feel like a polished native developer tool.

---

# 6. Git Integration

Guit uses the installed Git executable.

Examples:

```text
git --version
git status --porcelain=v1
git log
git branch
git diff
git commit
git push
git pull
```

Git operations should be isolated behind a clean C++ API.

The UI should never directly launch Git.

---

# 7. Repository Management

Support:

- open repository
- detect repository
- locate repository root
- recent repositories
- clone repository
- initialize repository
- repository information

The application should gracefully handle:

- invalid directories
- repositories with no commits
- detached HEAD
- missing Git
- inaccessible directories

---

# 8. Changes

Display:

- staged changes
- unstaged changes
- untracked files
- modified files
- added files
- deleted files
- renamed files

Actions:

- stage
- unstage
- stage all
- unstage all
- discard
- refresh

The UI should clearly communicate whether a file is staged, unstaged, or both.

---

# 9. Diff Viewer

Support:

- working tree diff
- staged diff
- commit diff
- file diff
- added files
- deleted files
- renamed files
- binary files

The diff should be readable and navigable.

Large diffs should not freeze the UI.

---

# 10. Commits

Support:

- commit message
- extended description
- commit
- amend
- commit and push

Validate user input.

Show useful errors.

Show the equivalent Git command.

After committing, refresh relevant repository state.

---

# 11. History

Display commit history.

Each commit should expose:

- abbreviated hash
- full hash
- author
- date
- subject

Selecting a commit should display:

- full message
- author
- timestamp
- parents
- changed files
- diff

History should eventually support searching.

---

# 12. Git Graph

Provide a visual representation of commit relationships.

Display:

- commits
- branches
- merges
- branch pointers
- HEAD

The graph should help users understand Git history rather than merely decorate it.

---

# 13. Branches

Support:

- list branches
- create branch
- switch branch
- rename branch
- delete branch
- create branch from commit
- compare branches
- merge branch
- rebase branch

Clearly distinguish:

- current branch
- local branches
- remote-tracking branches

Warn before destructive branch operations.

---

# 14. Remotes

Support:

- list remotes
- add remote
- edit remote
- remove remote
- rename remote
- fetch
- pull
- push

Display useful remote information.

Do not store credentials.

Allow Git credential helpers to handle authentication.

---

# 15. Tags

Support:

- list tags
- create tag
- inspect tag
- delete tag
- push tag

Support annotated tags where appropriate.

---

# 16. Stashes

Support:

- create stash
- list stashes
- inspect stash
- apply stash
- pop stash
- delete stash
- clear stashes

Explain stash behavior to beginners.

---

# 17. Merge

Support:

- selecting a branch to merge
- performing merge
- detecting conflicts
- displaying conflicted files
- continuing merge
- aborting merge

Explain what a merge does.

---

# 18. Conflict Resolution

Provide a conflict-oriented UI.

Display:

- conflicted files
- conflict status
- relevant diff information

Support the Git conflict lifecycle:

```text
merge/rebase
      ↓
conflict
      ↓
resolve files
      ↓
stage resolved files
      ↓
continue
```

Allow aborting the operation.

---

# 19. Rebase

Support:

- rebase onto branch
- rebase onto commit where appropriate
- conflict detection
- continue
- skip
- abort

Explain rebase before executing it.

---

# 20. Reset

Support:

- soft reset
- mixed reset
- hard reset

Clearly explain differences.

Hard reset must require explicit confirmation.

---

# 21. Revert

Support reverting commits.

Explain that revert creates a new commit rather than moving branch history backward.

---

# 22. Cherry-Pick

Support:

- selecting commit
- cherry-pick
- conflict handling
- continue
- abort

---

# 23. Compare

Allow comparisons between:

- working tree
- staged state
- commits
- branches
- tags

---

# 24. Search

History search should support:

- commit message
- author
- hash

Search should remain responsive with larger histories.

---

# 25. Reflog

Display reflog entries.

Explain why reflog exists.

Allow users to inspect entries without hiding the underlying Git terminology.

---

# 26. Git LFS

Detect:

- Git LFS installation
- repositories using LFS

Provide basic information and useful status.

Do not reimplement Git LFS.

---

# 27. Submodules

Display submodules.

Provide basic operations such as:

- initialize
- update
- synchronize

Clearly show submodule state.

---

# 28. Worktrees

Display existing worktrees.

Provide basic worktree management.

---

# 29. `.gitignore` Helper

Provide a helper for editing `.gitignore`.

Requirements:

- preserve existing content
- avoid accidental overwrites
- clearly show modifications
- support common ignore patterns

---

# 30. Terminal Integration

Allow opening the current repository in the user's preferred terminal.

Guit should complement the terminal, not attempt to replace it.

---

# 31. Beginner Mode

Beginner mode should emphasize:

- clear labels
- explanations
- safe defaults
- contextual Git terminology
- command visibility

Examples:

Instead of only:

```text
Stage
```

the UI can explain:

```text
Stage

Move this change into Git's staging area so it can be included in the next commit.
```

---

# 32. Advanced Mode

Advanced mode can expose:

- detailed Git output
- reflog
- advanced rebase controls
- reset options
- additional repository information
- lower-level Git details

---

# 33. Command Viewer

Important operations should have an accessible command representation.

Examples:

```text
git add README.md
```

```text
git commit -m "Initial commit"
```

```text
git switch -c feature/login
```

```text
git fetch origin
```

```text
git merge feature/login
```

This is a core Guit feature.

---

# 34. Settings

Settings should eventually include:

- theme
- system/light/dark
- Git executable
- terminal
- editor
- confirmation behavior
- beginner/advanced mode
- keyboard shortcuts

Use appropriate Qt persistence mechanisms.

---

# 35. Notifications

Useful notifications may include:

- commit completed
- push completed
- pull completed
- fetch completed
- clone completed
- operation failed

Notifications should be informative rather than excessive.

---

# 36. Accessibility

Support:

- keyboard navigation
- keyboard shortcuts
- visible focus
- readable typography
- accessible labels
- sensible tab order
- adequate contrast

---

# 37. Performance

Avoid blocking the GUI thread.

Be especially careful with:

- large repositories
- large histories
- large diffs
- clone
- fetch
- pull
- push

Only optimize measured problems.

---

# 38. Error States

Provide clear UI states for:

- Git unavailable
- invalid repository
- empty repository
- detached HEAD
- merge conflict
- rebase conflict
- network failure
- authentication failure
- permission failure
- Git command failure

Do not hide Git's useful diagnostic output.

---

# 39. Testing

Use temporary repositories.

Example test lifecycle:

```text
create temporary directory
        ↓
initialize Git repository
        ↓
create test files
        ↓
perform Git operation
        ↓
verify result
        ↓
destroy temporary repository
```

Tests must never modify the developer's real repositories.

---

# 40. CI

GitHub Actions should eventually:

- configure the project
- install/use required Qt dependencies
- build with MinGW
- run tests
- report failures

---

# 41. Packaging

The final project should produce a usable Windows release.

The release should include required Qt runtime components.

Git remains an external dependency.

---

# 42. Documentation

The repository should contain:

```text
README.md
LICENSE
CONTRIBUTING.md
CODE_OF_CONDUCT.md
SECURITY.md

docs/
├── architecture.md
├── development.md
└── git-concepts.md
```

The README should clearly explain:

- what Guit is
- why it exists
- screenshots
- features
- installation
- requirements
- development
- contributing
- license

---

# 43. Future Possibilities

These are not required for the initial release but should not be architecturally impossible:

- Linux support
- macOS support
- GitHub integration
- GitLab integration
- commit signing
- GPG/SSH status information
- customizable UI
- plugins/extensions
- repository-specific settings

Do not implement these merely because they are listed here.

---

# 44. Release Standard

Before calling Guit release-ready:

- clean build succeeds
- tests pass
- core Git operations work
- errors are handled
- dangerous operations are protected
- UI is responsive
- no secrets exist in the repository
- no fake features exist
- no dead buttons exist
- documentation is complete
- Windows packaging works
- CI passes

The final result should be a real open-source application rather than a prototype.

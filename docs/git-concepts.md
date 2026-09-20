# Git Concepts Reference

This document explains the Git concepts used in Guit, to help users understand what's happening under the hood.

## Core Concepts

### Repository
A Git repository is a directory containing your project files plus a hidden `.git` folder that stores all history, branches, tags, and configuration.

```
my-project/
├── .git/          # Git database (hidden)
├── src/
├── README.md
└── .gitignore
```

### Working Tree
The files you see and edit in your project folder. This is your "working copy."

### Index (Staging Area)
An intermediate area where you prepare changes before committing. Think of it as a "shopping cart" for your next commit.

```
Working Tree → [git add] → Staging Area → [git commit] → Repository
```

### Commit
A snapshot of your entire project at a point in time. Each commit has:
- **Hash** (SHA-1): Unique identifier (e.g., `a1b2c3d`)
- **Author**: Name and email
- **Date**: Timestamp
- **Message**: Description of changes
- **Parents**: Previous commit(s) - forms the history chain

```
... ← parent ← parent ← current commit ← child ← child ...
```

### HEAD
A pointer to the current commit/branch you're on. Like a bookmark saying "you are here."

```
HEAD → main → a1b2c3d (latest commit)
```

### Branch
A lightweight movable pointer to a commit. The default branch is usually `main`.

```
main → a1b2c3d (HEAD)
feature/login → e5f6g7h
```

### Remote
Another copy of the repository, typically on a server (GitHub, GitLab, etc.).

```
origin → https://github.com/user/repo.git
upstream → https://github.com/original/repo.git
```

### Remote-tracking Branch
A local reference to a branch on a remote. Updated when you fetch.

```
origin/main → a1b2c3d (last known state of main on origin)
```

## Common Workflows

### Making Changes

```
1. Edit files in working tree
2. git add <file>          # Stage changes
3. git commit -m "message" # Create commit
```

### Staging
- **Staged**: Changes added to index, will be in next commit
- **Unstaged**: Changes in working tree, not yet staged
- **Both**: File has both staged and unstaged changes

### Viewing History
```
git log                    # Full history
git log --oneline          # Compact
git log --graph            # Visual graph
git log --all              # All branches
```

### Branching
```
git branch                 # List branches
git branch <name>          # Create branch
git switch <name>          # Switch to branch
git switch -c <name>       # Create and switch
git branch -d <name>       # Delete (safe)
git branch -D <name>       # Force delete
```

### Merging
```
git merge <branch>         # Merge branch into current
git merge --no-ff <branch> # Always create merge commit
```

**Conflict**: When same lines changed differently. Git pauses for you to resolve.

### Rebasing
```
git rebase <base>          # Replay commits on top of base
```
Rewrites history - only for **local, unshared** commits.

### Remote Operations
```
git remote -v              # List remotes
git remote add <name> <url> # Add remote
git fetch <remote>         # Download objects, update remote-tracking
git pull <remote> <branch> # Fetch + merge
git push <remote> <branch> # Upload commits
```

### Tags
```
git tag                    # List tags
git tag v1.0               # Lightweight tag
git tag -a v1.0 -m "msg"   # Annotated tag (recommended)
git push origin v1.0       # Push specific tag
git push origin --tags     # Push all tags
```

### Stashing
```
git stash                  # Save changes temporarily
git stash list             # List stashes
git stash pop              # Restore and remove
git stash apply            # Restore, keep in stash
git stash drop             # Delete stash
```

### Resetting
```
git reset --soft <commit>  # Move HEAD, keep index & working tree
git reset --mixed <commit> # Move HEAD, reset index (default)
git reset --hard <commit>  # Move HEAD, reset index & working tree (DESTRUCTIVE)
```

### Reverting
```
git revert <commit>        # Create new commit that undoes changes
```
Safe for shared history - adds a new commit instead of rewriting.

### Cherry-picking
```
git cherry-pick <commit>   # Apply commit onto current branch
```

## Guit's Visual Indicators

### File Status Badges
| Badge | Meaning |
|-------|---------|
| `M` | Modified |
| `A` | Added (staged) |
| `D` | Deleted |
| `R` | Renamed |
| `??` | Untracked |
| `!!` | Ignored |
| `U` | Conflicted |

### Branch Indicators
- `●` - Current branch (HEAD)
- `→` - Upstream tracking
- `(remote)` - Remote-tracking branch

### Commit Indicators
- `●` - HEAD
- `merge` - Merge commit (two parents)

## Common Git Commands Reference

| Action | Command |
|--------|---------|
| Initialize repo | `git init` |
| Clone repo | `git clone <url>` |
| Status | `git status` |
| Add all | `git add -A` |
| Add file | `git add <file>` |
| Unstage | `git restore --staged <file>` |
| Discard changes | `git restore <file>` |
| Commit | `git commit -m "msg"` |
| Amend commit | `git commit --amend` |
| Log (compact) | `git log --oneline --graph` |
| Branches | `git branch -a` |
| Switch branch | `git switch <name>` |
| Create branch | `git switch -c <name>` |
| Delete branch | `git branch -d <name>` |
| Merge | `git merge <branch>` |
| Rebase | `git rebase <base>` |
| Fetch | `git fetch` |
| Pull | `git pull` |
| Push | `git push` |
| Tag | `git tag -a v1.0 -m "msg"` |
| Stash | `git stash` |
| Stash pop | `git stash pop` |
| Reset soft | `git reset --soft HEAD~1` |
| Reset hard | `git reset --hard HEAD~1` |
| Revert | `git revert <commit>` |
| Cherry-pick | `git cherry-pick <commit>` |
| Reflog | `git reflog` |

## Guit's Git Transparency

Guit shows the exact Git command for every important operation:

| Action | Guit Shows |
|--------|------------|
| Stage file | `git add -- file.txt` |
| Unstage | `git restore --staged file.txt` |
| Discard | `git restore --source=HEAD --staged --worktree -- file.txt` |
| Commit | `git commit -m "message"` |
| Amend | `git commit --amend -m "message"` |
| Switch branch | `git switch branch` |
| Create branch | `git switch -c feature/name` |
| Merge | `git merge --no-ff feature` |
| Rebase | `git rebase main` |
| Fetch | `git fetch origin` |
| Pull | `git pull origin` |
| Push | `git push origin main` |
| Tag | `git tag --annotate --message "..." v1.0` |
| Stash | `git stash push --message "wip"` |
| Reset | `git reset --hard HEAD~1` |
| Revert | `git revert --no-edit abc123` |

This transparency helps you learn Git while using Guit.
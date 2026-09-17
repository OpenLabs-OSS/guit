#pragma once

#include <QString>

namespace Guit
{

// Every Git operation reports its outcome through these types so that
// failures are never silently ignored (see AGENTS.md section 8).
enum class GitError
{
    None,
    GitNotFound,       // no usable git executable was found
    NotARepository,    // the directory is not inside a Git working tree
    InvalidInput,      // caller passed an invalid path, argument, ...
    ProcessStartFailed,// QProcess could not start git at all
    TimedOut,          // git did not finish within the requested timeout
    CommandFailed,     // git ran but exited with a non-zero exit code
    Cancelled          // the operation was cancelled by the user
};

QString gitErrorMessage(GitError error);

// Raw outcome of running one git process. Higher layers (GitClient,
// GitRepository) translate this into domain-specific results.
struct GitProcessResult
{
    bool started = false;
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
    GitError error = GitError::None;
    QString errorMessage;

    [[nodiscard]] bool isSuccess() const
    {
        return started && error == GitError::None && exitCode == 0;
    }
};

} // namespace Guit

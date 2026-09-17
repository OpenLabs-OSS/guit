#include "GitResult.h"

namespace Guit
{

QString gitErrorMessage(GitError error)
{
    switch (error) {
    case GitError::None:
        return QStringLiteral("No error.");
    case GitError::GitNotFound:
        return QStringLiteral("No Git executable was found. Install Git and make sure it is on PATH.");
    case GitError::NotARepository:
        return QStringLiteral("The selected directory is not inside a Git repository.");
    case GitError::InvalidInput:
        return QStringLiteral("Invalid input was passed to the Git operation.");
    case GitError::ProcessStartFailed:
        return QStringLiteral("The Git process could not be started.");
    case GitError::TimedOut:
        return QStringLiteral("The Git operation timed out.");
    case GitError::CommandFailed:
        return QStringLiteral("Git reported an error.");
    case GitError::Cancelled:
        return QStringLiteral("The Git operation was cancelled.");
    }
    return QStringLiteral("Unknown Git error.");
}

} // namespace Guit

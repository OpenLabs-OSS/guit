#include "BranchController.h"

namespace Guit
{

BranchController::BranchController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

bool BranchController::validateBranchName(const QString &name) const
{
    return m_repository->validateBranchName(name);
}

void BranchController::refresh()
{
    m_branches = m_repository->branches();
    emit branchesChanged(m_branches);
}

void BranchController::create(const QString &name, const QString &startPoint, bool checkout)
{
    handleResult(m_repository->createBranch(name, startPoint, checkout));
}

void BranchController::switchTo(const QString &name)
{
    handleResult(m_repository->switchBranch(name));
}

void BranchController::rename(const QString &oldName, const QString &newName)
{
    handleResult(m_repository->renameBranch(oldName, newName));
}

void BranchController::remove(const QString &name, bool force)
{
    handleResult(m_repository->deleteBranch(name, force));
}

void BranchController::compare(const QString &from, const QString &to)
{
    const AheadBehind counts = m_repository->aheadBehind(from, to);
    const QList<FileDiff> diffs = m_repository->compareDiff(from, to);
    emit comparisonReady(from, to, counts, diffs);
}

void BranchController::handleResult(const OperationResult &result)
{
    refresh();
    m_repository->refreshHead();
    emit headChanged();
    if (result.ok)
        emit branchOperationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

} // namespace Guit

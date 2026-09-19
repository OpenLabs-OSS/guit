#include "BranchController.h"

namespace Guit
{

BranchController::BranchController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

bool BranchController::validateBranchName(const QString &name) const
{
    return m_repository->validateBranchName(name);
}

void BranchController::refresh()
{
    GitRepository *repository = m_repository;
    submit<QList<BranchInfo>>([repository]() { return repository->branches(); },
                              [this](const QList<BranchInfo> &branches) {
                                  m_branches = branches;
                                  emit branchesChanged(m_branches);
                                  emit loadingChanged(false);
                              });
}

void BranchController::create(const QString &name, const QString &startPoint, bool checkout)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name, startPoint, checkout]() {
        return repository->createBranch(name, startPoint, checkout);
    },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void BranchController::switchTo(const QString &name)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name]() { return repository->switchBranch(name); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void BranchController::rename(const QString &oldName, const QString &newName)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, oldName, newName]() { return repository->renameBranch(oldName, newName); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

void BranchController::remove(const QString &name, bool force)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name, force]() { return repository->deleteBranch(name, force); },
                            [this](const OperationResult &result) { reloadAfter(result); });
}

struct BranchComparison
{
    QString from;
    QString to;
    AheadBehind counts;
    QList<FileDiff> diffs;
};

void BranchController::compare(const QString &from, const QString &to)
{
    GitRepository *repository = m_repository;
    submit<BranchComparison>(
        [repository, from, to]() {
            BranchComparison comparison;
            comparison.from = from;
            comparison.to = to;
            comparison.counts = repository->aheadBehind(from, to);
            comparison.diffs = repository->compareDiff(from, to);
            return comparison;
        },
        [this](const BranchComparison &comparison) {
            emit comparisonReady(comparison.from, comparison.to, comparison.counts, comparison.diffs);
            emit loadingChanged(false);
        });
}

void BranchController::reloadAfter(const OperationResult &result)
{
    if (!result.ok) {
        emit loadingChanged(false);
        emit operationFailed(result.message, {}, result.command);
        return;
    }
    GitRepository *repository = m_repository;
    submit<QList<BranchInfo>>(
        [repository]() {
            repository->refreshHead();
            return repository->branches();
        },
        [this, result](const QList<BranchInfo> &branches) {
            m_branches = branches;
            emit branchesChanged(m_branches);
            emit headChanged();
            emit branchOperationDone(result.message, result.command);
            emit loadingChanged(false);
        });
}

} // namespace Guit

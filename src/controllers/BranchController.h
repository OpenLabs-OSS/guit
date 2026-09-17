#pragma once

#include "../git/GitModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates the Branches page: listing, create/switch/rename/delete,
// name validation, and pairwise comparison.
class BranchController : public QObject
{
    Q_OBJECT

public:
    explicit BranchController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] QList<BranchInfo> branches() const { return m_branches; }
    bool validateBranchName(const QString &name) const;

public slots:
    void refresh();
    void create(const QString &name, const QString &startPoint, bool checkout);
    void switchTo(const QString &name);
    void rename(const QString &oldName, const QString &newName);
    void remove(const QString &name, bool force);
    void compare(const QString &from, const QString &to);

signals:
    void branchesChanged(const QList<Guit::BranchInfo> &branches);
    void branchOperationDone(const QString &message, const QString &command);
    void comparisonReady(const QString &from, const QString &to,
                         const Guit::AheadBehind &aheadBehind,
                         const QList<Guit::FileDiff> &diffs);
    void operationFailed(const QString &reason, const QString &details, const QString &command);
    void headChanged();

private:
    void handleResult(const OperationResult &result);

    GitRepository *m_repository = nullptr;
    QList<BranchInfo> m_branches;
};

} // namespace Guit

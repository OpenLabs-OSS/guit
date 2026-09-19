#pragma once

#include "AsyncController.h"
#include "../git/AdvancedModels.h"
#include "../git/GitModels.h"
#include "../git/GitRepository.h"

#include <QObject>
#include <QString>

namespace Guit
{

// Coordinates local tag management. Pushing a tag needs the network layer,
// so TagController emits pushRequested and MainWindow routes it to the
// RemoteController that owns network operations.
class TagController : public AsyncController
{
    Q_OBJECT

public:
    explicit TagController(GitRepository *repository, QObject *parent = nullptr);

    [[nodiscard]] QList<TagInfo> tags() const { return m_tags; }

public slots:
    void refresh();
    void create(const QString &name, const QString &message, const QString &target);
    void remove(const QString &name);
    void inspect(const QString &name);
    void requestPush(const QString &remote, const QString &tag);

signals:
    void tagsChanged(const QList<Guit::TagInfo> &tags);
    void tagOperationDone(const QString &message, const QString &command);
    void tagDetails(const Guit::TagInfo &tag, const Guit::CommitDetails &details);
    void pushRequested(const QString &remote, const QString &tag);
    void operationFailed(const QString &reason, const QString &details, const QString &command);

private:
    GitRepository *m_repository = nullptr;
    QList<TagInfo> m_tags;
};

} // namespace Guit

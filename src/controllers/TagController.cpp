#include "TagController.h"

namespace Guit
{

TagController::TagController(GitRepository *repository, QObject *parent)
    : AsyncController(parent)
    , m_repository(repository)
{
}

void TagController::refresh()
{
    GitRepository *repository = m_repository;
    submit<QList<TagInfo>>([repository]() { return repository->tags(); },
                           [this](const QList<TagInfo> &tags) {
                               m_tags = tags;
                               emit tagsChanged(m_tags);
                               emit loadingChanged(false);
                           });
}

void TagController::create(const QString &name, const QString &message, const QString &target)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name, message, target]() {
        return repository->createTag(name, message, target);
    },
                            [this](const OperationResult &result) {
                                if (!result.ok) {
                                    emit loadingChanged(false);
                                    emit operationFailed(result.message, {}, result.command);
                                    return;
                                }
                                refresh();
                                emit tagOperationDone(result.message, result.command);
                            });
}

void TagController::remove(const QString &name)
{
    GitRepository *repository = m_repository;
    submit<OperationResult>([repository, name]() { return repository->deleteTag(name); },
                            [this](const OperationResult &result) {
                                if (!result.ok) {
                                    emit loadingChanged(false);
                                    emit operationFailed(result.message, {}, result.command);
                                    return;
                                }
                                refresh();
                                emit tagOperationDone(result.message, result.command);
                            });
}

void TagController::inspect(const QString &name)
{
    for (const TagInfo &tag : m_tags) {
        if (tag.name == name) {
            GitRepository *repository = m_repository;
            submit<CommitDetails>([repository, name]() { return repository->showTag(name); },
                                  [this, tag](const CommitDetails &details) {
                                      emit tagDetails(tag, details);
                                      emit loadingChanged(false);
                                  });
            return;
        }
    }
    emit operationFailed(tr("Tag “%1” was not found.").arg(name), {}, {});
}

void TagController::requestPush(const QString &remote, const QString &tag)
{
    emit pushRequested(remote, tag);
}

} // namespace Guit

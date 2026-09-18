#include "TagController.h"

namespace Guit
{

TagController::TagController(GitRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

void TagController::refresh()
{
    m_tags = m_repository->tags();
    emit tagsChanged(m_tags);
}

void TagController::create(const QString &name, const QString &message, const QString &target)
{
    const OperationResult result = m_repository->createTag(name, message, target);
    refresh();
    if (result.ok)
        emit tagOperationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

void TagController::remove(const QString &name)
{
    const OperationResult result = m_repository->deleteTag(name);
    refresh();
    if (result.ok)
        emit tagOperationDone(result.message, result.command);
    else
        emit operationFailed(result.message, {}, result.command);
}

void TagController::inspect(const QString &name)
{
    for (const TagInfo &tag : m_tags) {
        if (tag.name == name) {
            emit tagDetails(tag, m_repository->showTag(name));
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

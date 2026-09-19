#pragma once

#include "../git/GitRepository.h"

#include <QFutureWatcher>
#include <QObject>
#include <QtConcurrent>

namespace Guit
{

// Base for controllers whose Git work runs off the GUI thread. submit()
// runs a task on the repository's serial background queue and delivers
// the result on the GUI thread; results from superseded generations
// (rapid refreshes, repo switches) are dropped. loadingChanged drives
// page loading indicators and disables action buttons while work runs.
class AsyncController : public QObject
{
    Q_OBJECT

public:
    explicit AsyncController(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void loadingChanged(bool loading);

protected:
    template <typename Result, typename Task, typename Delivered>
    void submit(Task task, Delivered delivered)
    {
        ++m_generation;
        const int generation = m_generation;
        emit loadingChanged(true);
        QFuture<Result> future = QtConcurrent::run(GitRepository::backgroundPool(), task);
        auto *watcher = new QFutureWatcher<Result>(this);
        connect(watcher, &QFutureWatcher<Result>::finished, this,
                [this, watcher, generation, delivered]() {
                    watcher->deleteLater();
                    if (generation != m_generation)
                        return; // superseded; loading stays on until the latest delivers
                    delivered(watcher->result());
                });
        watcher->setFuture(future);
    }

private:
    int m_generation = 0;
};

} // namespace Guit

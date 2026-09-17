#pragma once

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Guit
{

// Repository context page. Shows real Milestone 1 data only: Git
// availability, repository root, branch/HEAD state, and recent
// repositories. Everything displayed here actually works.
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    struct Data
    {
        bool gitAvailable = false;
        QString gitVersion;
        QString gitExecutable;
        bool hasRepository = false;
        QString rootPath;
        QString branchDisplay;
        QString headDisplay;
        QString commitHash;
        QStringList recentRepositories;
    };

    explicit OverviewPage(QWidget *parent = nullptr);

    void setData(const Data &data);

signals:
    void openRequested();
    void recentOpened(const QString &path);

private:
    QLabel *m_gitBanner = nullptr;
    QLabel *m_repoTitle = nullptr;
    QLabel *m_repoDetails = nullptr;
    QLabel *m_commandLabel = nullptr;
    QPushButton *m_openButton = nullptr;
    QListWidget *m_recentList = nullptr;
    QLabel *m_recentLabel = nullptr;
};

} // namespace Guit

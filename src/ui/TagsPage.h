#pragma once

#include "DiffViewer.h"
#include "../controllers/TagController.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

namespace Guit
{

// Tag management page: list with annotated/lightweight distinction,
// create, inspect (message + target commit diff), delete, and push.
class TagsPage : public QWidget
{
    Q_OBJECT

public:
    explicit TagsPage(TagController *controller, const QStringList &remoteNames, QWidget *parent = nullptr);

    void setRemoteNames(const QStringList &remotes);

public slots:
    void refresh();

private slots:
    void onTagsChanged(const QList<TagInfo> &tags);
    void onTagDetails(const TagInfo &tag, const CommitDetails &details);
    void onCreate();
    void onInspect();
    void onDelete();
    void onPush();
    void setLoading(bool loading);

private:
    QString selectedTag() const;

    TagController *m_controller = nullptr;
    QListWidget *m_tagList = nullptr;
    QLineEdit *m_filterBox = nullptr;
    QLabel *m_infoLabel = nullptr;
    DiffViewer *m_diff = nullptr;
    QLabel *m_commandLabel = nullptr;
    QComboBox *m_remoteBox = nullptr;
    QProgressBar *m_loadingBar = nullptr;
    QList<QPushButton *> m_actionButtons;
    QList<TagInfo> m_tags;
};

} // namespace Guit

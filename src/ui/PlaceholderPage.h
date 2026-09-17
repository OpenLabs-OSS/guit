#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace Guit
{

// Honest placeholder for views arriving in later milestones. It navigates
// like a real page, explains the underlying Git concept, and names the Git
// command that will power it — but contains no fake controls and performs
// no fake operations.
class PlaceholderPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderPage(const QString &title,
                             const QString &explanation,
                             const QString &gitCommand,
                             const QString &milestone,
                             QWidget *parent = nullptr);

private:
    QLabel *m_title = nullptr;
    QLabel *m_explanation = nullptr;
    QLabel *m_command = nullptr;
    QLabel *m_milestone = nullptr;
};

} // namespace Guit

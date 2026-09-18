#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

namespace Guit
{

class RepositoryInfoController;

// Edits .gitignore with live file content, common-pattern presets, and
// explicit save. Existing content is preserved: presets append.
class GitignoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitignoreDialog(RepositoryInfoController *controller, const QString &currentContent, QWidget *parent = nullptr);

private slots:
    void addPreset(const QString &preset);
    void save();

private:
    RepositoryInfoController *m_controller = nullptr;
    QTextEdit *m_editor = nullptr;
};

} // namespace Guit

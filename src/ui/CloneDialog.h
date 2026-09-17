#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Guit
{

// Collects a clone source URL and target directory. Cloning itself runs
// asynchronously through RepositoryController with progress.
class CloneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloneDialog(QWidget *parent = nullptr);

    [[nodiscard]] QString sourceUrl() const;
    [[nodiscard]] QString targetDirectory() const;

private slots:
    void browse();
    void validate();

private:
    QLineEdit *m_urlEdit = nullptr;
    QLineEdit *m_dirEdit = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_cloneButton = nullptr;
};

} // namespace Guit

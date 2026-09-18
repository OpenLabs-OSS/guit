#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Guit
{

// Saves uncommitted changes onto the stash with an optional message.
class StashDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StashDialog(QWidget *parent = nullptr);

    [[nodiscard]] QString message() const;
    [[nodiscard]] bool includeUntracked() const;

private:
    QLineEdit *m_messageEdit = nullptr;
    QCheckBox *m_untrackedBox = nullptr;
};

} // namespace Guit

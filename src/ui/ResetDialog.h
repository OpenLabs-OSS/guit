#pragma once

#include "../git/GitRepository.h"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

namespace Guit
{

// Resets the current branch to a target commit. The mode radios explain
// soft/mixed/hard; hard reset is destructive and confirmed by the caller.
class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResetDialog(const QString &target, QWidget *parent = nullptr);

    [[nodiscard]] ResetMode mode() const;

private:
    QRadioButton *m_softButton = nullptr;
    QRadioButton *m_mixedButton = nullptr;
    QRadioButton *m_hardButton = nullptr;
};

} // namespace Guit

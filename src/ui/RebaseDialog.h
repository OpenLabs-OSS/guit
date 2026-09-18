#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace Guit
{

// Starts a rebase with an explicit warning: unlike merge, rebase rewrites
// history, so it must not be used on commits others already have.
class RebaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RebaseDialog(const QStringList &branches, const QString &current, QWidget *parent = nullptr);

    [[nodiscard]] QString onto() const;

private slots:
    void validate();

private:
    QComboBox *m_ontoBox = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit

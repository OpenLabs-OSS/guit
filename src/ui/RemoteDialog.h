#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Guit
{

// Creates or edits a remote (name + URL). Used for add, rename (name only
// is prefilled), and URL editing.
class RemoteDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Add,
        Edit
    };

    explicit RemoteDialog(Mode mode, QWidget *parent = nullptr);

    [[nodiscard]] QString remoteName() const;
    [[nodiscard]] QString remoteUrl() const;

    void setRemoteName(const QString &name);
    void setRemoteUrl(const QString &url);
    void setNameEditable(bool editable);

private slots:
    void validate();

private:
    Mode m_mode = Mode::Add;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit

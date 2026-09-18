#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace Guit
{

// Pushes a branch or tag to a remote, with upstream tracking and guarded
// force options. Also handles remote-branch deletion in Delete mode.
class PushDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Push,
        DeleteRemoteBranch
    };

    explicit PushDialog(Mode mode, const QStringList &remotes, const QStringList &sources, QWidget *parent = nullptr);

    [[nodiscard]] QString remote() const;
    [[nodiscard]] QString source() const;
    [[nodiscard]] bool setUpstream() const;
    [[nodiscard]] bool force() const;

    void setCurrentRemote(const QString &remote);
    void setCurrentSource(const QString &source);

private slots:
    void validate();

private:
    Mode m_mode = Mode::Push;
    QComboBox *m_remoteBox = nullptr;
    QComboBox *m_sourceBox = nullptr;
    QCheckBox *m_upstreamBox = nullptr;
    QCheckBox *m_forceBox = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit

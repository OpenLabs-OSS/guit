#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

namespace Guit
{

// Creates a tag: lightweight (name only, points at a commit) or annotated
// (with message, tagger, and date — preferred for releases).
class TagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagDialog(QWidget *parent = nullptr);

    [[nodiscard]] QString tagName() const;
    [[nodiscard]] QString message() const;
    [[nodiscard]] QString target() const;

private slots:
    void validate();

private:
    QLineEdit *m_nameEdit = nullptr;
    QCheckBox *m_annotatedBox = nullptr;
    QTextEdit *m_messageEdit = nullptr;
    QLineEdit *m_targetEdit = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace Guit

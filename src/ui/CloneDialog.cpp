#include "CloneDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace Guit
{

CloneDialog::CloneDialog(QWidget *parent)
    : QDialog(parent)
    , m_urlEdit(new QLineEdit(this))
    , m_dirEdit(new QLineEdit(this))
    , m_hintLabel(new QLabel(this))
{
    setWindowTitle(tr("Clone Repository"));
    m_urlEdit->setPlaceholderText(tr("https://example.com/user/repo.git"));
    m_urlEdit->setToolTip(tr("Any URL or local path Git can clone from. Authentication is handled by Git itself."));
    m_hintLabel->setWordWrap(true);

    auto *browseButton = new QPushButton(tr("Browse…"), this);

    auto *form = new QFormLayout();
    form->addRow(tr("Source URL:"), m_urlEdit);
    form->addRow(tr("Target directory:"), m_dirEdit);

    auto *dirRow = new QHBoxLayout();
    dirRow->addWidget(m_dirEdit, 1);
    dirRow->addWidget(browseButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_cloneButton = buttons->button(QDialogButtonBox::Ok);
    m_cloneButton->setText(tr("Clone"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(dirRow);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browseButton, &QPushButton::clicked, this, &CloneDialog::browse);
    connect(m_urlEdit, &QLineEdit::textChanged, this, &CloneDialog::validate);
    connect(m_dirEdit, &QLineEdit::textChanged, this, &CloneDialog::validate);
    validate();
}

QString CloneDialog::sourceUrl() const
{
    return m_urlEdit->text().trimmed();
}

QString CloneDialog::targetDirectory() const
{
    return m_dirEdit->text().trimmed();
}

void CloneDialog::browse()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Clone Target"));
    if (!dir.isEmpty())
        m_dirEdit->setText(dir);
}

void CloneDialog::validate()
{
    // Cloning into a non-empty directory fails; require an empty or
    // non-existent target so the error never comes as a surprise.
    const QString dir = targetDirectory();
    bool dirOk = !dir.isEmpty() && (!QDir(dir).exists() || QDir(dir).isEmpty());
    const bool urlOk = !sourceUrl().isEmpty();
    m_cloneButton->setEnabled(urlOk && dirOk);
    if (!urlOk) {
        m_hintLabel->setText(tr("Enter the repository URL to clone from."));
    } else if (!dirOk) {
        m_hintLabel->setText(tr("Choose an empty or new target directory."));
    } else {
        m_hintLabel->setText(tr("Git: git clone %1 %2").arg(sourceUrl(), dir));
    }
}

} // namespace Guit

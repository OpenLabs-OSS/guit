#include "PlaceholderPage.h"

namespace Guit
{

PlaceholderPage::PlaceholderPage(const QString &title,
                                 const QString &explanation,
                                 const QString &gitCommand,
                                 const QString &milestone,
                                 QWidget *parent)
    : QWidget(parent)
    , m_title(new QLabel(title, this))
    , m_explanation(new QLabel(explanation, this))
    , m_command(new QLabel(tr("Git: %1").arg(gitCommand), this))
    , m_milestone(new QLabel(milestone, this))
{
    QFont titleFont = m_title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    m_title->setFont(titleFont);

    m_explanation->setWordWrap(true);
    m_command->setWordWrap(true);
    m_command->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_command->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_title);
    layout->addWidget(m_explanation);
    layout->addWidget(m_command);
    layout->addWidget(m_milestone);
    layout->addStretch(1);
}

} // namespace Guit

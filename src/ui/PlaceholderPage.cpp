#include "Theme.h"
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
    Theme::applyTitle(m_title);

    m_explanation->setWordWrap(true);
    Theme::applySecondary(m_explanation);
    m_command->setWordWrap(true);
    m_command->setTextInteractionFlags(Qt::TextSelectableByMouse);
    Theme::applyMono(m_command);
    Theme::applyMuted(m_milestone);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::pageMargin(), Theme::pageMargin(), Theme::pageMargin(), Theme::pageMargin());
    layout->setSpacing(Theme::controlSpacing());
    layout->addWidget(m_title);
    layout->addWidget(m_explanation);
    layout->addWidget(m_command);
    layout->addWidget(m_milestone);
    layout->addStretch(1);
}

} // namespace Guit

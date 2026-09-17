#include "OverviewPage.h"

namespace Guit
{

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
    , m_gitBanner(new QLabel(this))
    , m_repoTitle(new QLabel(this))
    , m_repoDetails(new QLabel(this))
    , m_commandLabel(new QLabel(this))
    , m_openButton(new QPushButton(tr("Open Repository…"), this))
    , m_recentList(new QListWidget(this))
    , m_recentLabel(new QLabel(tr("Recent repositories"), this))
{
    m_gitBanner->setWordWrap(true);
    m_repoTitle->setWordWrap(true);
    m_repoDetails->setWordWrap(true);
    m_repoDetails->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));

    QFont titleFont = m_repoTitle->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    m_repoTitle->setFont(titleFont);

    QFont sectionFont = m_recentLabel->font();
    sectionFont.setBold(true);
    m_recentLabel->setFont(sectionFont);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_gitBanner);
    layout->addWidget(m_repoTitle);
    layout->addWidget(m_repoDetails);
    layout->addWidget(m_commandLabel);
    layout->addWidget(m_openButton, 0, Qt::AlignLeft);
    layout->addWidget(m_recentLabel);
    layout->addWidget(m_recentList, 1);
    layout->addStretch(1);

    connect(m_openButton, &QPushButton::clicked, this, &OverviewPage::openRequested);
    connect(m_recentList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        if (item != nullptr)
            emit recentOpened(item->data(Qt::UserRole).toString());
    });
}

void OverviewPage::setData(const Data &data)
{
    if (!data.gitAvailable) {
        m_gitBanner->setText(tr("Git was not found. Install Git and make sure it is on PATH, then restart Guit."));
        m_gitBanner->setStyleSheet(QStringLiteral("color: #B00020;"));
    } else {
        m_gitBanner->setText(tr("Git %1 ready.").arg(data.gitVersion));
        m_gitBanner->setStyleSheet(QString());
    }

    if (!data.hasRepository) {
        m_repoTitle->setText(tr("No repository open"));
        m_repoDetails->setText(tr("Open a Git repository to inspect it. Guit uses your installed Git — nothing is hidden from you."));
        m_commandLabel->setText(tr("Git: git rev-parse --show-toplevel"));
    } else {
        m_repoTitle->setText(data.rootPath);
        m_repoDetails->setText(tr("Branch: %1\nHEAD: %2%3")
                                   .arg(data.branchDisplay, data.headDisplay,
                                        data.commitHash.isEmpty() ? QString()
                                                                  : QStringLiteral("\nCommit: ") + data.commitHash));
        m_commandLabel->setText(tr("Git: git branch --show-current · git rev-parse --verify HEAD"));
    }

    m_recentList->clear();
    for (const QString &recent : data.recentRepositories) {
        auto *item = new QListWidgetItem(recent, m_recentList);
        item->setData(Qt::UserRole, recent);
        item->setToolTip(tr("Double-click to open this repository."));
    }
    const bool hasRecents = !data.recentRepositories.isEmpty();
    m_recentLabel->setVisible(hasRecents);
    m_recentList->setVisible(hasRecents);
}

} // namespace Guit

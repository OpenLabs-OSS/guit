#include "DiffViewer.h"

#include <QFontDatabase>
#include <QVBoxLayout>

namespace Guit
{

DiffHighlighter::DiffHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    m_added.setForeground(QColor(0x1A7F37));
    m_removed.setForeground(QColor(0xB00020));
    m_hunk.setForeground(QColor(0x0A58CA));
    m_hunk.setFontWeight(QFont::Bold);
    m_header.setFontWeight(QFont::Bold);
    m_binary.setForeground(Qt::darkGray);
    m_binary.setFontItalic(true);
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(QStringLiteral("Binary files ")) || text.startsWith(QStringLiteral("GIT binary patch"))) {
        setFormat(0, text.size(), m_binary);
        return;
    }
    if (text.startsWith(QLatin1Char('+')) && !text.startsWith(QStringLiteral("+++"))) {
        setFormat(0, text.size(), m_added);
        return;
    }
    if (text.startsWith(QLatin1Char('-')) && !text.startsWith(QStringLiteral("---"))) {
        setFormat(0, text.size(), m_removed);
        return;
    }
    if (text.startsWith(QStringLiteral("@@"))) {
        setFormat(0, text.size(), m_hunk);
        return;
    }
    if (text.startsWith(QStringLiteral("diff --git"))) {
        setFormat(0, text.size(), m_header);
    }
}

DiffViewer::DiffViewer(QWidget *parent)
    : QWidget(parent)
    , m_fileList(new QListWidget(this))
    , m_diffText(new QPlainTextEdit(this))
{
    m_diffText->setReadOnly(true);
    m_diffText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_diffText->setLineWrapMode(QPlainTextEdit::NoWrap);
    new DiffHighlighter(m_diffText->document());

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_fileList);
    splitter->addWidget(m_diffText);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({100, 400});

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    connect(m_fileList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= m_diffs.size()) {
            m_diffText->clear();
            return;
        }
        m_diffText->setPlainText(renderFile(m_diffs.at(row)));
    });
}

void DiffViewer::setDiffs(const QList<FileDiff> &diffs)
{
    m_diffs = diffs;
    m_fileList->clear();
    m_diffText->clear();
    if (diffs.isEmpty()) {
        auto *empty = new QListWidgetItem(tr("No differences to show."), m_fileList);
        empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
        return;
    }
    for (const FileDiff &file : diffs) {
        QString label = file.displayPath() + QStringLiteral("  (") + file.summary() + QLatin1Char(')');
        if (file.isRename)
            label = file.oldPath + QStringLiteral(" → ") + file.newPath
                + QStringLiteral("  (") + file.summary() + QLatin1Char(')');
        auto *item = new QListWidgetItem(label, m_fileList);
        item->setToolTip(file.isBinary ? tr("Binary file: content is not shown as text.")
                                       : tr("%1 added, %2 removed").arg(file.added).arg(file.removed));
    }
    m_fileList->setCurrentRow(0);
}

void DiffViewer::clear()
{
    setDiffs({});
}

QString DiffViewer::renderFile(const FileDiff &file)
{
    QString text;
    text += QStringLiteral("diff --git a/%1 b/%2\n").arg(file.oldPath, file.newPath);
    if (file.isRename)
        text += QStringLiteral("rename from %1\nrename to %2\n").arg(file.oldPath, file.newPath);
    if (file.isNewFile)
        text += QStringLiteral("new file\n");
    if (file.isDeleted)
        text += QStringLiteral("deleted file\n");
    if (file.isBinary) {
        text += QStringLiteral("Binary files differ.\n");
        return text;
    }
    for (const DiffHunk &hunk : file.hunks) {
        text += QStringLiteral("@@ -%1,%2 +%3,%4 @@%5\n")
                    .arg(hunk.oldStart)
                    .arg(hunk.oldCount)
                    .arg(hunk.newStart)
                    .arg(hunk.newCount)
                    .arg(hunk.sectionHeading);
        for (const DiffLine &line : hunk.lines)
            text += line.origin + line.content + QLatin1Char('\n');
    }
    return text;
}

} // namespace Guit

#include "DiffViewer.h"

#include "Theme.h"

#include <QApplication>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QVBoxLayout>

namespace Guit
{

DiffHighlighter::DiffHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    refresh();
}

void DiffHighlighter::refresh()
{
    const ThemeSpec &spec = Theme::currentSpec();
    m_added.setForeground(spec.diffAddText);
    m_removed.setForeground(spec.diffDelText);
    m_addedBg.setForeground(spec.diffAddText);
    m_addedBg.setBackground(spec.diffAddBg);
    m_removedBg.setForeground(spec.diffDelText);
    m_removedBg.setBackground(spec.diffDelBg);
    m_hunk.setForeground(spec.hunkText);
    m_hunk.setFontWeight(QFont::Bold);
    m_header.setFontWeight(QFont::Bold);
    m_header.setForeground(spec.secondaryText);
    m_binary.setForeground(spec.mutedText);
    m_binary.setFontItalic(true);
    rehighlight();
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(QStringLiteral("Binary files ")) || text.startsWith(QStringLiteral("GIT binary patch"))) {
        setFormat(0, text.size(), m_binary);
        return;
    }
    if (text.startsWith(QLatin1Char('+')) && !text.startsWith(QStringLiteral("+++"))) {
        setFormat(0, text.size(), m_addedBg);
        return;
    }
    if (text.startsWith(QLatin1Char('-')) && !text.startsWith(QStringLiteral("---"))) {
        setFormat(0, text.size(), m_removedBg);
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

class GutterWidget : public QWidget
{
public:
    explicit GutterWidget(DiffTextEdit *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override { return QSize(m_editor->gutterWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override { m_editor->paintGutter(event); }

private:
    DiffTextEdit *m_editor = nullptr;
};

DiffTextEdit::DiffTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_gutter(new GutterWidget(this))
{
    setReadOnly(true);
    setFont(Theme::monoFont());
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameShape(QFrame::NoFrame);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &DiffTextEdit::updateGutterWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &DiffTextEdit::updateGutter);
    updateGutterWidth();
}

void DiffTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    m_gutter->setGeometry(QRect(contents.left(), contents.top(), gutterWidth(), contents.height()));
}

void DiffTextEdit::updateGutterWidth()
{
    setViewportMargins(gutterWidth(), 0, 0, 0);
}

void DiffTextEdit::updateGutter(const QRect &rect, int dy)
{
    if (dy != 0)
        m_gutter->scroll(0, dy);
    else
        m_gutter->update(0, rect.y(), m_gutter->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateGutterWidth();
}

int DiffTextEdit::gutterWidth() const
{
    int digits = 1;
    int lines = qMax(1, blockCount());
    while (lines >= 10) {
        lines /= 10;
        ++digits;
    }
    const int charWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    return 8 + charWidth * digits + 8;
}

void DiffTextEdit::paintGutter(QPaintEvent *event)
{
    QPainter painter(m_gutter);
    painter.fillRect(event->rect(), Theme::currentSpec().window);
    painter.setPen(Theme::currentSpec().mutedText);
    painter.setFont(font());
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    const int lineHeight = static_cast<int>(blockBoundingRect(block).height());
    const int right = m_gutter->width() - 8;
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && top >= event->rect().top())
            painter.drawText(0, top, right, lineHeight, Qt::AlignRight, QString::number(blockNumber + 1));
        block = block.next();
        top += lineHeight;
        ++blockNumber;
    }
}

DiffViewer::DiffViewer(QWidget *parent)
    : QWidget(parent)
    , m_fileList(new QListWidget(this))
    , m_diffText(new DiffTextEdit(this))
    , m_highlighter(new DiffHighlighter(m_diffText->document()))
{
    m_fileList->setAlternatingRowColors(true);
    m_fileList->setUniformItemSizes(true);

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
    // Theme switches rebuild palettes globally; the highlighter must
    // follow because it caches its own formats.
    qApp->installEventFilter(this);
}

bool DiffViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange && m_highlighter != nullptr)
        m_highlighter->refresh();
    return QWidget::eventFilter(watched, event);
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

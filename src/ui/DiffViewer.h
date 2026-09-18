#pragma once

#include "../git/DiffInfo.h"

#include <QListWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QWidget>

namespace Guit
{

// Highlights added/removed/hunk-header lines using the active theme spec.
// Rebuilds its formats whenever the application palette changes so theme
// switches never leave stale colors behind.
class DiffHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DiffHighlighter(QTextDocument *document);

    void refresh();

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_added;
    QTextCharFormat m_removed;
    QTextCharFormat m_addedBg;
    QTextCharFormat m_removedBg;
    QTextCharFormat m_hunk;
    QTextCharFormat m_header;
    QTextCharFormat m_binary;
};

// Monospace diff text with a muted line-number gutter.
class DiffTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DiffTextEdit(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateGutterWidth();
    void updateGutter(const QRect &rect, int dy);

private:
    void paintGutter(QPaintEvent *event);
    int gutterWidth() const;

    QWidget *m_gutter = nullptr;
    friend class GutterWidget;
};

// Reusable diff viewer: file list on top, colored unified diff below.
// Used by the Changes, History, and Branches pages.
class DiffViewer : public QWidget
{
    Q_OBJECT

public:
    explicit DiffViewer(QWidget *parent = nullptr);

    void setDiffs(const QList<FileDiff> &diffs);
    void clear();

protected:
    // ApplicationPaletteChange (theme switch) rebuilds the cached
    // highlighter formats; QCoreApplication::paletteChanged is deprecated.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    static QString renderFile(const FileDiff &file);

    QListWidget *m_fileList = nullptr;
    DiffTextEdit *m_diffText = nullptr;
    DiffHighlighter *m_highlighter = nullptr;
    QList<FileDiff> m_diffs;
};

} // namespace Guit

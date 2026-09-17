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

// Colors added/removed/hunk-header lines in the diff text.
class DiffHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DiffHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_added;
    QTextCharFormat m_removed;
    QTextCharFormat m_hunk;
    QTextCharFormat m_header;
    QTextCharFormat m_binary;
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

private:
    static QString renderFile(const FileDiff &file);

    QListWidget *m_fileList = nullptr;
    QPlainTextEdit *m_diffText = nullptr;
    QList<FileDiff> m_diffs;
};

} // namespace Guit

#pragma once

#include <QDialog>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace Guit
{

// Static help dialogs: keyboard shortcuts and concise Git concept
// explanations. Content is fixed text — no fake controls.
class ShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutsDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Keyboard Shortcuts"));
        resize(440, 380);
        auto *view = new QTextBrowser(this);
        view->setHtml(tr("<h3>Navigation</h3>"
                         "<p><b>Alt+1 … Alt+7</b> — Overview, Changes, History, Branches, Tags, Stashes, Remotes</p>"
                         "<h3>Repository</h3>"
                         "<p><b>Ctrl+O</b> — Open repository<br>"
                         "<b>F5</b> — Refresh the current page</p>"
                         "<h3>Changes</h3>"
                         "<p><b>Ctrl+Return</b> — Commit staged changes</p>"
                         "<h3>Application</h3>"
                         "<p><b>Ctrl+Q</b> — Quit</p>"));
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(view);
    }
};

class ConceptsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConceptsDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Git Concepts"));
        resize(520, 480);
        auto *view = new QTextBrowser(this);
        view->setHtml(tr("<h3>Working tree</h3><p>Your files on disk. Editing happens here.</p>"
                         "<h3>Staging area (index)</h3><p>Where you assemble the next commit. "
                         "<i>git add</i> moves changes here; only staged changes are committed.</p>"
                         "<h3>Commit</h3><p>A snapshot with a message, author, date, and parent commit(s). "
                         "History is a chain of commits.</p>"
                         "<h3>Branch</h3><p>A movable pointer to a commit. Switching branches moves HEAD and updates your files.</p>"
                         "<h3>HEAD</h3><p>What is currently checked out: usually a branch, sometimes a bare commit (detached HEAD).</p>"
                         "<h3>Remote</h3><p>Another copy of the repository, e.g. <i>origin</i>. "
                         "<i>fetch</i> downloads, <i>pull</i> fetches and integrates, <i>push</i> sends.</p>"
                         "<h3>Merge</h3><p>Joins two histories with a merge commit. Conflicts pause the merge for resolution.</p>"
                         "<h3>Rebase</h3><p>Replays commits onto another branch. Rewrites history — only for unshared work.</p>"
                         "<h3>Stash</h3><p>Shelves uncommitted changes to restore later with apply or pop.</p>"
                         "<h3>Reset</h3><p>Moves the branch pointer. Soft keeps everything, mixed unstages, hard discards work.</p>"
                         "<h3>Revert</h3><p>Undoes a commit with a <i>new</i> commit. Safe for shared history.</p>"
                         "<h3>Cherry-pick</h3><p>Copies one commit onto the current branch.</p>"
                         "<h3>Reflog</h3><p>A log of where HEAD pointed — the safety net for recovering from mistakes.</p>"));
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(view);
    }
};

} // namespace Guit

// Dev-only UI snapshot tool: renders every major page and dialog in
// Light and Dark themes offscreen and saves PNGs for visual review.
// Not part of the shipped application.
#include <app/AppSettings.h>
#include <app/ThemeManager.h>
#include <controllers/BranchController.h>
#include <controllers/ChangesController.h>
#include <controllers/HistoryController.h>
#include <controllers/MergeController.h>
#include <controllers/RemoteController.h>
#include <controllers/RepositoryInfoController.h>
#include <controllers/StashController.h>
#include <controllers/TagController.h>
#include <controllers/RepositoryController.h>
#include <git/GitRepository.h>
#include <ui/BranchDialog.h>
#include <ui/CloneDialog.h>
#include <ui/MainWindow.h>
#include <ui/MergeDialog.h>
#include <ui/SettingsDialog.h>
#include <ui/Sidebar.h>
#include <utils/Logger.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>

namespace
{

void runGit(const QString &dir, const QStringList &args)
{
    QProcess git;
    git.setProgram(QStringLiteral("git"));
    QStringList full = {QStringLiteral("-c"), QStringLiteral("user.name=Uisnap"),
                        QStringLiteral("-c"), QStringLiteral("user.email=uisnap@example.com"),
                        QStringLiteral("-c"), QStringLiteral("commit.gpgsign=false"),
                        QStringLiteral("-c"), QStringLiteral("core.autocrlf=false")};
    full.append(args);
    git.setArguments(full);
    git.setWorkingDirectory(dir);
    git.start();
    git.waitForStarted();
    git.waitForFinished(30000);
}

void writeFile(const QString &dir, const QString &name, const QString &content)
{
    QFile file(QDir(dir).filePath(name));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        file.write(content.toUtf8());
}

// A fixture with history, branches, tags, stash, and pending changes.
QString makeFixture()
{
    auto *dir = new QTemporaryDir();
    const QString path = dir->path();
    runGit(path, {QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")});
    runGit(path, {QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Uisnap")});
    runGit(path, {QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("uisnap@example.com")});
    writeFile(path, QStringLiteral("main.cpp"), QStringLiteral("int main() {}\n"));
    writeFile(path, QStringLiteral("README.md"), QStringLiteral("# demo\n"));
    runGit(path, {QStringLiteral("add"), QStringLiteral("-A")});
    runGit(path, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")});
    runGit(path, {QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature/login")});
    writeFile(path, QStringLiteral("login.cpp"), QStringLiteral("void login() {}\n"));
    runGit(path, {QStringLiteral("add"), QStringLiteral("-A")});
    runGit(path, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Add login form")});
    runGit(path, {QStringLiteral("checkout"), QStringLiteral("main")});
    writeFile(path, QStringLiteral("docs.txt"), QStringLiteral("docs\n"));
    runGit(path, {QStringLiteral("add"), QStringLiteral("-A")});
    runGit(path, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Update docs")});
    runGit(path, {QStringLiteral("tag"), QStringLiteral("-a"), QStringLiteral("-m"), QStringLiteral("Release"), QStringLiteral("v1.0")});
    runGit(path, {QStringLiteral("checkout"), QStringLiteral("feature/login")});
    writeFile(path, QStringLiteral("main.cpp"), QStringLiteral("int main() { return 0; }\n"));
    writeFile(path, QStringLiteral("scratch.txt"), QStringLiteral("wip\n"));
    runGit(path, {QStringLiteral("stash"), QStringLiteral("push"), QStringLiteral("-m"), QStringLiteral("wip work")});
    writeFile(path, QStringLiteral("main.cpp"), QStringLiteral("int main() { return 1; }\n"));
    writeFile(path, QStringLiteral("newfile.txt"), QStringLiteral("new\n"));
    runGit(path, {QStringLiteral("add"), QStringLiteral("newfile.txt")}); // staged entry for snapshots
    writeFile(path, QStringLiteral("lonely.txt"), QStringLiteral("untracked\n"));
    // Leak the temp dir for the process lifetime (snapshots are quick).
    (void)dir;
    return path;
}

void grab(QWidget *widget, const QString &path)
{
    widget->show();
    QApplication::processEvents();
    widget->grab().save(path);
}

void mark(const QString &stage)
{
    QFile log(QDir::temp().filePath(QStringLiteral("uisnap-stages.log")));
    if (log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        log.write((stage + QStringLiteral("\n")).toUtf8());
        log.flush();
    }
}

} // namespace

int main(int argc, char *argv[])
{
    // Redirect settings to a temp location: snapshots must never touch
    // the user's real configuration.
    QTemporaryDir settingsDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Guit"));
    QApplication::setApplicationName(QStringLiteral("GuitSnap"));

    // Offscreen has no font directory: load system fonts so snapshots
    // render real text instead of tofu boxes.
    QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeui.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeuib.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/arial.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/consola.ttf"));

    QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("uisnap");
    QDir().mkpath(outDir);

    const QString repoPath = makeFixture();
    mark(QStringLiteral("fixture-done"));
    {
        Guit::GitClient client;
        mark(client.hasGit() ? QStringLiteral("has-git:") + client.gitExecutable() : QStringLiteral("no-git"));
        const auto probeInfo = client.probeRepository(repoPath);
        mark(probeInfo.isRepository ? QStringLiteral("is-repo")
                                    : QStringLiteral("not-repo:") + probeInfo.raw.errorMessage);
        Guit::GitRepository probe;
        mark(probe.open(repoPath) ? QStringLiteral("probe-ok") : QStringLiteral("probe-fail"));
    }

    Guit::AppSettings settings;
    Guit::ThemeManager themes(&settings);
    Guit::GitRepository repository;
    Guit::RepositoryController controller(&repository, &settings);
    Guit::ChangesController changes(&repository);
    Guit::HistoryController history(&repository);
    Guit::BranchController branches(&repository);
    Guit::RemoteController remotes(&repository);
    Guit::TagController tags(&repository);
    Guit::StashController stashes(&repository);
    Guit::MergeController merge(&repository);
    Guit::RepositoryInfoController repoInfo(&repository);

    const QList<QPair<QString, Guit::ThemeManager::Theme>> themeList = {
        {QStringLiteral("light"), Guit::ThemeManager::Theme::Light},
        {QStringLiteral("dark"), Guit::ThemeManager::Theme::Dark},
    };
    for (const auto &entry : themeList) {
        themes.setTheme(entry.second);

        Guit::MainWindow window(&controller, &changes, &history, &branches, &remotes, &tags, &stashes, &merge,
                                &repoInfo, &settings, &themes);
        window.resize(1280, 800);
        controller.openRepository(repoPath);
        mark(QStringLiteral("opened-") + entry.first);
        window.show();
        QApplication::processEvents();

        const QList<QPair<QString, Guit::Sidebar::Page>> pages = {
            {QStringLiteral("overview"), Guit::Sidebar::Page::Overview},
            {QStringLiteral("changes"), Guit::Sidebar::Page::Changes},
            {QStringLiteral("history"), Guit::Sidebar::Page::History},
            {QStringLiteral("branches"), Guit::Sidebar::Page::Branches},
            {QStringLiteral("tags"), Guit::Sidebar::Page::Tags},
            {QStringLiteral("stashes"), Guit::Sidebar::Page::Stashes},
            {QStringLiteral("remotes"), Guit::Sidebar::Page::Remotes},
        };
        for (const auto &page : pages) {
            window.showPage(page.second);
            QApplication::processEvents();
            window.grab().save(QDir(outDir).filePath(QStringLiteral("%1-%2.png").arg(entry.first, page.first)));
            mark(entry.first + QStringLiteral("-") + page.first);
        }
        window.hide();
    }

    // Dialogs in dark mode.
    themes.setTheme(Guit::ThemeManager::Theme::Dark);
    Guit::BranchDialog branchDialog(&branches, Guit::BranchDialog::Mode::Create);
    grab(&branchDialog, QDir(outDir).filePath(QStringLiteral("dialog-branch.png")));
    branchDialog.hide();
    Guit::MergeDialog mergeDialog({QStringLiteral("main"), QStringLiteral("feature/login")}, QStringLiteral("feature/login"));
    grab(&mergeDialog, QDir(outDir).filePath(QStringLiteral("dialog-merge.png")));
    mergeDialog.hide();
    Guit::SettingsDialog settingsDialog(&settings, &themes);
    grab(&settingsDialog, QDir(outDir).filePath(QStringLiteral("dialog-settings.png")));

    return 0;
}

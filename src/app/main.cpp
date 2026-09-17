#include "../app/AppSettings.h"
#include "../app/ThemeManager.h"
#include "../controllers/BranchController.h"
#include "../controllers/ChangesController.h"
#include "../controllers/HistoryController.h"
#include "../controllers/RepositoryController.h"
#include "../git/GitRepository.h"
#include "../ui/MainWindow.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(guitMainLog, "guit.app.main")

int main(int argc, char *argv[])
{
    Guit::Logging::initialize(/*logToFile=*/true);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Guit"));
    QCoreApplication::setApplicationName(QStringLiteral("Guit"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    Guit::AppSettings settings;
    Guit::ThemeManager themes(&settings);

    // The Git executable override (if the user configured one) flows
    // Controller -> GitRepository -> GitClient -> GitProcess.
    Guit::GitRepository repository;
    repository.client()->setGitExecutableOverride(settings.gitExecutableOverride());

    if (!repository.client()->hasGit()) {
        qCWarning(guitMainLog) << "No Git executable found on PATH. The application will start in a degraded state.";
    } else {
        qCInfo(guitMainLog) << "Using git:" << repository.client()->gitExecutable()
                            << "version" << repository.client()->version().toString();
        const QString overridePath = settings.gitExecutableOverride();
        if (!overridePath.isEmpty() && repository.client()->gitExecutable() != overridePath)
            qCWarning(guitMainLog) << "Configured Git override is not usable:" << overridePath;
    }

    Guit::RepositoryController controller(&repository, &settings);
    Guit::ChangesController changes(&repository);
    Guit::HistoryController history(&repository);
    Guit::BranchController branches(&repository);
    Guit::MainWindow window(&controller, &changes, &history, &branches, &settings, &themes);
    window.show();

    // Optional: `guit <path>` opens a repository on startup.
    const QStringList positional = QCoreApplication::arguments().mid(1);
    if (!positional.isEmpty())
        controller.openRepository(positional.constFirst());

    return app.exec();
}

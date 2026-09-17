#pragma once

// Shared fixture: builds deterministic temporary Git repositories for
// tests. Tests must NEVER touch the developer's real repositories — every
// fixture lives in a QTemporaryDir that is destroyed with the test object.

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

namespace GuitTest
{

inline QString testGitExecutable()
{
    // GUIT_GIT_EXECUTABLE lets CI point at a specific git; otherwise PATH.
    const QString fromEnv = QProcessEnvironment::systemEnvironment().value(QStringLiteral("GUIT_GIT_EXECUTABLE"));
    if (!fromEnv.isEmpty())
        return fromEnv;
    return QStringLiteral("git");
}

class TempRepo
{
public:
    TempRepo()
    {
        QVERIFY2(dir.isValid(), "Failed to create temporary directory");
        runOrFail({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")});
    }

    [[nodiscard]] QString path() const { return dir.path(); }

    void runOrFail(const QStringList &args)
    {
        QProcess git;
        git.setProgram(testGitExecutable());
        // Deterministic identity, branch name, and no platform surprises.
        QStringList full = {QStringLiteral("-c"), QStringLiteral("user.name=Guit Test"),
                            QStringLiteral("-c"), QStringLiteral("user.email=guit-test@example.com"),
                            QStringLiteral("-c"), QStringLiteral("commit.gpgsign=false"),
                            QStringLiteral("-c"), QStringLiteral("core.autocrlf=false")};
        full.append(args);
        git.setArguments(full);
        git.setWorkingDirectory(path());
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        // Fixed timestamps keep commit hashes deterministic across runs.
        env.insert(QStringLiteral("GIT_AUTHOR_DATE"), QStringLiteral("2024-01-02T03:04:05+00:00"));
        env.insert(QStringLiteral("GIT_COMMITTER_DATE"), QStringLiteral("2024-01-02T03:04:05+00:00"));
        git.setProcessEnvironment(env);
        git.start();
        QVERIFY2(git.waitForStarted(), qPrintable(QStringLiteral("git failed to start: ") + git.errorString()));
        QVERIFY2(git.waitForFinished(30000), "git timed out in test fixture");
        QVERIFY2(git.exitCode() == 0,
                 qPrintable(QStringLiteral("git %1 failed: %2").arg(args.join(QLatin1Char(' ')),
                                                                   QString::fromUtf8(git.readAllStandardError()))));
    }

    void writeFile(const QString &relativePath, const QString &content)
    {
        QFile file(QDir(path()).filePath(relativePath));
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(file.fileName()));
        file.write(content.toUtf8());
        file.close();
    }

    void commit(const QString &message)
    {
        runOrFail({QStringLiteral("add"), QStringLiteral("-A")});
        runOrFail({QStringLiteral("commit"), QStringLiteral("-m"), message});
    }

private:
    QTemporaryDir dir;
};

} // namespace GuitTest

#pragma once

// Shared fixture: builds deterministic temporary Git repositories for
// tests. Tests must NEVER touch the developer's real repositories — every
// fixture lives in a QTemporaryDir that is destroyed with the test object.

#include <QDir>
#include <QFile>
#include <QPair>
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
    TempRepo(bool bare = false)
    {
        QVERIFY2(dir.isValid(), "Failed to create temporary directory");
        if (bare) {
            runOrFail({QStringLiteral("init"), QStringLiteral("--bare"), QStringLiteral("-b"), QStringLiteral("main")});
            return;
        }
        runOrFail({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")});
        // Repo-local identity: Guit itself never invents an author, but the
        // test suite needs commits to succeed on machines without Git
        // user configuration.
        runOrFail({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Guit Test")});
        runOrFail({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("guit-test@example.com")});
    }

    [[nodiscard]] QString path() const { return dir.path(); }

    // Runs git, returning the exit code. Use for operations that are
    // *expected* to fail (merge conflicts, ...).
    int runRaw(const QStringList &args)
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
        if (!git.waitForStarted())
            return -100;
        if (!git.waitForFinished(30000))
            return -101;
        return git.exitCode();
    }

    void runOrFail(const QStringList &args)
    {
        const int exitCode = runRaw(args);
        QVERIFY2(exitCode == 0, qPrintable(QStringLiteral("git %1 exited with %2").arg(args.join(QLatin1Char(' '))).arg(exitCode)));
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

    // Runs git in an arbitrary directory (e.g. inspecting a bare remote).
    // Returns {exitCode, stdout}.
    static QPair<int, QString> runIn(const QString &directory, const QStringList &args)
    {
        QProcess git;
        git.setProgram(testGitExecutable());
        git.setArguments(args);
        git.setWorkingDirectory(directory);
        git.start();
        if (!git.waitForStarted())
            return {-100, {}};
        if (!git.waitForFinished(30000))
            return {-101, {}};
        return {git.exitCode(), QString::fromUtf8(git.readAllStandardOutput())};
    }

private:
    QTemporaryDir dir;
};

} // namespace GuitTest

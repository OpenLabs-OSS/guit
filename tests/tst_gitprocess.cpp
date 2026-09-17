// GitProcess: real process execution, exit codes, stdout/stderr capture,
// start failures, and timeouts. Uses the real git executable.

#include <git/GitProcess.h>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace Guit;

class TestGitProcess : public QObject
{
    Q_OBJECT

private slots:
    void runsGitVersionSuccessfully()
    {
        const GitProcessResult result = GitProcess::run(QStringLiteral("git"), {QStringLiteral("--version")});
        QVERIFY(result.started);
        QCOMPARE(result.error, GitError::None);
        QCOMPARE(result.exitCode, 0);
        QVERIFY(result.isSuccess());
        QVERIFY(result.standardOutput.contains(QStringLiteral("git version")));
    }

    void capturesNonZeroExitCode()
    {
        QTemporaryDir plainDir;
        QVERIFY(plainDir.isValid());
        // rev-parse inside a non-repository fails with a useful stderr.
        const GitProcessResult result = GitProcess::run(
            QStringLiteral("git"), {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}, plainDir.path());
        QVERIFY(result.started);
        QVERIFY(!result.isSuccess());
        QCOMPARE(result.error, GitError::CommandFailed);
        QVERIFY(result.exitCode != 0);
        QVERIFY(!result.standardError.isEmpty());
    }

    void reportsMissingExecutable()
    {
        const GitProcessResult result = GitProcess::run(QStringLiteral(""), {QStringLiteral("--version")});
        QVERIFY(!result.started);
        QCOMPARE(result.error, GitError::GitNotFound);
        QVERIFY(!result.isSuccess());
    }

    void reportsUnstartableProgram()
    {
        const GitProcessResult result = GitProcess::run(
            QStringLiteral("/nonexistent-directory-xyz/guit-no-such-git"), {QStringLiteral("--version")});
        QVERIFY(!result.started);
        QCOMPARE(result.error, GitError::ProcessStartFailed);
        QVERIFY(!result.errorMessage.isEmpty());
    }

    void rejectsInvalidTimeout()
    {
        const GitProcessResult result =
            GitProcess::run(QStringLiteral("git"), {QStringLiteral("--version")}, {}, 0);
        QCOMPARE(result.error, GitError::InvalidInput);
    }

    void runsInWorkingDirectory()
    {
        QTemporaryDir plainDir;
        QVERIFY(plainDir.isValid());
        const GitProcessResult result =
            GitProcess::run(QStringLiteral("git"), {QStringLiteral("rev-parse"), QStringLiteral("--git-dir")}, plainDir.path());
        QVERIFY(result.started);
        // Outside a repository this fails — the point is that the working
        // directory was honored and the process ran (started == true).
        QVERIFY(!result.isSuccess());
    }
};

QTEST_MAIN(TestGitProcess)
#include "tst_gitprocess.moc"

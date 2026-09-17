// GitCommandFormatter: display quoting for the equivalent-command UI.

#include <utils/GitCommandFormatter.h>

#include <QtTest>

using namespace Guit;

class TestCommandFormatter : public QObject
{
    Q_OBJECT

private slots:
    void leavesSimpleArgumentsUnquoted()
    {
        QCOMPARE(GitCommandFormatter::format({QStringLiteral("status"), QStringLiteral("--porcelain=v1")}),
                 QStringLiteral("git status --porcelain=v1"));
    }

    void quotesArgumentsWithSpaces()
    {
        QCOMPARE(GitCommandFormatter::format({QStringLiteral("add"), QStringLiteral("--"), QStringLiteral("My File.txt")}),
                 QStringLiteral("git add -- \"My File.txt\""));
        QCOMPARE(GitCommandFormatter::format({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}),
                 QStringLiteral("git commit -m \"Initial commit\""));
    }

    void escapesEmbeddedQuotes()
    {
        QCOMPARE(GitCommandFormatter::quoteArgument(QStringLiteral("say \"hi\"")),
                 QStringLiteral("\"say \\\"hi\\\"\""));
    }

    void quotesEmptyArgument()
    {
        QCOMPARE(GitCommandFormatter::quoteArgument({}), QStringLiteral("\"\""));
    }

    void switchBranchExample()
    {
        // The documented example from AGENTS.md must render exactly.
        QCOMPARE(GitCommandFormatter::format({QStringLiteral("switch"), QStringLiteral("-c"), QStringLiteral("feature/login")}),
                 QStringLiteral("git switch -c feature/login"));
    }
};

QTEST_MAIN(TestCommandFormatter)
#include "tst_commandformatter.moc"

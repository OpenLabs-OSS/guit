#include "AppSettings.h"

#include <QDir>

namespace Guit
{

namespace
{
constexpr char kThemeKey[] = "ui/theme";
constexpr char kGitExecutableKey[] = "git/executable";
constexpr char kRecentKey[] = "repository/recent";
constexpr char kGeometryKey[] = "ui/mainWindowGeometry";
} // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
}

QString AppSettings::theme() const
{
    return m_settings.value(QString::fromLatin1(kThemeKey), QStringLiteral("system")).toString();
}

void AppSettings::setTheme(const QString &theme)
{
    m_settings.setValue(QString::fromLatin1(kThemeKey), theme);
}

QString AppSettings::gitExecutableOverride() const
{
    return m_settings.value(QString::fromLatin1(kGitExecutableKey)).toString();
}

void AppSettings::setGitExecutableOverride(const QString &path)
{
    if (path.isEmpty())
        m_settings.remove(QString::fromLatin1(kGitExecutableKey));
    else
        m_settings.setValue(QString::fromLatin1(kGitExecutableKey), path);
}

QStringList AppSettings::recentRepositories() const
{
    return m_settings.value(QString::fromLatin1(kRecentKey)).toStringList();
}

void AppSettings::addRecentRepository(const QString &path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned.isEmpty())
        return;
    QStringList recents = recentRepositories();
    recents.removeAll(cleaned);
    recents.prepend(cleaned);
    while (recents.size() > maxRecentRepositories)
        recents.removeLast();
    m_settings.setValue(QString::fromLatin1(kRecentKey), recents);
}

void AppSettings::removeRecentRepository(const QString &path)
{
    QStringList recents = recentRepositories();
    if (recents.removeAll(QDir::cleanPath(path)) > 0)
        m_settings.setValue(QString::fromLatin1(kRecentKey), recents);
}

void AppSettings::clearRecentRepositories()
{
    m_settings.remove(QString::fromLatin1(kRecentKey));
}

QByteArray AppSettings::windowGeometry() const
{
    return m_settings.value(QString::fromLatin1(kGeometryKey)).toByteArray();
}

void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    m_settings.setValue(QString::fromLatin1(kGeometryKey), geometry);
}

} // namespace Guit

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace Guit
{

// Qt persistence for Guit settings (QSettings: registry on Windows,
// plist on macOS, INI on Linux). Organization/app names are set in main().
class AppSettings : public QObject
{
    Q_OBJECT

public:
    static constexpr int maxRecentRepositories = 10;

    explicit AppSettings(QObject *parent = nullptr);

    QString theme() const;
    void setTheme(const QString &theme);

    QString gitExecutableOverride() const;
    void setGitExecutableOverride(const QString &path);

    QStringList recentRepositories() const;
    void addRecentRepository(const QString &path);
    void removeRecentRepository(const QString &path);
    void clearRecentRepositories();

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

private:
    QSettings m_settings;
};

} // namespace Guit

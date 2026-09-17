#pragma once

#include <QString>

namespace Guit
{

// Parsed form of `git --version` output, e.g. "git version 2.43.0.windows.1".
struct GitVersion
{
    int major = -1;
    int minor = -1;
    int patch = -1;
    QString raw;

    [[nodiscard]] bool isValid() const { return major >= 0 && minor >= 0; }
    [[nodiscard]] QString toString() const;
    static GitVersion parse(const QString &versionOutput);

    bool operator==(const GitVersion &other) const = default;
};

} // namespace Guit

#include "config/OutputPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace output {
namespace {

QString normalizedExtension(const QString &extension)
{
    QString result = extension.trimmed();
    while (result.startsWith(QLatin1Char('.')))
        result.remove(0, 1);
    return result;
}

} // namespace

QString picturesDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/Pictures");
    return base + QStringLiteral("/Camera");
}

QString videosDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/Videos");
    return base + QStringLiteral("/Camera");
}

QString buildMediaName(const QString &prefix, const QDateTime &when, const QString &extension, int collisionIndex)
{
    QString name = prefix + QLatin1Char('_')
        + when.toString(QStringLiteral("yyyyMMdd_HHmmss"));
    if (collisionIndex > 0)
        name += QStringLiteral("_%1").arg(collisionIndex);
    const QString ext = normalizedExtension(extension);
    if (!ext.isEmpty())
        name += QLatin1Char('.') + ext;
    return name;
}

QString uniqueMediaPath(const QString &dir, const QString &prefix, const QString &extension,
                        const QDateTime &when, const std::function<bool(const QString &)> &exists)
{
    for (int index = 0; index < 1000; ++index) {
        const QString candidate = QDir(dir).filePath(buildMediaName(prefix, when, extension, index));
        if (!exists(candidate))
            return candidate;
    }
    return QDir(dir).filePath(buildMediaName(prefix, when, extension, 1000));
}

QString partPathFor(const QString &finalPath)
{
    return finalPath + QStringLiteral(".part");
}

QString finalPathFor(const QString &partPath)
{
    if (partPath.endsWith(QStringLiteral(".part")))
        return partPath.chopped(5);
    return partPath;
}

bool isPartPath(const QString &path)
{
    return path.endsWith(QStringLiteral(".part"));
}

bool ensureDir(const QString &path, QString *error)
{
    if (QDir().mkpath(path))
        return true;
    if (error)
        *error = QStringLiteral("Could not create %1").arg(path);
    return false;
}

} // namespace output

#pragma once

#include <QDateTime>
#include <QString>

#include <functional>

namespace output {

QString picturesDir();
QString videosDir();

QString buildMediaName(const QString &prefix, const QDateTime &when, const QString &extension, int collisionIndex = 0);
QString uniqueMediaPath(const QString &dir, const QString &prefix, const QString &extension,
                        const QDateTime &when, const std::function<bool(const QString &)> &exists);
QString partPathFor(const QString &finalPath);
QString finalPathFor(const QString &partPath);
bool isPartPath(const QString &path);
bool pathTaken(const QString &path);
QString sanitizedPath(const QString &path);
bool ensureDir(const QString &path, QString *error = nullptr);

} // namespace output

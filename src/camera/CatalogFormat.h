#pragma once

#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>

namespace catalog {

struct Entry
{
    int width = 0;
    int height = 0;
    double minFps = 0.0;
    double maxFps = 0.0;
    int pixelFormat = -1;
    QString pixelLabel;
};

QList<Entry> normalize(const QList<Entry> &raw);
QList<QSize> resolutions(const QList<Entry> &entries);
QStringList pixelLabels(const QList<Entry> &entries, const QSize &resolution);
QList<double> frameRates(const QList<Entry> &entries, const QSize &resolution, const QString &pixelLabel);
QSize nearestResolution(const QList<Entry> &entries, const QSize &desired);
QString nearestPixelLabel(const QList<Entry> &entries, const QSize &resolution, const QString &desired);
double nearestFrameRate(const QList<Entry> &entries, const QSize &resolution, const QString &pixelLabel, double desired);
QString formatLabel(const QSize &resolution, const QString &pixelLabel, double fps);
QString resolutionLabel(const QSize &resolution);

} // namespace catalog

#include "camera/CatalogFormat.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace catalog {
namespace {

const QList<double> &standardRates()
{
    static const QList<double> rates = {5, 10, 15, 20, 24, 25, 30, 50, 60};
    return rates;
}

bool sameResolution(const QSize &a, const QSize &b)
{
    return a.width() == b.width() && a.height() == b.height();
}

bool sameRate(double a, double b)
{
    return std::abs(a - b) < 0.01;
}

QString fpsText(double fps)
{
    if (sameRate(fps, std::round(fps)))
        return QString::number(static_cast<qint64>(std::llround(fps)));
    return QString::number(fps, 'g', 3);
}

} // namespace

QList<Entry> normalize(const QList<Entry> &raw)
{
    QList<Entry> result;
    for (const Entry &entry : raw) {
        if (entry.width <= 0 || entry.height <= 0)
            continue;
        bool duplicate = false;
        for (const Entry &existing : result) {
            if (existing.width == entry.width && existing.height == entry.height
                && existing.pixelFormat == entry.pixelFormat
                && sameRate(existing.minFps, entry.minFps) && sameRate(existing.maxFps, entry.maxFps)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
            result.append(entry);
    }

    std::sort(result.begin(), result.end(), [](const Entry &a, const Entry &b) {
        const qint64 aArea = static_cast<qint64>(a.width) * a.height;
        const qint64 bArea = static_cast<qint64>(b.width) * b.height;
        if (aArea != bArea)
            return aArea > bArea;
        if (a.width != b.width)
            return a.width > b.width;
        if (a.height != b.height)
            return a.height > b.height;
        if (a.pixelLabel != b.pixelLabel)
            return a.pixelLabel < b.pixelLabel;
        return a.maxFps > b.maxFps;
    });
    return result;
}

QList<QSize> resolutions(const QList<Entry> &entries)
{
    QList<QSize> result;
    for (const Entry &entry : entries) {
        const QSize size(entry.width, entry.height);
        bool seen = false;
        for (const QSize &existing : result) {
            if (sameResolution(existing, size)) {
                seen = true;
                break;
            }
        }
        if (!seen)
            result.append(size);
    }
    return result;
}

QStringList pixelLabels(const QList<Entry> &entries, const QSize &resolution)
{
    QStringList result;
    for (const Entry &entry : entries) {
        if (!sameResolution(QSize(entry.width, entry.height), resolution))
            continue;
        if (!result.contains(entry.pixelLabel))
            result.append(entry.pixelLabel);
    }
    if (result.isEmpty()) {
        for (const Entry &entry : entries) {
            if (!result.contains(entry.pixelLabel))
                result.append(entry.pixelLabel);
        }
    }
    return result;
}

QList<double> frameRates(const QList<Entry> &entries, const QSize &resolution, const QString &pixelLabel)
{
    QList<double> result;
    const auto add = [&result](double fps) {
        for (double existing : result) {
            if (sameRate(existing, fps))
                return;
        }
        result.append(fps);
    };

    for (const Entry &entry : entries) {
        if (!sameResolution(QSize(entry.width, entry.height), resolution))
            continue;
        if (!pixelLabel.isEmpty() && entry.pixelLabel != pixelLabel)
            continue;
        const double low = std::min(entry.minFps, entry.maxFps);
        const double high = std::max(entry.minFps, entry.maxFps);
        if (low <= 0.0 && high <= 0.0)
            continue;
        add(high);
        for (double rate : standardRates()) {
            if (rate >= low - 0.01 && rate <= high + 0.01)
                add(rate);
        }
        add(low);
    }

    std::sort(result.begin(), result.end(), std::greater<double>());
    return result;
}

QSize nearestResolution(const QList<Entry> &entries, const QSize &desired)
{
    const QList<QSize> options = resolutions(entries);
    if (options.isEmpty())
        return desired;

    for (const QSize &size : options) {
        if (sameResolution(size, desired))
            return size;
    }

    if (!desired.isValid() || desired.isEmpty())
        return options.first();

    QSize best = options.first();
    qint64 bestDistance = std::llabs(static_cast<qint64>(best.width()) * best.height()
                                     - static_cast<qint64>(desired.width()) * desired.height());
    for (int i = 1; i < options.size(); ++i) {
        const QSize &candidate = options.at(i);
        const qint64 distance = std::llabs(static_cast<qint64>(candidate.width()) * candidate.height()
                                           - static_cast<qint64>(desired.width()) * desired.height());
        if (distance < bestDistance) {
            best = candidate;
            bestDistance = distance;
        }
    }
    return best;
}

QString nearestPixelLabel(const QList<Entry> &entries, const QSize &resolution, const QString &desired)
{
    const QStringList labels = pixelLabels(entries, resolution);
    if (labels.isEmpty())
        return desired;
    if (labels.contains(desired))
        return desired;
    return labels.first();
}

double nearestFrameRate(const QList<Entry> &entries, const QSize &resolution, const QString &pixelLabel, double desired)
{
    const QList<double> rates = frameRates(entries, resolution, pixelLabel);
    if (rates.isEmpty())
        return desired;
    if (desired <= 0.0)
        return rates.first();

    double best = rates.first();
    double bestDistance = std::abs(best - desired);
    for (double rate : rates) {
        const double distance = std::abs(rate - desired);
        if (distance < bestDistance) {
            best = rate;
            bestDistance = distance;
        }
    }
    return best;
}

QString formatLabel(const QSize &resolution, const QString &pixelLabel, double fps)
{
    if (!resolution.isValid())
        return QString();
    QString label = resolutionLabel(resolution);
    if (!pixelLabel.isEmpty())
        label += QStringLiteral(" · ") + pixelLabel;
    if (fps > 0.0)
        label += QStringLiteral(" · ") + fpsText(fps) + QStringLiteral(" fps");
    return label;
}

QString resolutionLabel(const QSize &resolution)
{
    return QStringLiteral("%1x%2").arg(resolution.width()).arg(resolution.height());
}

} // namespace catalog

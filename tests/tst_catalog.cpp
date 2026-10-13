#include <QtTest>

#include "camera/CatalogFormat.h"

class TestCatalog : public QObject
{
    Q_OBJECT

private slots:
    void normalizeDeduplicatesAndSorts();
    void resolutionsAreUniqueAndOrdered();
    void pixelLabelsFollowResolution();
    void frameRatesIncludeStandardRatesWithinRange();
    void nearestResolutionPrefersExactThenClosestArea();
    void nearestFrameRateClampsToAvailable();
    void formatLabelIsHumanReadable();
};

static QList<catalog::Entry> sample()
{
    return {
        {1920, 1080, 30.0, 30.0, 29, QStringLiteral("MJPEG")},
        {1920, 1080, 5.0, 5.0, 17, QStringLiteral("YUYV")},
        {1280, 720, 10.0, 10.0, 17, QStringLiteral("YUYV")},
        {640, 480, 30.0, 30.0, 17, QStringLiteral("YUYV")},
        {640, 480, 30.0, 30.0, 29, QStringLiteral("MJPEG")},
        {640, 480, 30.0, 30.0, 29, QStringLiteral("MJPEG")},
        {320, 240, 5.0, 30.0, 29, QStringLiteral("MJPEG")},
    };
}

void TestCatalog::normalizeDeduplicatesAndSorts()
{
    const QList<catalog::Entry> entries = catalog::normalize(sample());
    QCOMPARE(entries.size(), 6);
    QCOMPARE(entries.first().width, 1920);
    QCOMPARE(entries.first().height, 1080);
    QCOMPARE(entries.last().width, 320);
    QCOMPARE(entries.last().height, 240);
}

void TestCatalog::resolutionsAreUniqueAndOrdered()
{
    const QList<QSize> sizes = catalog::resolutions(catalog::normalize(sample()));
    QCOMPARE(sizes.size(), 4);
    QCOMPARE(sizes.at(0), QSize(1920, 1080));
    QCOMPARE(sizes.at(1), QSize(1280, 720));
    QCOMPARE(sizes.at(2), QSize(640, 480));
    QCOMPARE(sizes.at(3), QSize(320, 240));
}

void TestCatalog::pixelLabelsFollowResolution()
{
    const QList<catalog::Entry> entries = catalog::normalize(sample());
    const QStringList full = catalog::pixelLabels(entries, QSize(1920, 1080));
    QCOMPARE(full, QStringList({QStringLiteral("MJPEG"), QStringLiteral("YUYV")}));
    const QStringList small = catalog::pixelLabels(entries, QSize(640, 480));
    QCOMPARE(small, QStringList({QStringLiteral("MJPEG"), QStringLiteral("YUYV")}));
    const QStringList missing = catalog::pixelLabels(entries, QSize(1024, 768));
    QCOMPARE(missing.size(), 2);
}

void TestCatalog::frameRatesIncludeStandardRatesWithinRange()
{
    const QList<catalog::Entry> entries = catalog::normalize(sample());
    const QList<double> rates = catalog::frameRates(entries, QSize(320, 240), QStringLiteral("MJPEG"));
    QCOMPARE(rates.first(), 30.0);
    QVERIFY(rates.contains(5.0));
    QVERIFY(rates.contains(24.0));
    QVERIFY(!rates.contains(60.0));

    const QList<double> fixed = catalog::frameRates(entries, QSize(1920, 1080), QStringLiteral("MJPEG"));
    QCOMPARE(fixed, QList<double>({30.0}));
}

void TestCatalog::nearestResolutionPrefersExactThenClosestArea()
{
    const QList<catalog::Entry> entries = catalog::normalize(sample());
    QCOMPARE(catalog::nearestResolution(entries, QSize(1280, 720)), QSize(1280, 720));
    QCOMPARE(catalog::nearestResolution(entries, QSize(1280, 800)), QSize(1280, 720));
    QCOMPARE(catalog::nearestResolution(entries, QSize(800, 600)), QSize(640, 480));
    QCOMPARE(catalog::nearestResolution(entries, QSize()), QSize(1920, 1080));
}

void TestCatalog::nearestFrameRateClampsToAvailable()
{
    const QList<catalog::Entry> entries = catalog::normalize(sample());
    QCOMPARE(catalog::nearestFrameRate(entries, QSize(320, 240), QStringLiteral("MJPEG"), 23.0), 24.0);
    QCOMPARE(catalog::nearestFrameRate(entries, QSize(320, 240), QStringLiteral("MJPEG"), 100.0), 30.0);
    QCOMPARE(catalog::nearestFrameRate(entries, QSize(1920, 1080), QStringLiteral("YUYV"), 0.0), 5.0);
    QCOMPARE(catalog::nearestFrameRate(entries, QSize(1920, 1080), QStringLiteral("MJPEG"), 12.0), 30.0);
}

void TestCatalog::formatLabelIsHumanReadable()
{
    QCOMPARE(catalog::formatLabel(QSize(640, 480), QStringLiteral("MJPEG"), 30.0),
             QStringLiteral("640x480 · MJPEG · 30 fps"));
    QCOMPARE(catalog::formatLabel(QSize(640, 480), QString(), 0.0), QStringLiteral("640x480"));
    QCOMPARE(catalog::resolutionLabel(QSize(1920, 1080)), QStringLiteral("1920x1080"));
}

QTEST_APPLESS_MAIN(TestCatalog)

#include "tst_catalog.moc"

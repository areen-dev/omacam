#include <QtTest>

#include "config/OutputPaths.h"

#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>

class TestOutputPaths : public QObject
{
    Q_OBJECT

private slots:
    void buildsTimestampedNames();
    void resolvesCollisions();
    void handlesPartSuffixes();
    void ensuresDirectories();
};

void TestOutputPaths::buildsTimestampedNames()
{
    const QDateTime when(QDate(2026, 9, 19), QTime(23, 5, 7));
    QCOMPARE(output::buildMediaName(QStringLiteral("IMG"), when, QStringLiteral("jpg")),
             QStringLiteral("IMG_20260919_230507.jpg"));
    QCOMPARE(output::buildMediaName(QStringLiteral("VID"), when, QStringLiteral(".mp4"), 2),
             QStringLiteral("VID_20260919_230507_2.mp4"));
}

void TestOutputPaths::resolvesCollisions()
{
    QSet<QString> taken;
    taken.insert(QStringLiteral("/tmp/cam/IMG_20260919_230507.jpg"));
    taken.insert(QStringLiteral("/tmp/cam/IMG_20260919_230507_1.jpg"));

    const QString path = output::uniqueMediaPath(QStringLiteral("/tmp/cam"), QStringLiteral("IMG"),
                                                 QStringLiteral("jpg"),
                                                 QDateTime(QDate(2026, 9, 19), QTime(23, 5, 7)),
                                                 [&taken](const QString &candidate) {
                                                     return taken.contains(candidate);
                                                 });
    QCOMPARE(path, QStringLiteral("/tmp/cam/IMG_20260919_230507_2.jpg"));
}

void TestOutputPaths::handlesPartSuffixes()
{
    const QString finalPath = QStringLiteral("/tmp/cam/VID_20260919_230507.mp4");
    const QString partPath = output::partPathFor(finalPath);
    QCOMPARE(partPath, QStringLiteral("/tmp/cam/VID_20260919_230507.mp4.part"));
    QVERIFY(output::isPartPath(partPath));
    QVERIFY(!output::isPartPath(finalPath));
    QCOMPARE(output::finalPathFor(partPath), finalPath);
    QCOMPARE(output::finalPathFor(finalPath), finalPath);
}

void TestOutputPaths::ensuresDirectories()
{
    QTemporaryDir dir;
    const QString nested = dir.filePath(QStringLiteral("with space/Camera"));
    QString error;
    QVERIFY2(output::ensureDir(nested, &error), qPrintable(error));
    QVERIFY(QDir(nested).exists());
}

QTEST_APPLESS_MAIN(TestOutputPaths)

#include "tst_output_paths.moc"

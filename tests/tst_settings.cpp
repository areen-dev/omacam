#include <QtTest>

#include "config/SettingsStore.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void parsesTypedValues();
    void rejectsMalformedLines();
    void roundTripsValues();
    void defaultsWhenFileMissing();
    void savesAndReloadsAtomically();
    void preservesCorruptFile();
    void newerSchemaIsReadOnly();
    void migratesLegacySettingsFile();
private:
    QTemporaryDir m_configHome;
};

void TestSettings::initTestCase()
{
    QVERIFY(m_configHome.isValid());
    qputenv("XDG_CONFIG_HOME", m_configHome.path().toUtf8());
}

void TestSettings::parsesTypedValues()
{
    const QString text = QStringLiteral(
        "# comment\n"
        "resolution=1920x1080\n"
        "frame_rate=30\n"
        "mirror=true\n"
        "photos_dir=\"/home/user/My Pictures/Camera\"\n"
        "quote=\"say \\\"hi\\\"\"\n"
        "\n"
        "unknown=value\n");

    bool ok = false;
    const QMap<QString, QVariant> values = SettingsStore::parseIni(text, &ok);
    QVERIFY(ok);
    QCOMPARE(values.value(QStringLiteral("resolution")).toString(), QStringLiteral("1920x1080"));
    QCOMPARE(values.value(QStringLiteral("frame_rate")).toInt(), 30);
    QCOMPARE(values.value(QStringLiteral("mirror")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("photos_dir")).toString(), QStringLiteral("/home/user/My Pictures/Camera"));
    QCOMPARE(values.value(QStringLiteral("quote")).toString(), QStringLiteral("say \"hi\""));
    QCOMPARE(values.value(QStringLiteral("unknown")).toString(), QStringLiteral("value"));
}

void TestSettings::rejectsMalformedLines()
{
    bool ok = false;
    SettingsStore::parseIni(QStringLiteral("resolution=640x480\nthis line has no separator\n"), &ok);
    QVERIFY(!ok);
}

void TestSettings::roundTripsValues()
{
    QMap<QString, QVariant> values;
    values.insert(QStringLiteral("resolution"), QStringLiteral("1280x720"));
    values.insert(QStringLiteral("frame_rate"), 30.0);
    values.insert(QStringLiteral("mirror"), false);
    values.insert(QStringLiteral("photos_dir"), QStringLiteral("/tmp/with space/Camera"));

    const QString text = SettingsStore::serializeIni(values);
    bool ok = false;
    const QMap<QString, QVariant> parsed = SettingsStore::parseIni(text, &ok);
    QVERIFY(ok);
    QCOMPARE(parsed, values);
}

void TestSettings::defaultsWhenFileMissing()
{
    QTemporaryDir dir;
    SettingsStore store(dir.filePath(QStringLiteral("camera.conf")));
    QCOMPARE(store.load(), SettingsStore::LoadStatus::Defaults);
    QCOMPARE(store.value(QStringLiteral("mirror"), true).toBool(), true);
}

void TestSettings::savesAndReloadsAtomically()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("nested/camera.conf"));

    SettingsStore store(path);
    store.load();
    store.setValue(QStringLiteral("resolution"), QStringLiteral("640x480"));
    store.setValue(QStringLiteral("frame_rate"), 30.0);
    QString error;
    QVERIFY2(store.save(&error), qPrintable(error));
    QVERIFY(QFile::exists(path));

    SettingsStore reloaded(path);
    QCOMPARE(reloaded.load(), SettingsStore::LoadStatus::Loaded);
    QCOMPARE(reloaded.value(QStringLiteral("resolution")).toString(), QStringLiteral("640x480"));
    QCOMPARE(reloaded.value(QStringLiteral("frame_rate")).toDouble(), 30.0);
    QCOMPARE(reloaded.value(QStringLiteral("schema_version")).toInt(), SettingsStore::kSchemaVersion);
}

void TestSettings::preservesCorruptFile()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("camera.conf"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not an ini file\n");
    file.close();

    SettingsStore store(path);
    QCOMPARE(store.load(), SettingsStore::LoadStatus::Corrupt);
    QVERIFY(store.statusMessage().contains(QStringLiteral("corrupt")));

    const QStringList entries = QDir(dir.path()).entryList(QStringList(QStringLiteral("camera.conf.corrupt-*")), QDir::Files);
    QCOMPARE(entries.size(), 1);
}

void TestSettings::newerSchemaIsReadOnly()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("camera.conf"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("schema_version=99\nresolution=640x480\n");
    file.close();

    SettingsStore store(path);
    QCOMPARE(store.load(), SettingsStore::LoadStatus::NewerSchema);
    QVERIFY(store.readOnly());
    QCOMPARE(store.value(QStringLiteral("resolution")).toString(), QStringLiteral("640x480"));

    QString error;
    QVERIFY(!store.save(&error));
    QVERIFY(!error.isEmpty());

    QFile check(path);
    QVERIFY(check.open(QIODevice::ReadOnly));
    QVERIFY(check.readAll().contains("schema_version=99"));
}

void TestSettings::migratesLegacySettingsFile()
{
    QTemporaryDir dir;
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());

    const QString legacyDir = dir.filePath(QStringLiteral("dev.areen.camera"));
    QVERIFY(QDir().mkpath(legacyDir));
    const QString legacyPath = legacyDir + QStringLiteral("/camera.conf");
    QFile legacy(legacyPath);
    QVERIFY(legacy.open(QIODevice::WriteOnly));
    legacy.write("schema_version=1\nphotos_dir=\"/tmp/legacy-photos\"\nself_timer_seconds=3\n");
    legacy.close();

    const QString newPath = dir.filePath(QStringLiteral("dev.areen.omacam/camera.conf"));
    SettingsStore store(newPath);
    QCOMPARE(store.load(), SettingsStore::LoadStatus::Loaded);
    QCOMPARE(store.value(QStringLiteral("photos_dir")).toString(), QStringLiteral("/tmp/legacy-photos"));
    QCOMPARE(store.value(QStringLiteral("self_timer_seconds")).toInt(), 3);

    QString error;
    QVERIFY2(store.save(&error), qPrintable(error));
    QVERIFY(QFile::exists(newPath));
    QVERIFY(QFile::exists(legacyPath));

    SettingsStore reloaded(newPath);
    QCOMPARE(reloaded.load(), SettingsStore::LoadStatus::Loaded);
    QCOMPARE(reloaded.value(QStringLiteral("photos_dir")).toString(), QStringLiteral("/tmp/legacy-photos"));
}

QTEST_APPLESS_MAIN(TestSettings)
#include "tst_settings.moc"

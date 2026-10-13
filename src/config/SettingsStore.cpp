#include "config/SettingsStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

QString quoteString(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

QString unquoteString(const QString &value, bool *ok)
{
    if (value.size() < 2 || !value.startsWith(QLatin1Char('"')) || !value.endsWith(QLatin1Char('"'))) {
        *ok = false;
        return value;
    }
    QString result;
    result.reserve(value.size() - 2);
    bool escaped = false;
    for (int i = 1; i < value.size() - 1; ++i) {
        const QChar ch = value.at(i);
        if (escaped) {
            if (ch == QLatin1Char('n'))
                result.append(QLatin1Char('\n'));
            else
                result.append(ch);
            escaped = false;
        } else if (ch == QLatin1Char('\\')) {
            escaped = true;
        } else {
            result.append(ch);
        }
    }
    *ok = !escaped;
    return result;
}

} // namespace

SettingsStore::SettingsStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

QString SettingsStore::defaultFilePath()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty())
        return QDir::homePath() + QStringLiteral("/.config/dev.areen.omacam/camera.conf");
    return configDir + QStringLiteral("/camera.conf");
}

QString SettingsStore::legacyFilePath()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (configDir.isEmpty())
        configDir = QDir::homePath() + QStringLiteral("/.config");
    return configDir + QStringLiteral("/dev.areen.camera/camera.conf");
}

SettingsStore::LoadStatus SettingsStore::load()
{
    m_values.clear();
    m_readOnly = false;
    m_statusMessage.clear();

    QString sourcePath = m_filePath;
    if (!QFile::exists(m_filePath)) {
        const QString legacy = legacyFilePath();
        if (m_filePath != legacy && QFile::exists(legacy)) {
            sourcePath = legacy;
        } else {
            m_status = LoadStatus::Defaults;
            return m_status;
        }
    }

    QFile file(sourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_status = LoadStatus::Unreadable;
        m_statusMessage = QStringLiteral("Could not read %1: %2").arg(sourcePath, file.errorString());
        return m_status;
    }

    bool ok = false;
    const QMap<QString, QVariant> parsed = parseIni(QString::fromUtf8(file.readAll()), &ok);
    if (!ok) {
        const QString backup = sourcePath + QStringLiteral(".corrupt-")
            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
        QFile::copy(sourcePath, backup);
        m_status = LoadStatus::Corrupt;
        m_statusMessage = QStringLiteral("Settings file was malformed; original preserved as %1").arg(backup);
        return m_status;
    }

    m_values = parsed;
    const int version = m_values.value(QStringLiteral("schema_version")).toInt(0);
    if (version > kSchemaVersion) {
        m_readOnly = true;
        m_status = LoadStatus::NewerSchema;
        m_statusMessage = QStringLiteral("Settings were written by a newer version; keeping them read-only");
        return m_status;
    }

    m_status = LoadStatus::Loaded;
    return m_status;
}

QVariant SettingsStore::value(const QString &key, const QVariant &fallback) const
{
    return m_values.value(key, fallback);
}

void SettingsStore::setValue(const QString &key, const QVariant &value)
{
    m_values.insert(key, value);
}

bool SettingsStore::save(QString *error)
{
    if (m_readOnly) {
        if (error)
            *error = QStringLiteral("Settings are read-only");
        return false;
    }

    const QString dirPath = QFileInfo(m_filePath).absolutePath();
    if (!QDir().mkpath(dirPath)) {
        if (error)
            *error = QStringLiteral("Could not create %1").arg(dirPath);
        return false;
    }

    m_values.insert(QStringLiteral("schema_version"), kSchemaVersion);

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("Could not open %1: %2").arg(m_filePath, file.errorString());
        return false;
    }

    const QByteArray payload = serializeIni(m_values).toUtf8();
    if (file.write(payload) != payload.size() || !file.commit()) {
        if (error)
            *error = QStringLiteral("Could not write %1: %2").arg(m_filePath, file.errorString());
        return false;
    }
    return true;
}

QMap<QString, QVariant> SettingsStore::parseIni(const QString &text, bool *ok)
{
    QMap<QString, QVariant> result;
    bool valid = true;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';')))
            continue;

        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            valid = false;
            break;
        }

        const QString key = line.left(separator).trimmed();
        const QString rawValue = line.mid(separator + 1).trimmed();
        if (key.isEmpty()) {
            valid = false;
            break;
        }

        if (rawValue.startsWith(QLatin1Char('"'))) {
            bool unquoteOk = false;
            const QString value = unquoteString(rawValue, &unquoteOk);
            if (!unquoteOk) {
                valid = false;
                break;
            }
            result.insert(key, value);
        } else if (rawValue == QLatin1String("true") || rawValue == QLatin1String("false")) {
            result.insert(key, rawValue == QLatin1String("true"));
        } else {
            bool intOk = false;
            const qlonglong intValue = rawValue.toLongLong(&intOk);
            if (intOk) {
                result.insert(key, intValue);
                continue;
            }
            bool doubleOk = false;
            const double doubleValue = rawValue.toDouble(&doubleOk);
            if (doubleOk) {
                result.insert(key, doubleValue);
                continue;
            }
            result.insert(key, rawValue);
        }
    }

    if (ok)
        *ok = valid;
    return result;
}

QString SettingsStore::serializeIni(const QMap<QString, QVariant> &values)
{
    QString output;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        const QVariant &value = it.value();
        QString encoded;
        switch (value.typeId()) {
        case QMetaType::Bool:
            encoded = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            encoded = QString::number(value.toLongLong());
            break;
        case QMetaType::Double:
            encoded = QString::number(value.toDouble(), 'g', 12);
            break;
        default:
            encoded = quoteString(value.toString());
            break;
        }
        output += it.key() + QLatin1Char('=') + encoded + QLatin1Char('\n');
    }
    return output;
}

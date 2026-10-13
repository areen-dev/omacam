#pragma once

#include <QMap>
#include <QString>
#include <QVariant>

class SettingsStore
{
public:
    enum class LoadStatus {
        Loaded,
        Defaults,
        Corrupt,
        NewerSchema,
        Unreadable,
    };

    static constexpr int kSchemaVersion = 1;

    explicit SettingsStore(QString filePath);

    static QString defaultFilePath();
    static QString legacyFilePath();

    LoadStatus load();
    LoadStatus status() const { return m_status; }
    QString statusMessage() const { return m_statusMessage; }
    bool readOnly() const { return m_readOnly; }

    bool contains(const QString &key) const { return m_values.contains(key); }
    QVariant value(const QString &key, const QVariant &fallback = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    bool save(QString *error = nullptr);
    QString filePath() const { return m_filePath; }

    static QMap<QString, QVariant> parseIni(const QString &text, bool *ok = nullptr);
    static QString serializeIni(const QMap<QString, QVariant> &values);

private:
    QString m_filePath;
    QMap<QString, QVariant> m_values;
    LoadStatus m_status = LoadStatus::Defaults;
    QString m_statusMessage;
    bool m_readOnly = false;
};

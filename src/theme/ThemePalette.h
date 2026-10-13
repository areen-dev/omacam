#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>

#include "theme/ThemeParser.h"

class ThemePalette : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor background READ background NOTIFY paletteChanged)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY paletteChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY paletteChanged)
    Q_PROPERTY(QColor surfaceAlt READ surfaceAlt NOTIFY paletteChanged)
    Q_PROPERTY(QColor muted READ muted NOTIFY paletteChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
    Q_PROPERTY(QColor danger READ danger NOTIFY paletteChanged)
    Q_PROPERTY(QColor success READ success NOTIFY paletteChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QString source READ source NOTIFY paletteChanged)

public:
    explicit ThemePalette(QObject *parent = nullptr);

    QColor background() const { return m_background; }
    QColor foreground() const { return m_foreground; }
    QColor surface() const { return m_surface; }
    QColor surfaceAlt() const { return m_surfaceAlt; }
    QColor muted() const { return m_muted; }
    QColor accent() const { return m_accent; }
    QColor danger() const { return m_danger; }
    QColor success() const { return m_success; }
    bool dark() const { return m_dark; }
    QString source() const { return m_source; }

    Q_INVOKABLE void reload();

signals:
    void paletteChanged();

private:
    void queryOmarchy();
    void applyOutput(const QString &text);
    void scheduleReload();
    void updateWatchPaths();

    QProcess m_process;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QColor m_background;
    QColor m_foreground;
    QColor m_surface;
    QColor m_surfaceAlt;
    QColor m_muted;
    QColor m_accent;
    QColor m_danger;
    QColor m_success;
    bool m_dark = true;
    QString m_source = QStringLiteral("fallback");
};

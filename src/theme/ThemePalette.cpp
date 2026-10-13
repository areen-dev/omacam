#include "theme/ThemePalette.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace {

const char *kFallbackBackground = "#101010";
const char *kFallbackForeground = "#f2f2f2";
const char *kFallbackSurface = "#161616";
const char *kFallbackSurfaceAlt = "#262626";
const char *kFallbackMuted = "#8f8f8f";
const char *kFallbackAccent = "#808080";
const char *kFallbackDanger = "#bf616a";
const char *kFallbackSuccess = "#a3be8c";

QColor colorOr(const QString &value, const char *fallback)
{
    return QColor(theme::sanitizedColor(value, QString::fromLatin1(fallback)));
}

} // namespace

ThemePalette::ThemePalette(QObject *parent)
    : QObject(parent)
{
    m_background = QColor(QString::fromLatin1(kFallbackBackground));
    m_foreground = QColor(QString::fromLatin1(kFallbackForeground));
    m_surface = QColor(QString::fromLatin1(kFallbackSurface));
    m_surfaceAlt = QColor(QString::fromLatin1(kFallbackSurfaceAlt));
    m_muted = QColor(QString::fromLatin1(kFallbackMuted));
    m_accent = QColor(QString::fromLatin1(kFallbackAccent));
    m_danger = QColor(QString::fromLatin1(kFallbackDanger));
    m_success = QColor(QString::fromLatin1(kFallbackSuccess));

    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        if (exitCode == 0 && status == QProcess::NormalExit)
            applyOutput(QString::fromUtf8(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_source != QLatin1String("fallback")) {
            m_source = QStringLiteral("fallback");
            emit paletteChanged();
        }
    });

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(200);
    connect(&m_debounce, &QTimer::timeout, this, [this] {
        updateWatchPaths();
        queryOmarchy();
    });

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { scheduleReload(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { scheduleReload(); });

    updateWatchPaths();
    queryOmarchy();
}

void ThemePalette::reload()
{
    queryOmarchy();
}

void ThemePalette::queryOmarchy()
{
    if (m_process.state() != QProcess::NotRunning)
        return;
    m_process.start(QStringLiteral("omarchy-theme-color"), {QStringLiteral("--all")});
}

void ThemePalette::applyOutput(const QString &text)
{
    const theme::Colors parsed = theme::parseOmarchyThemeOutput(text);
    if (parsed.background.isEmpty() && parsed.foreground.isEmpty())
        return;

    m_background = colorOr(parsed.background, kFallbackBackground);
    m_foreground = colorOr(parsed.foreground, kFallbackForeground);
    m_surface = colorOr(parsed.surface, kFallbackSurface);
    m_surfaceAlt = colorOr(parsed.surfaceAlt, kFallbackSurfaceAlt);
    m_muted = colorOr(parsed.muted, kFallbackMuted);
    m_accent = colorOr(parsed.accent, kFallbackAccent);
    m_danger = colorOr(parsed.danger, kFallbackDanger);
    m_success = colorOr(parsed.success, kFallbackSuccess);
    m_dark = parsed.mode != QLatin1String("light");
    m_source = QStringLiteral("omarchy");
    qInfo().noquote() << "Theme palette loaded from omarchy-theme-color:"
                      << (m_dark ? QStringLiteral("dark") : QStringLiteral("light"))
                      << m_background.name() << m_accent.name();
    emit paletteChanged();
}

void ThemePalette::scheduleReload()
{
    m_debounce.start();
}

void ThemePalette::updateWatchPaths()
{
    const QString stateDir = QDir::homePath() + QStringLiteral("/.local/state/omarchy/current/theme");
    const QStringList candidates = {
        stateDir + QStringLiteral("/colors.toml"),
        stateDir,
        QDir::homePath() + QStringLiteral("/.local/state/omarchy/current"),
        QDir::homePath() + QStringLiteral("/.local/state/omarchy"),
    };

    QStringList existing;
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            existing.append(path);
    }

    if (!existing.isEmpty())
        m_watcher.addPaths(existing);
}

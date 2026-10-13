#pragma once

#include <QString>

namespace theme {

struct Colors
{
    QString background;
    QString foreground;
    QString surface;
    QString surfaceAlt;
    QString muted;
    QString accent;
    QString danger;
    QString success;
    QString mode = QStringLiteral("dark");
};

Colors parseOmarchyThemeOutput(const QString &text);
bool isHexColor(const QString &value);
QString sanitizedColor(const QString &value, const QString &fallback);

} // namespace theme

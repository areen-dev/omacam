#include "theme/ThemeParser.h"

#include <QRegularExpression>

namespace theme {

Colors parseOmarchyThemeOutput(const QString &text)
{
    Colors colors;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const int separator = line.indexOf(QLatin1Char('\t'));
        if (separator <= 0)
            continue;
        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (key.isEmpty() || value.isEmpty())
            continue;

        if (key == QLatin1String("background"))
            colors.background = value;
        else if (key == QLatin1String("foreground"))
            colors.foreground = value;
        else if (key == QLatin1String("dark_background"))
            colors.surface = value;
        else if (key == QLatin1String("lighter_background"))
            colors.surfaceAlt = value;
        else if (key == QLatin1String("muted"))
            colors.muted = value;
        else if (key == QLatin1String("accent"))
            colors.accent = value;
        else if (key == QLatin1String("red"))
            colors.danger = value;
        else if (key == QLatin1String("green"))
            colors.success = value;
        else if (key == QLatin1String("mode"))
            colors.mode = value;
    }
    return colors;
}

bool isHexColor(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^#(?:[0-9a-fA-F]{3}|[0-9a-fA-F]{6})$"));
    return pattern.match(value).hasMatch();
}

QString sanitizedColor(const QString &value, const QString &fallback)
{
    return isHexColor(value) ? value : fallback;
}

} // namespace theme

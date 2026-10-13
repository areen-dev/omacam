#include <QtTest>

#include "theme/ThemeParser.h"

class TestThemeParser : public QObject
{
    Q_OBJECT

private slots:
    void parsesOmarchyOutput();
    void toleratesMissingKeys();
    void validatesHexColors();
    void sanitizesInvalidColors();
};

void TestThemeParser::parsesOmarchyOutput()
{
    const QString text = QStringLiteral(
        "accent\t#808080\n"
        "background\t#090909\n"
        "dark_background\t#070707\n"
        "foreground\t#f7f7f7\n"
        "green\t#a3be8c\n"
        "lighter_background\t#222222\n"
        "mode\tdark\n"
        "muted\t#62605a\n"
        "red\t#bf616a\n");

    const theme::Colors colors = theme::parseOmarchyThemeOutput(text);
    QCOMPARE(colors.background, QStringLiteral("#090909"));
    QCOMPARE(colors.foreground, QStringLiteral("#f7f7f7"));
    QCOMPARE(colors.surface, QStringLiteral("#070707"));
    QCOMPARE(colors.surfaceAlt, QStringLiteral("#222222"));
    QCOMPARE(colors.muted, QStringLiteral("#62605a"));
    QCOMPARE(colors.accent, QStringLiteral("#808080"));
    QCOMPARE(colors.danger, QStringLiteral("#bf616a"));
    QCOMPARE(colors.success, QStringLiteral("#a3be8c"));
    QCOMPARE(colors.mode, QStringLiteral("dark"));
}

void TestThemeParser::toleratesMissingKeys()
{
    const theme::Colors colors = theme::parseOmarchyThemeOutput(QStringLiteral("mode\tlight\n"));
    QVERIFY(colors.background.isEmpty());
    QCOMPARE(colors.mode, QStringLiteral("light"));

    const theme::Colors empty = theme::parseOmarchyThemeOutput(QString());
    QVERIFY(empty.background.isEmpty());
    QCOMPARE(empty.mode, QStringLiteral("dark"));
}

void TestThemeParser::validatesHexColors()
{
    QVERIFY(theme::isHexColor(QStringLiteral("#fff")));
    QVERIFY(theme::isHexColor(QStringLiteral("#F7F7F7")));
    QVERIFY(!theme::isHexColor(QStringLiteral("rgb(1,2,3)")));
    QVERIFY(!theme::isHexColor(QStringLiteral("#gggggg")));
    QVERIFY(!theme::isHexColor(QStringLiteral("f7f7f7")));
    QVERIFY(!theme::isHexColor(QStringLiteral("#f7f7f7ff")));
}

void TestThemeParser::sanitizesInvalidColors()
{
    QCOMPARE(theme::sanitizedColor(QStringLiteral("#123456"), QStringLiteral("#000000")),
             QStringLiteral("#123456"));
    QCOMPARE(theme::sanitizedColor(QStringLiteral("rgb(1,2,3)"), QStringLiteral("#000000")),
             QStringLiteral("#000000"));
}

QTEST_APPLESS_MAIN(TestThemeParser)

#include "tst_theme_parser.moc"

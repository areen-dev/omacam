#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
#include <QTimer>
#include "camera/CameraModel.h"
#include "theme/ThemePalette.h"

namespace {

class SelfTest : public QObject
{
public:
    SelfTest(CameraModel *model, const QString &outputDir, const QString &format, int videoSeconds,
             bool quitDuringRecord, int timerSeconds, bool cycleTimer, bool recordDuringTimer)
        : m_model(model)
        , m_outputDir(outputDir)
        , m_format(format)
        , m_videoSeconds(videoSeconds)
        , m_quitDuringRecord(quitDuringRecord)
        , m_timerSeconds(timerSeconds)
        , m_timerOk(timerSeconds == 0)
        , m_timerCancelOk(timerSeconds == 0)
        , m_cycleTimer(cycleTimer)
        , m_recordDuringTimer(recordDuringTimer)
    {
        connect(model, &CameraModel::photoSaved, this, [this](const QString &path) {
            report(QStringLiteral("photoSaved %1").arg(path));
            if (m_recordingActiveForTimerTest) {
                m_timerDuringRecordOk = false;
                report(QStringLiteral("timerDuringRecord photoSavedDuringRecord %1").arg(path));
            }
            if (m_timerSeconds > 0) {
                const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_photoRequestMs;
                report(QStringLiteral("timerElapsed ms=%1 expected=%2").arg(elapsed).arg(m_timerSeconds * 1000));
                m_timerOk = elapsed >= (m_timerSeconds * 1000) - 150;
            }
            m_photoOk = true;
            m_phase = Phase::WaitingForRecordStart;
            QTimer::singleShot(800, this, [this] { startRecording(); });
        });
        connect(model, &CameraModel::photoFailed, this, [this](const QString &message) {
            report(QStringLiteral("photoFailed %1").arg(message));
            m_phase = Phase::WaitingForRecordStart;
            QTimer::singleShot(800, this, [this] { startRecording(); });
        });
        connect(model, &CameraModel::videoSaved, this, [this](const QString &path) {
            report(QStringLiteral("videoSaved %1").arg(path));
            m_videoOk = true;
            finish();
        });
        connect(model, &CameraModel::videoFailed, this, [this](const QString &message) {
            report(QStringLiteral("videoFailed %1").arg(message));
            finish();
        });
        connect(model, &CameraModel::message, this, [this](const QString &text, bool isError) {
            report(QStringLiteral("message error=%1 %2").arg(isError ? QStringLiteral("true") : QStringLiteral("false"), text));
        });
    }

    void start()
    {
        m_model->setPhotosDir(m_outputDir + QStringLiteral("/photos"));
        m_model->setVideosDir(m_outputDir + QStringLiteral("/videos"));

        m_watchdog.start(60000);
        connect(&m_watchdog, &QTimer::timeout, this, [this] {
            report(QStringLiteral("watchdog timeout phase=%1").arg(static_cast<int>(m_phase)));
            finish();
        });

        QTimer::singleShot(1500, this, [this] {
            report(QStringLiteral("device %1 (%2)").arg(m_model->deviceDescription(), m_model->deviceId()));
            report(QStringLiteral("state %1").arg(m_model->stateText()));
            QStringList resolutions;
            for (const QVariant &value : m_model->resolutions())
                resolutions << value.toMap().value(QStringLiteral("value")).toString();
            report(QStringLiteral("resolutions %1").arg(resolutions.join(QLatin1Char(','))));
            report(QStringLiteral("pixelFormats %1").arg(m_model->pixelFormats().join(QLatin1Char(','))));
            QStringList frameRates;
            for (const QVariant &value : m_model->frameRates())
                frameRates << value.toMap().value(QStringLiteral("label")).toString();
            report(QStringLiteral("frameRates %1").arg(frameRates.join(QLatin1Char(','))));

            if (!m_format.isEmpty()) {
                m_model->setResolution(m_format);
                if (m_model->pixelFormats().contains(QStringLiteral("MJPEG")))
                    m_model->setPixelFormat(QStringLiteral("MJPEG"));
                report(QStringLiteral("selection %1 %2 %3").arg(m_model->resolution(), m_model->pixelFormat())
                       .arg(m_model->frameRate()));
            }

            if (m_cycleTimer) {
                QStringList cycle;
                for (int i = 0; i < 4; ++i) {
                    m_model->cycleTimer();
                    cycle << QString::number(m_model->timerSeconds());
                }
                report(QStringLiteral("timerCycle %1").arg(cycle.join(QLatin1Char(','))));
                m_cycleOk = cycle == QStringList({QStringLiteral("3"), QStringLiteral("10"),
                                                  QStringLiteral("0"), QStringLiteral("3")});
                m_timerSeconds = m_model->timerSeconds();
                m_timerCancelOk = true;
            }

            if (m_recordDuringTimer) {
                m_model->setTimerSeconds(3);
                m_timerSeconds = 3;
                m_timerOk = true;
                m_timerCancelOk = true;
                m_phase = Phase::WaitingForPhoto;
                QTimer::singleShot(1200, this, [this] {
                    m_photoRequestMs = QDateTime::currentMSecsSinceEpoch();
                    report(QStringLiteral("timerDuringRecord photoRequest timer=3"));
                    m_model->takePhoto();
                    QTimer::singleShot(500, this, [this] {
                        m_recordingActiveForTimerTest = true;
                        report(QStringLiteral("timerDuringRecord recordRequest"));
                        m_model->startRecording();
                        QTimer::singleShot(4000, this, [this] {
                            report(QStringLiteral("timerDuringRecord recordStop"));
                            m_model->stopRecording();
                        });
                    });
                });
                return;
            }

            m_phase = Phase::WaitingForPhoto;
            QTimer::singleShot(1200, this, [this] {
                if (m_timerSeconds > 0)
                    m_model->setTimerSeconds(m_timerSeconds);
                m_photoRequestMs = QDateTime::currentMSecsSinceEpoch();
                report(QStringLiteral("photoRequest timer=%1").arg(m_timerSeconds));
                m_model->takePhoto();
                if (m_timerSeconds > 0) {
                    QTimer::singleShot(1000, this, [this] {
                        const bool wasActive = m_model->timerActive();
                        m_model->cancelTimer();
                        m_timerCancelOk = wasActive && !m_model->timerActive();
                        report(QStringLiteral("timerCancel wasActive=%1 activeAfter=%2")
                                   .arg(wasActive ? QStringLiteral("true") : QStringLiteral("false"),
                                        m_model->timerActive() ? QStringLiteral("true") : QStringLiteral("false")));
                        m_photoRequestMs = QDateTime::currentMSecsSinceEpoch();
                        m_model->takePhoto();
                    });
                }
            });
        });
    }

private:
    enum class Phase {
        Idle,
        WaitingForPhoto,
        WaitingForRecordStart,
        Recording,
    };

    void report(const QString &line)
    {
        QTextStream out(stdout);
        out << "SELFTEST " << line << Qt::endl;
    }

    void startRecording()
    {
        if (m_phase == Phase::Recording)
            return;
        m_phase = Phase::Recording;
        report(QStringLiteral("recordRequest seconds=%1").arg(m_videoSeconds));
        m_model->startRecording();
        QTimer::singleShot(m_videoSeconds * 1000, this, [this] {
            if (m_quitDuringRecord) {
                report(QStringLiteral("quitDuringRecord"));
                QGuiApplication::quit();
                return;
            }
            report(QStringLiteral("recordStop"));
            m_model->stopRecording();
        });
    }

    void finish()
    {
        if (m_finished)
            return;
        m_finished = true;
        m_model->shutdown();
        if (m_recordDuringTimer) {
            report(QStringLiteral("result timerDuringRecord=%1 video=%2")
                       .arg(m_timerDuringRecordOk ? QStringLiteral("ok") : QStringLiteral("fail"),
                            m_videoOk ? QStringLiteral("ok") : QStringLiteral("fail")));
            QGuiApplication::exit((m_timerDuringRecordOk && m_videoOk) ? 0 : 2);
            return;
        }
        report(QStringLiteral("result photo=%1 video=%2 timer=%3 cycle=%4")
                   .arg(m_photoOk ? QStringLiteral("ok") : QStringLiteral("fail"),
                        m_videoOk ? QStringLiteral("ok") : QStringLiteral("fail"),
                        (m_timerOk && m_timerCancelOk) ? QStringLiteral("ok") : QStringLiteral("fail"),
                        m_cycleOk ? QStringLiteral("ok") : QStringLiteral("fail")));
        QGuiApplication::exit((m_photoOk && m_videoOk && m_timerOk && m_timerCancelOk && m_cycleOk) ? 0 : 2);
    }

    CameraModel *m_model = nullptr;
    QString m_outputDir;
    QString m_format;
    int m_videoSeconds = 5;
    bool m_quitDuringRecord = false;
    int m_timerSeconds = 0;
    qint64 m_photoRequestMs = 0;
    Phase m_phase = Phase::Idle;
    bool m_photoOk = false;
    bool m_videoOk = false;
    bool m_timerOk = true;
    bool m_timerCancelOk = true;
    bool m_cycleTimer = false;
    bool m_cycleOk = true;
    bool m_recordDuringTimer = false;
    bool m_timerDuringRecordOk = true;
    bool m_recordingActiveForTimerTest = false;
    bool m_finished = false;
    QTimer m_watchdog;
};

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName(QStringLiteral("dev.areen.omacam"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Omacam"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setDesktopFileName(QStringLiteral("dev.areen.omacam"));

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Omacam camera app for Omarchy"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption selfTestOption(QStringLiteral("self-test"),
                                      QStringLiteral("Run an automated camera capture check into <dir>."),
                                      QStringLiteral("dir"));
    QCommandLineOption formatOption(QStringLiteral("format"),
                                    QStringLiteral("Resolution to select in self-test mode (WxH)."),
                                    QStringLiteral("WxH"));
    QCommandLineOption secondsOption(QStringLiteral("seconds"),
                                     QStringLiteral("Recording length in self-test mode."),
                                     QStringLiteral("seconds"),
                                     QStringLiteral("5"));
    QCommandLineOption quitDuringRecordOption(QStringLiteral("quit-during-record"),
                                              QStringLiteral("Quit while recording in self-test mode."));
    QCommandLineOption timerOption(QStringLiteral("timer-seconds"),
                                   QStringLiteral("Use the self-timer with this delay in self-test mode."),
                                   QStringLiteral("seconds"),
                                   QStringLiteral("0"));
    QCommandLineOption cycleTimerOption(QStringLiteral("cycle-timer"),
                                        QStringLiteral("Cycle the timer (3, 10, off) in self-test mode."));
    QCommandLineOption recordDuringTimerOption(QStringLiteral("record-during-timer"),
                                               QStringLiteral("Start recording while the self-timer countdown runs in self-test mode."));
    QCommandLineOption uiCheckOption(QStringLiteral("ui-check"),
                                     QStringLiteral("Load the UI, then report measured preview fps after <seconds>."),
                                     QStringLiteral("seconds"),
                                     QStringLiteral("6"));
    parser.addOption(selfTestOption);
    parser.addOption(formatOption);
    parser.addOption(secondsOption);
    parser.addOption(quitDuringRecordOption);
    parser.addOption(timerOption);
    parser.addOption(cycleTimerOption);
    parser.addOption(recordDuringTimerOption);
    parser.addOption(uiCheckOption);
    parser.process(app);

    CameraModel cameraModel;

    if (parser.isSet(selfTestOption)) {
        const bool quitDuringRecord = parser.isSet(quitDuringRecordOption);
        SelfTest selfTest(&cameraModel, parser.value(selfTestOption),
                          parser.value(formatOption),
                          parser.value(secondsOption).toInt(),
                          quitDuringRecord,
                          parser.value(timerOption).toInt(),
                          parser.isSet(cycleTimerOption),
                          parser.isSet(recordDuringTimerOption));
        selfTest.start();
        const int result = app.exec();
        if (quitDuringRecord) {
            cameraModel.shutdown();
            QTextStream out(stdout);
            const QString path = cameraModel.lastVideoPath();
            const bool ok = !path.isEmpty() && QFileInfo::exists(path);
            out << "SELFTEST postShutdown video=" << (path.isEmpty() ? QStringLiteral("none") : path)
                << " exists=" << (ok ? QStringLiteral("true") : QStringLiteral("false")) << Qt::endl;
            return ok ? 0 : 2;
        }
        return result;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    ThemePalette themePalette;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("CameraModel"), &cameraModel);
    engine.rootContext()->setContextProperty(QStringLiteral("Theme"), &themePalette);
    engine.loadFromModule("dev.areen.omacam", "Main");

    if (engine.rootObjects().isEmpty())
        return 1;

    if (parser.isSet(uiCheckOption)) {
        QTimer::singleShot(std::max(1, parser.value(uiCheckOption).toInt()) * 1000, &app, [&cameraModel, &engine] {
            QTextStream out(stdout);
            const double fps = cameraModel.measuredFps();
            const QVariant settingsVisible = engine.rootObjects().isEmpty()
                ? QVariant()
                : engine.rootObjects().first()->property("settingsVisible");
            QObject *errorBanner = engine.rootObjects().isEmpty()
                ? nullptr
                : engine.rootObjects().first()->findChild<QObject *>(QStringLiteral("errorBanner"));
            const bool errorBannerVisible = errorBanner && errorBanner->property("visible").toBool();
            out << "UICHECK measuredFps=" << fps
                << " state=" << cameraModel.stateText()
                << " settingsVisible=" << (settingsVisible.toBool() ? QStringLiteral("true") : QStringLiteral("false"))
                << " errorBanner=" << (errorBannerVisible ? QStringLiteral("true") : QStringLiteral("false"))
                << " format=" << cameraModel.activeFormatLabel() << Qt::endl;
            QGuiApplication::exit(fps > 0.0 ? 0 : 3);
        });
    }

    return app.exec();
}

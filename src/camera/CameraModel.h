#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCameraFormat>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QMediaRecorder>

#include "camera/CatalogFormat.h"

#include <memory>

class QAudioInput;
class SettingsStore;

class CameraModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool hasCamera READ hasCamera NOTIFY devicesChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceDescription READ deviceDescription NOTIFY deviceChanged)

    Q_PROPERTY(QVariantList resolutions READ resolutions NOTIFY formatsChanged)
    Q_PROPERTY(QString resolution READ resolution NOTIFY selectionChanged)
    Q_PROPERTY(QStringList pixelFormats READ pixelFormats NOTIFY formatsChanged)
    Q_PROPERTY(QString pixelFormat READ pixelFormat NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList frameRates READ frameRates NOTIFY formatsChanged)
    Q_PROPERTY(double frameRate READ frameRate NOTIFY selectionChanged)
    Q_PROPERTY(QString activeFormatLabel READ activeFormatLabel NOTIFY activeFormatChanged)

    Q_PROPERTY(double measuredFps READ measuredFps NOTIFY measuredFpsChanged)
    Q_PROPERTY(int timerSeconds READ timerSeconds WRITE setTimerSeconds NOTIFY timerSecondsChanged)
    Q_PROPERTY(int timerRemaining READ timerRemaining NOTIFY timerRemainingChanged)
    Q_PROPERTY(bool timerActive READ timerActive NOTIFY timerActiveChanged)
    Q_PROPERTY(bool mirror READ mirror WRITE setMirror NOTIFY mirrorChanged)
    Q_PROPERTY(bool micEnabled READ micEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)
    Q_PROPERTY(bool micAvailable READ micAvailable NOTIFY micAvailableChanged)
    Q_PROPERTY(QString photosDir READ photosDir WRITE setPhotosDir NOTIFY photosDirChanged)
    Q_PROPERTY(QString videosDir READ videosDir WRITE setVideosDir NOTIFY videosDirChanged)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(bool imageCaptureReady READ imageCaptureReady NOTIFY imageCaptureReadyChanged)
    Q_PROPERTY(QString lastPhotoPath READ lastPhotoPath NOTIFY lastPhotoPathChanged)
    Q_PROPERTY(QString lastVideoPath READ lastVideoPath NOTIFY lastVideoPathChanged)
    Q_PROPERTY(qint64 recordingDuration READ recordingDuration NOTIFY recordingDurationChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY stateChanged)
    Q_PROPERTY(QString startupWarning READ startupWarning NOTIFY startupWarningChanged)

public:
    enum class State {
        NoCamera,
        Starting,
        Streaming,
        Recording,
        Error,
    };
    Q_ENUM(State)

    explicit CameraModel(QObject *parent = nullptr);
    ~CameraModel() override;

    QVariantList devices() const { return m_devices; }
    bool hasCamera() const { return !m_devices.isEmpty(); }
    QString deviceId() const { return m_deviceId; }
    QString deviceDescription() const { return m_deviceDescription; }

    QVariantList resolutions() const { return m_resolutions; }
    QString resolution() const { return m_resolution; }
    QStringList pixelFormats() const { return m_pixelFormats; }
    QString pixelFormat() const { return m_pixelFormat; }
    QVariantList frameRates() const { return m_frameRates; }
    double frameRate() const { return m_frameRate; }
    QString activeFormatLabel() const { return m_activeFormatLabel; }

    double measuredFps() const { return m_measuredFps; }
    int timerSeconds() const { return m_timerSeconds; }
    void setTimerSeconds(int seconds);
    int timerRemaining() const { return m_timerRemaining; }
    bool timerActive() const { return m_timerActive; }
    bool mirror() const { return m_mirror; }
    void setMirror(bool mirror);
    bool micEnabled() const { return m_micEnabled; }
    void setMicEnabled(bool enabled);
    bool micAvailable() const { return m_micAvailable; }
    QString photosDir() const { return m_photosDir; }
    void setPhotosDir(const QString &dir);
    QString videosDir() const { return m_videosDir; }
    void setVideosDir(const QString &dir);

    State state() const { return m_state; }
    QString stateText() const;
    bool hasError() const { return m_state == State::Error; }
    QString errorText() const { return m_errorText; }
    bool imageCaptureReady() const { return m_imageCaptureReady; }
    QString lastPhotoPath() const { return m_lastPhotoPath; }
    QString lastVideoPath() const { return m_lastVideoPath; }
    qint64 recordingDuration() const { return m_recordingDuration; }
    bool recording() const { return m_state == State::Recording; }
    QString startupWarning() const { return m_startupWarning; }

    Q_INVOKABLE void selectDevice(const QString &id);
    Q_INVOKABLE void setResolution(const QString &resolution);
    Q_INVOKABLE void setPixelFormat(const QString &pixelFormat);
    Q_INVOKABLE void setFrameRate(double fps);
    Q_INVOKABLE void takePhoto();
    Q_INVOKABLE void cancelTimer();
    Q_INVOKABLE void cycleTimer();
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void openPhotosDir();
    Q_INVOKABLE void openVideosDir();
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void retry();
    Q_INVOKABLE void attachVideoOutput(QObject *videoOutput);
    Q_INVOKABLE void notePreviewFrame();
    Q_INVOKABLE void shutdown();

signals:
    void devicesChanged();
    void deviceChanged();
    void formatsChanged();
    void selectionChanged();
    void activeFormatChanged();
    void measuredFpsChanged();
    void timerSecondsChanged();
    void timerRemainingChanged();
    void timerActiveChanged();
    void mirrorChanged();
    void micEnabledChanged();
    void micAvailableChanged();
    void photosDirChanged();
    void videosDirChanged();
    void stateChanged();
    void errorTextChanged();
    void imageCaptureReadyChanged();
    void lastPhotoPathChanged();
    void lastVideoPathChanged();
    void recordingDurationChanged();
    void startupWarningChanged();

    void photoSaved(const QString &path);
    void photoFailed(const QString &message);
    void videoSaved(const QString &path);
    void videoFailed(const QString &message);
    void message(const QString &text, bool isError);

private:
    void setState(State state);
    void setErrorText(const QString &text);
    void setStartupWarning(const QString &text);
    void rebuildDevices(bool initial);
    void rebuildFormats();
    void applySelection(const QSize &resolution, const QString &pixelFormat, double frameRate, bool persist);
    void applyCameraFormat();
    void updateActiveFormatLabel();
    void captureNow();
    void saveSettings();
    void loadSettings();
    void chooseRecordingFormat();
    void finalizeRecording(bool success);
    void failRecordingStart(const QString &error);
    QString pixelFormatLabel(int pixelFormat) const;
    static QSize parseResolution(const QString &text);

    QMediaDevices m_mediaDevices;
    QMediaCaptureSession m_session;
    QCamera *m_camera = nullptr;
    QImageCapture *m_imageCapture = nullptr;
    QMediaRecorder *m_recorder = nullptr;
    QAudioInput *m_audioInput = nullptr;

    std::unique_ptr<SettingsStore> m_settings;

    QList<QCameraDevice> m_cameraDevices;
    QCameraDevice m_cameraDevice;
    QVariantList m_devices;
    QString m_deviceId;
    QString m_deviceDescription;

    QList<catalog::Entry> m_catalog;
    QVariantList m_resolutions;
    QStringList m_pixelFormats;
    QVariantList m_frameRates;
    QString m_resolution;
    QString m_pixelFormat;
    double m_frameRate = 0.0;
    QString m_activeFormatLabel;

    bool m_mirror = true;
    bool m_micEnabled = true;
    bool m_micAvailable = false;
    QString m_photosDir;
    QString m_videosDir;
    State m_state = State::NoCamera;
    QString m_errorText;
    bool m_imageCaptureReady = false;
    QString m_lastPhotoPath;
    QString m_lastVideoPath;
    qint64 m_recordingDuration = 0;
    QString m_startupWarning;

    QString m_recordingExtension = QStringLiteral("mp4");
    QString m_recordPartPath;
    QString m_recordFinalPath;
    bool m_recordingFinalizePending = false;
    bool m_recordingStartPending = false;

    QTimer m_fpsTimer;
    qint64 m_frameCount = 0;
    double m_measuredFps = 0.0;
    QTimer m_selfTimer;
    QTimer m_recordStartTimer;
    int m_timerSeconds = 0;
    int m_timerRemaining = 0;
    bool m_timerActive = false;
    bool m_initialized = false;
    bool m_shuttingDown = false;
};

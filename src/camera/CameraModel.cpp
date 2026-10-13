#include "camera/CameraModel.h"

#include "config/OutputPaths.h"
#include "config/SettingsStore.h"

#include <QAudioDevice>
#include <QAudioInput>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHash>
#include <QImageWriter>
#include <QMediaFormat>
#include <QMetaEnum>
#include <QThreadPool>
#include <QUrl>
#include <QVideoFrameFormat>

#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>

namespace {

const char *kDeviceKey = "camera_device_id";
const char *kResolutionKey = "resolution";
const char *kPixelFormatKey = "pixel_format";
const char *kFrameRateKey = "frame_rate";
const char *kMirrorKey = "mirror";
const char *kMicKey = "mic_enabled";
const char *kTimerKey = "self_timer_seconds";
const char *kPhotosDirKey = "photos_dir";
const char *kVideosDirKey = "videos_dir";

} // namespace

CameraModel::CameraModel(QObject *parent)
    : QObject(parent)
{
    m_settings = std::make_unique<SettingsStore>(SettingsStore::defaultFilePath());
    const SettingsStore::LoadStatus status = m_settings->load();
    if (status != SettingsStore::LoadStatus::Loaded && status != SettingsStore::LoadStatus::Defaults)
        setStartupWarning(m_settings->statusMessage());

    m_photosDir = m_settings->value(kPhotosDirKey, output::picturesDir()).toString();
    m_videosDir = m_settings->value(kVideosDirKey, output::videosDir()).toString();
    if (m_photosDir.isEmpty())
        m_photosDir = output::picturesDir();
    if (m_videosDir.isEmpty())
        m_videosDir = output::videosDir();
    m_mirror = m_settings->value(kMirrorKey, true).toBool();
    m_micEnabled = m_settings->value(kMicKey, true).toBool();
    m_timerSeconds = m_settings->value(kTimerKey, 0).toInt();
    if (m_timerSeconds != 3 && m_timerSeconds != 10)
        m_timerSeconds = 0;

    m_audioInput = new QAudioInput(this);
    m_micAvailable = !QMediaDevices::defaultAudioInput().isNull();

    m_camera = new QCamera(this);
    m_imageCapture = new QImageCapture(this);
    m_recorder = new QMediaRecorder(this);

    m_session.setCamera(m_camera);
    m_session.setImageCapture(m_imageCapture);
    m_session.setRecorder(m_recorder);
    m_session.setAudioInput(nullptr);

    connect(m_camera, &QCamera::activeChanged, this, [this](bool active) {
        if (m_shuttingDown)
            return;
        if (active) {
            if (m_state == State::Starting)
                setState(State::Streaming);
        } else if (m_state == State::Streaming && m_camera->error() != QCamera::NoError) {
            setState(State::Error);
            setErrorText(QStringLiteral("Camera stopped unexpectedly"));
        }
    });
    connect(m_camera, &QCamera::cameraFormatChanged, this, [this] {
        updateActiveFormatLabel();
    });
    connect(m_camera, &QCamera::errorOccurred, this, [this](QCamera::Error, const QString &errorString) {
        if (m_state != State::Recording && m_state != State::Error) {
            m_camera->stop();
            setState(State::Error);
        }
        setErrorText(errorString);
        emit message(errorString, true);
    });

    connect(m_imageCapture, &QImageCapture::readyForCaptureChanged, this, [this](bool ready) {
        if (m_imageCaptureReady == ready)
            return;
        m_imageCaptureReady = ready;
        emit imageCaptureReadyChanged();
    });
    connect(m_imageCapture, &QImageCapture::imageCaptured, this, [this](int, const QImage &preview) {
        if (preview.isNull()) {
            const QString text = QStringLiteral("Camera returned an empty frame");
            emit photoFailed(text);
            emit message(text, true);
            return;
        }

        QString error;
        if (!output::ensureDir(m_photosDir, &error)) {
            emit photoFailed(error);
            emit message(error, true);
            return;
        }

        const QString path = output::uniqueMediaPath(m_photosDir, QStringLiteral("IMG"),
                                                     QStringLiteral("jpg"), QDateTime::currentDateTime(),
                                                     [](const QString &candidate) {
                                                         return QFileInfo::exists(candidate);
                                                     });

        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, path] {
            const bool saved = watcher->result();
            watcher->deleteLater();
            if (saved) {
                m_lastPhotoPath = path;
                emit lastPhotoPathChanged();
                emit photoSaved(path);
                emit message(QStringLiteral("Photo saved"), false);
            } else {
                const QString text = QStringLiteral("Could not save %1").arg(path);
                emit photoFailed(text);
                emit message(text, true);
            }
        });
        watcher->setFuture(QtConcurrent::run([preview, path] {
            QImageWriter writer(path, QByteArrayLiteral("jpeg"));
            writer.setQuality(92);
            return writer.write(preview);
        }));
    });
    connect(m_imageCapture, &QImageCapture::errorOccurred, this,
            [this](int, QImageCapture::Error, const QString &errorString) {
                emit photoFailed(errorString);
                emit message(errorString, true);
            });

    connect(m_recorder, &QMediaRecorder::recorderStateChanged, this, [this](QMediaRecorder::RecorderState recorderState) {
        if (recorderState == QMediaRecorder::RecordingState) {
            m_recordingStartPending = false;
            m_recordStartTimer.stop();
            setState(State::Recording);
            m_recordingFinalizePending = false;
        } else if (recorderState == QMediaRecorder::StoppedState) {
            if (m_recordingStartPending) {
                failRecordingStart(m_recorder->errorString());
            } else if (m_recordingFinalizePending) {
                finalizeRecording(m_recorder->error() == QMediaRecorder::NoError);
            } else if (m_state == State::Recording) {
                finalizeRecording(false);
            }
        }
    });
    connect(m_recorder, &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
        if (m_recordingDuration == duration)
            return;
        m_recordingDuration = duration;
        emit recordingDurationChanged();
    });
    connect(m_recorder, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error, const QString &errorString) {
                setErrorText(errorString);
                if (m_recordingStartPending && m_recorder->recorderState() != QMediaRecorder::RecordingState) {
                    failRecordingStart(errorString);
                    return;
                }
                if (m_recorder->recorderState() == QMediaRecorder::RecordingState)
                    m_recorder->stop();
            });

    connect(&m_mediaDevices, &QMediaDevices::videoInputsChanged, this, [this] {
        if (m_recorder->recorderState() != QMediaRecorder::StoppedState)
            return;
        refreshDevices();
    });

    m_fpsTimer.setInterval(1000);
    connect(&m_fpsTimer, &QTimer::timeout, this, [this] {
        const double fps = static_cast<double>(m_frameCount);
        m_frameCount = 0;
        if (std::abs(fps - m_measuredFps) < 0.05)
            return;
        m_measuredFps = fps;
        emit measuredFpsChanged();
    });
    m_fpsTimer.start();

    m_selfTimer.setInterval(1000);
    m_selfTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_selfTimer, &QTimer::timeout, this, [this] {
        if (!m_timerActive)
            return;
        --m_timerRemaining;
        emit timerRemainingChanged();
        if (m_timerRemaining > 0)
            return;
        m_timerActive = false;
        m_selfTimer.stop();
        emit timerActiveChanged();
        if (m_state == State::Recording) {
            const QString text = QStringLiteral("Stop recording before taking a photo");
            emit photoFailed(text);
            emit message(text, true);
            return;
        }
        captureNow();
    });

    m_recordStartTimer.setSingleShot(true);
    m_recordStartTimer.setInterval(5000);
    connect(&m_recordStartTimer, &QTimer::timeout, this, [this] {
        failRecordingStart(QStringLiteral("Recording did not start"));
    });

    rebuildDevices(true);

    m_initialized = true;
    if (m_camera->isActive() || hasCamera())
        saveSettings();
}

CameraModel::~CameraModel()
{
    shutdown();
}

QString CameraModel::stateText() const
{
    switch (m_state) {
    case State::NoCamera:
        return QStringLiteral("No camera found");
    case State::Starting:
        return QStringLiteral("Starting camera…");
    case State::Streaming:
        return QStringLiteral("Streaming");
    case State::Recording:
        return QStringLiteral("Recording");
    case State::Error:
        return QStringLiteral("Camera error");
    }
    return QString();
}

void CameraModel::setMirror(bool mirror)
{
    if (m_mirror == mirror)
        return;
    m_mirror = mirror;
    emit mirrorChanged();
    saveSettings();
}

void CameraModel::setMicEnabled(bool enabled)
{
    if (m_micEnabled == enabled)
        return;
    m_micEnabled = enabled;
    if (m_recorder->recorderState() != QMediaRecorder::RecordingState)
        m_session.setAudioInput((m_micEnabled && m_micAvailable) ? m_audioInput : nullptr);
    emit micEnabledChanged();
    saveSettings();
}

void CameraModel::setTimerSeconds(int seconds)
{
    const int clamped = (seconds == 3 || seconds == 10) ? seconds : 0;
    if (m_timerSeconds == clamped)
        return;
    m_timerSeconds = clamped;
    cancelTimer();
    emit timerSecondsChanged();
    saveSettings();
}

void CameraModel::cycleTimer()
{
    const int next = m_timerSeconds == 0 ? 3 : (m_timerSeconds == 3 ? 10 : 0);
    setTimerSeconds(next);
}

void CameraModel::setPhotosDir(const QString &dir)
{
    const QString trimmed = dir.trimmed();
    if (trimmed.isEmpty() || trimmed == m_photosDir)
        return;
    m_photosDir = trimmed;
    emit photosDirChanged();
    saveSettings();
}

void CameraModel::setVideosDir(const QString &dir)
{
    const QString trimmed = dir.trimmed();
    if (trimmed.isEmpty() || trimmed == m_videosDir)
        return;
    m_videosDir = trimmed;
    emit videosDirChanged();
    saveSettings();
}

void CameraModel::selectDevice(const QString &id)
{
    if (id == m_deviceId)
        return;
    cancelTimer();
    if (m_recorder->recorderState() != QMediaRecorder::StoppedState) {
        emit message(QStringLiteral("Stop recording before switching camera"), true);
        return;
    }

    const QCameraDevice device = [this, &id] {
        for (const QCameraDevice &candidate : m_cameraDevices) {
            if (QString::fromUtf8(candidate.id()) == id)
                return candidate;
        }
        return QCameraDevice();
    }();
    if (device.isNull())
        return;

    m_camera->stop();
    m_cameraDevice = device;
    m_deviceId = QString::fromUtf8(device.id());
    m_deviceDescription = device.description();
    m_camera->setCameraDevice(device);
    emit deviceChanged();

    rebuildFormats();
    applySelection(QSize(), QString(), 0.0, true);

    m_errorText.clear();
    emit errorTextChanged();
    setState(State::Starting);
    m_camera->start();
    saveSettings();
}

void CameraModel::setResolution(const QString &resolution)
{
    applySelection(parseResolution(resolution), m_pixelFormat, m_frameRate, true);
}

void CameraModel::setPixelFormat(const QString &pixelFormat)
{
    applySelection(parseResolution(m_resolution), pixelFormat, m_frameRate, true);
}

void CameraModel::setFrameRate(double fps)
{
    applySelection(parseResolution(m_resolution), m_pixelFormat, fps, true);
}

void CameraModel::takePhoto()
{
    if (!hasCamera() || !m_camera->isActive()) {
        const QString text = QStringLiteral("Camera is not running");
        emit photoFailed(text);
        emit message(text, true);
        return;
    }
    if (m_state == State::Recording) {
        const QString text = QStringLiteral("Stop recording before taking a photo");
        emit photoFailed(text);
        emit message(text, true);
        return;
    }
    if (!m_imageCapture->isReadyForCapture()) {
        const QString text = QStringLiteral("Camera is not ready yet");
        emit photoFailed(text);
        emit message(text, true);
        return;
    }

    if (m_timerSeconds > 0) {
        if (m_timerActive) {
            cancelTimer();
            return;
        }
        m_timerActive = true;
        m_timerRemaining = m_timerSeconds;
        emit timerRemainingChanged();
        emit timerActiveChanged();
        m_selfTimer.start();
        return;
    }

    captureNow();
}

void CameraModel::cancelTimer()
{
    if (!m_timerActive)
        return;
    m_timerActive = false;
    m_selfTimer.stop();
    m_timerRemaining = 0;
    emit timerRemainingChanged();
    emit timerActiveChanged();
}

void CameraModel::captureNow()
{
    m_imageCapture->capture();
}

void CameraModel::startRecording()
{
    if (!hasCamera() || !m_camera->isActive()) {
        const QString text = QStringLiteral("Camera is not running");
        emit videoFailed(text);
        emit message(text, true);
        return;
    }
    if (m_recordingStartPending)
        return;
    if (m_recorder->recorderState() != QMediaRecorder::StoppedState)
        return;
    if (m_state == State::Recording)
        return;

    QString error;
    if (!output::ensureDir(m_videosDir, &error)) {
        emit videoFailed(error);
        emit message(error, true);
        return;
    }

    cancelTimer();
    chooseRecordingFormat();

    m_recordFinalPath = output::uniqueMediaPath(m_videosDir, QStringLiteral("VID"), m_recordingExtension,
                                                QDateTime::currentDateTime(),
                                                [](const QString &candidate) {
                                                    return QFileInfo::exists(candidate)
                                                        || QFileInfo::exists(candidate + QStringLiteral(".part"));
                                                });
    m_recordPartPath = output::partPathFor(m_recordFinalPath);

    m_session.setAudioInput((m_micEnabled && m_micAvailable) ? m_audioInput : nullptr);
    m_recorder->setOutputLocation(QUrl::fromLocalFile(m_recordPartPath));
    m_recordingFinalizePending = false;
    m_recordingDuration = 0;
    emit recordingDurationChanged();
    m_recordingStartPending = true;
    m_recordStartTimer.start();
    m_recorder->record();
}

void CameraModel::stopRecording()
{
    if (m_recordingStartPending && m_recorder->recorderState() != QMediaRecorder::RecordingState) {
        failRecordingStart(m_recorder->errorString());
        return;
    }
    if (m_recorder->recorderState() != QMediaRecorder::RecordingState)
        return;
    m_recordingFinalizePending = true;
    m_recorder->stop();
}

void CameraModel::failRecordingStart(const QString &error)
{
    if (!m_recordingStartPending)
        return;
    m_recordingStartPending = false;
    m_recordStartTimer.stop();
    m_recordPartPath.clear();
    m_recordFinalPath.clear();
    const QString text = error.isEmpty() ? QStringLiteral("Recording failed to start") : error;
    emit videoFailed(text);
    emit message(text, true);
}

void CameraModel::openPhotosDir()
{
    QString error;
    if (!output::ensureDir(m_photosDir, &error)) {
        emit message(error, true);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_photosDir));
}

void CameraModel::openVideosDir()
{
    QString error;
    if (!output::ensureDir(m_videosDir, &error)) {
        emit message(error, true);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_videosDir));
}

void CameraModel::refreshDevices()
{
    if (m_recorder->recorderState() != QMediaRecorder::StoppedState) {
        emit message(QStringLiteral("Stop recording before scanning cameras"), true);
        return;
    }
    rebuildDevices(false);
}

void CameraModel::retry()
{
    if (!hasCamera() || m_camera->isActive())
        return;
    setErrorText(QString());
    setState(State::Starting);
    m_camera->start();
}

void CameraModel::attachVideoOutput(QObject *videoOutput)
{
    if (!videoOutput || videoOutput == m_session.videoOutput())
        return;
    m_session.setVideoOutput(videoOutput);
}

void CameraModel::notePreviewFrame()
{
    ++m_frameCount;
}

void CameraModel::shutdown()
{
    if (m_shuttingDown)
        return;
    m_shuttingDown = true;
    cancelTimer();
    m_recordStartTimer.stop();
    m_recordingStartPending = false;

    if (m_recorder && m_recorder->recorderState() == QMediaRecorder::RecordingState) {
        m_recordingFinalizePending = true;
        QEventLoop loop;
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(m_recorder, &QMediaRecorder::recorderStateChanged, &loop,
                [&loop](QMediaRecorder::RecorderState state) {
                    if (state == QMediaRecorder::StoppedState)
                        loop.quit();
                });
        m_recorder->stop();
        guard.start(5000);
        loop.exec();
        if (m_recordingFinalizePending)
            finalizeRecording(false);
    }

    if (m_camera)
        m_camera->stop();

    QThreadPool::globalInstance()->waitForDone(3000);
    saveSettings();
}

void CameraModel::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

void CameraModel::setErrorText(const QString &text)
{
    if (m_errorText == text)
        return;
    m_errorText = text;
    emit errorTextChanged();
}

void CameraModel::setStartupWarning(const QString &text)
{
    if (m_startupWarning == text)
        return;
    m_startupWarning = text;
    emit startupWarningChanged();
}

void CameraModel::rebuildDevices(bool initial)
{
    m_cameraDevices = QMediaDevices::videoInputs();

    m_devices.clear();
    for (const QCameraDevice &device : m_cameraDevices) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromUtf8(device.id()));
        entry.insert(QStringLiteral("description"), device.description());
        entry.insert(QStringLiteral("label"),
                     QStringLiteral("%1 (%2)").arg(device.description(), QString::fromUtf8(device.id())));
        m_devices.append(entry);
    }
    emit devicesChanged();

    if (m_cameraDevices.isEmpty()) {
        m_camera->stop();
        m_cameraDevice = QCameraDevice();
        m_deviceId.clear();
        m_deviceDescription.clear();
        emit deviceChanged();
        m_catalog.clear();
        rebuildFormats();
        setState(State::NoCamera);
        return;
    }

    const QString savedId = m_settings->value(kDeviceKey).toString();
    QCameraDevice chosen;
    for (const QCameraDevice &device : m_cameraDevices) {
        if (QString::fromUtf8(device.id()) == savedId) {
            chosen = device;
            break;
        }
    }
    if (chosen.isNull() && !initial && !m_deviceId.isEmpty()) {
        for (const QCameraDevice &device : m_cameraDevices) {
            if (QString::fromUtf8(device.id()) == m_deviceId) {
                chosen = device;
                break;
            }
        }
    }
    if (chosen.isNull())
        chosen = QMediaDevices::defaultVideoInput();
    if (chosen.isNull())
        chosen = m_cameraDevices.first();

    const QString chosenId = QString::fromUtf8(chosen.id());
    const bool changed = chosenId != m_deviceId;
    if (changed) {
        m_camera->stop();
        m_deviceId = chosenId;
        m_deviceDescription = chosen.description();
        m_cameraDevice = chosen;
        emit deviceChanged();
    }

    m_camera->setCameraDevice(chosen);
    rebuildFormats();

    if (initial || changed) {
        const QSize savedResolution = parseResolution(m_settings->value(kResolutionKey).toString());
        const QString savedPixelFormat = m_settings->value(kPixelFormatKey).toString();
        const double savedFrameRate = m_settings->value(kFrameRateKey, 0.0).toDouble();
        applySelection(savedResolution, savedPixelFormat, savedFrameRate, false);
        setState(State::Starting);
        m_camera->start();
    }
}

void CameraModel::rebuildFormats()
{
    m_resolutions.clear();
    if (m_cameraDevice.isNull()) {
        m_pixelFormats.clear();
        m_frameRates.clear();
        emit formatsChanged();
        return;
    }

    const QList<QCameraFormat> formats = m_cameraDevice.videoFormats();
    QList<catalog::Entry> raw;
    raw.reserve(formats.size());
    for (const QCameraFormat &format : formats) {
        catalog::Entry entry;
        entry.width = format.resolution().width();
        entry.height = format.resolution().height();
        entry.minFps = format.minFrameRate();
        entry.maxFps = format.maxFrameRate();
        entry.pixelFormat = static_cast<int>(format.pixelFormat());
        entry.pixelLabel = pixelFormatLabel(entry.pixelFormat);
        raw.append(entry);
    }
    m_catalog = catalog::normalize(raw);

    for (const QSize &size : catalog::resolutions(m_catalog)) {
        QVariantMap entry;
        entry.insert(QStringLiteral("value"), catalog::resolutionLabel(size));
        entry.insert(QStringLiteral("label"), catalog::resolutionLabel(size));
        entry.insert(QStringLiteral("width"), size.width());
        entry.insert(QStringLiteral("height"), size.height());
        m_resolutions.append(entry);
    }

    emit formatsChanged();
}

void CameraModel::applySelection(const QSize &resolution, const QString &pixelFormat, double frameRate, bool persist)
{
    if (m_catalog.isEmpty()) {
        if (!m_resolution.isEmpty()) {
            m_resolution.clear();
            m_pixelFormat.clear();
            m_frameRate = 0.0;
            emit selectionChanged();
        }
        return;
    }

    const QSize chosenResolution = catalog::nearestResolution(m_catalog, resolution);
    const QString chosenPixelFormat = catalog::nearestPixelLabel(m_catalog, chosenResolution, pixelFormat);
    const double chosenFrameRate = catalog::nearestFrameRate(m_catalog, chosenResolution, chosenPixelFormat, frameRate);

    const QString resolutionText = catalog::resolutionLabel(chosenResolution);
    const bool selectionChangedNow = resolutionText != m_resolution
        || chosenPixelFormat != m_pixelFormat
        || std::abs(chosenFrameRate - m_frameRate) > 0.01;

    m_resolution = resolutionText;
    m_pixelFormat = chosenPixelFormat;
    m_frameRate = chosenFrameRate;

    const QStringList newPixelFormats = catalog::pixelLabels(m_catalog, chosenResolution);
    QVariantList newFrameRates;
    for (double rate : catalog::frameRates(m_catalog, chosenResolution, chosenPixelFormat)) {
        QVariantMap entry;
        entry.insert(QStringLiteral("value"), rate);
        entry.insert(QStringLiteral("label"), QStringLiteral("%1 fps").arg(
            std::abs(rate - std::round(rate)) < 0.01 ? QString::number(static_cast<qint64>(std::llround(rate)))
                                                     : QString::number(rate, 'g', 3)));
        newFrameRates.append(entry);
    }

    const bool formatsChangedNow = newPixelFormats != m_pixelFormats || newFrameRates != m_frameRates;
    m_pixelFormats = newPixelFormats;
    m_frameRates = newFrameRates;

    if (selectionChangedNow)
        emit selectionChanged();
    if (formatsChangedNow)
        emit formatsChanged();

    applyCameraFormat();

    if (persist && m_initialized)
        saveSettings();
}

void CameraModel::applyCameraFormat()
{
    if (m_cameraDevice.isNull() || m_resolution.isEmpty())
        return;

    const QSize target = parseResolution(m_resolution);
    QCameraFormat best;
    double bestDistance = -1.0;
    for (const QCameraFormat &format : m_cameraDevice.videoFormats()) {
        if (format.resolution() != target)
            continue;
        if (pixelFormatLabel(static_cast<int>(format.pixelFormat())) != m_pixelFormat)
            continue;
        const double low = std::min(format.minFrameRate(), format.maxFrameRate());
        const double high = std::max(format.minFrameRate(), format.maxFrameRate());
        if (m_frameRate >= low - 0.01 && m_frameRate <= high + 0.01) {
            best = format;
            break;
        }
        const double distance = std::min(std::abs(m_frameRate - low), std::abs(m_frameRate - high));
        if (bestDistance < 0.0 || distance < bestDistance) {
            best = format;
            bestDistance = distance;
        }
    }

    if (!best.isNull())
        m_camera->setCameraFormat(best);

    updateActiveFormatLabel();
}

void CameraModel::updateActiveFormatLabel()
{
    const QCameraFormat active = m_camera->cameraFormat();
    QString label;
    if (active.resolution().isValid()) {
        label = catalog::formatLabel(active.resolution(), pixelFormatLabel(static_cast<int>(active.pixelFormat())),
                                     std::max(active.minFrameRate(), active.maxFrameRate()));
    } else if (!m_resolution.isEmpty()) {
        label = catalog::formatLabel(parseResolution(m_resolution), m_pixelFormat, m_frameRate);
    }

    if (label == m_activeFormatLabel)
        return;
    m_activeFormatLabel = label;
    emit activeFormatChanged();
}

void CameraModel::saveSettings()
{
    if (!m_settings || m_settings->readOnly())
        return;

    m_settings->setValue(kDeviceKey, m_deviceId);
    m_settings->setValue(kResolutionKey, m_resolution);
    m_settings->setValue(kPixelFormatKey, m_pixelFormat);
    m_settings->setValue(kFrameRateKey, m_frameRate);
    m_settings->setValue(kMirrorKey, m_mirror);
    m_settings->setValue(kMicKey, m_micEnabled);
    m_settings->setValue(kTimerKey, m_timerSeconds);
    m_settings->setValue(kPhotosDirKey, m_photosDir);
    m_settings->setValue(kVideosDirKey, m_videosDir);

    QString error;
    if (!m_settings->save(&error))
        setStartupWarning(error);
}

void CameraModel::chooseRecordingFormat()
{
    QMediaFormat probe;
    const QList<QMediaFormat::FileFormat> fileFormats = probe.supportedFileFormats(QMediaFormat::Encode);
    const QList<QMediaFormat::VideoCodec> videoCodecs = probe.supportedVideoCodecs(QMediaFormat::Encode);
    const QList<QMediaFormat::AudioCodec> audioCodecs = probe.supportedAudioCodecs(QMediaFormat::Encode);

    QMediaFormat format;
    if (fileFormats.contains(QMediaFormat::MPEG4)
        && videoCodecs.contains(QMediaFormat::VideoCodec::H264)) {
        format.setFileFormat(QMediaFormat::MPEG4);
        format.setVideoCodec(QMediaFormat::VideoCodec::H264);
        if (audioCodecs.contains(QMediaFormat::AudioCodec::AAC))
            format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
        else if (audioCodecs.contains(QMediaFormat::AudioCodec::MP3))
            format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
        m_recordingExtension = QStringLiteral("mp4");
    } else if (fileFormats.contains(QMediaFormat::WebM)
               && videoCodecs.contains(QMediaFormat::VideoCodec::VP9)) {
        format.setFileFormat(QMediaFormat::WebM);
        format.setVideoCodec(QMediaFormat::VideoCodec::VP9);
        if (audioCodecs.contains(QMediaFormat::AudioCodec::Opus))
            format.setAudioCodec(QMediaFormat::AudioCodec::Opus);
        else if (audioCodecs.contains(QMediaFormat::AudioCodec::Vorbis))
            format.setAudioCodec(QMediaFormat::AudioCodec::Vorbis);
        m_recordingExtension = QStringLiteral("webm");
    } else {
        format.setFileFormat(QMediaFormat::MPEG4);
        m_recordingExtension = QStringLiteral("mp4");
    }

    m_recorder->setMediaFormat(format);
    m_recorder->setQuality(QMediaRecorder::HighQuality);
}

void CameraModel::finalizeRecording(bool success)
{
    m_recordingFinalizePending = false;

    const QString finalPath = m_recordFinalPath;
    const QString partPath = m_recordPartPath;
    m_recordFinalPath.clear();
    m_recordPartPath.clear();

    QString produced = m_recorder->actualLocation().toLocalFile();
    if (produced.isEmpty())
        produced = partPath;

    const bool renamed = success && !produced.isEmpty() && !finalPath.isEmpty()
        && QFileInfo::exists(produced) && QFile::rename(produced, finalPath);

    if (renamed) {
        m_lastVideoPath = finalPath;
        emit lastVideoPathChanged();
        emit videoSaved(finalPath);
        emit message(QStringLiteral("Video saved"), false);
    } else {
        const QString text = m_recorder->errorString().isEmpty()
            ? QStringLiteral("Recording failed")
            : m_recorder->errorString();
        emit videoFailed(text);
        emit message(text, true);
    }

    if (m_recordingDuration != 0) {
        m_recordingDuration = 0;
        emit recordingDurationChanged();
    }

    setState(m_camera && m_camera->isActive() ? State::Streaming : State::Error);
}

QString CameraModel::pixelFormatLabel(int pixelFormat) const
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QVideoFrameFormat::PixelFormat>();
    QString name = QString::fromLatin1(metaEnum.valueToKey(pixelFormat) ? metaEnum.valueToKey(pixelFormat) : "");
    if (name.startsWith(QLatin1String("Format_")))
        name.remove(0, 7);
    static const QHash<QString, QString> overrides = {
        {QStringLiteral("Jpeg"), QStringLiteral("MJPEG")},
        {QStringLiteral("Y8"), QStringLiteral("GRAY8")},
        {QStringLiteral("Y16"), QStringLiteral("GRAY16")},
    };
    return overrides.value(name, name.isEmpty() ? QStringLiteral("Unknown") : name);
}

QSize CameraModel::parseResolution(const QString &text)
{
    const QStringList parts = text.split(QLatin1Char('x'));
    if (parts.size() != 2)
        return QSize();
    bool widthOk = false;
    bool heightOk = false;
    const int width = parts.at(0).toInt(&widthOk);
    const int height = parts.at(1).toInt(&heightOk);
    if (!widthOk || !heightOk || width <= 0 || height <= 0)
        return QSize();
    return QSize(width, height);
}

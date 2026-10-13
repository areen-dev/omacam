# Architecture

Status: implemented for the first vertical slice. This records the boundaries
the code actually uses; it is not a demand for a fixed folder hierarchy.

## Stack and rationale

C++20 with Qt 6.11 Quick (QML) and Qt Multimedia. Requirements behind the
choice: enumerate real camera resolutions, pixel formats and frame rates,
render a Wayland-native GPU preview, capture stills and record video with audio,
and keep the app small. Qt Multimedia exposes all of these directly
(`QCameraFormat`, `QImageCapture`, `QMediaRecorder`) with Qt 6 already present on
the target Omarchy installation. The FFmpeg backend is what Qt Multimedia loads
here; its GPL licensing is recorded in CREDITS.md.

## Boundaries and dependency direction

| Responsibility | Owns | Location |
| --- | --- | --- |
| Startup | App identity, command-line parsing, self-test harness, QML engine, context properties, process lifetime | `src/main.cpp` |
| Application/session | Camera device and format selection, capture/record state machine, error text, measured fps | `src/camera/CameraModel.{h,cpp}` |
| Domain/core | Format catalog build/query/selection math; settings parse/serialize/version; output naming and atomic directories; theme output parsing | `src/camera/CatalogFormat.cpp`, `src/config/SettingsStore.cpp`, `src/config/OutputPaths.cpp`, `src/theme/ThemeParser.cpp` |
| UI | Window, preview, control bar, settings panel, toasts, shortcuts; transient UI state only | `src/qml/*.qml` |
| Desktop/storage adapters | Omarchy theme process and file watching; XDG paths; settings file | `src/theme/ThemePalette.{h,cpp}`, `src/config/*` |

Dependency direction: core files depend only on Qt Core. `CameraModel` and
`ThemePalette` depend on core and on Qt Multimedia/Quick-adjacent services. QML
depends on both context properties (`CameraModel`, `Theme`) and reads no Qt
Multimedia C++ types directly. Core is unit-tested without a GUI
(`camera-core` static library, `tests/tst_*.cpp`).

## State and resource owners

- `CameraModel` owns `QCamera`, `QImageCapture`, `QMediaRecorder`,
  `QAudioInput` and the `QMediaCaptureSession`, plus the device/format catalog,
  the recording paths and the self-timer countdown. The countdown is cancelled
  on device switch and shutdown, and on expiry it captures through the normal
  image capture path. The QML preview attaches its `VideoOutput` sink
  through `CameraModel::attachVideoOutput`, so the session always has exactly
  one video output; the `--ui-check` mode guards that wiring by reporting
  measured preview frames per second.
- `SettingsStore` owns the settings file; writes go through `QSaveFile` and are
  versioned by `schema_version`. Malformed input is preserved as a backup and
  the app continues with defaults. A newer schema is loaded read-only. When the
  current file is missing, settings load from the pre-rename
  `~/.config/dev.areen.camera/camera.conf` path and the original file is kept.
- `ThemePalette` owns the `omarchy-theme-color` `QProcess` and a
  `QFileSystemWatcher` on `~/.local/state/omarchy/current/theme`; palette
  identity is cached, malformed output falls back to built-in colors.
- `OutputPaths` owns file naming (`IMG_/VID_` + timestamp, collision suffix) and
  directory creation.
- QML owns transient UI state (panel visibility, toast text). It never owns
  authoritative format or recording state.

Stale-result rejection: device and format changes are only accepted while the
recorder is stopped; the camera is stopped before a device switch. Photo saves
run on `QtConcurrent` workers that cannot be cancelled once started, so shutdown
waits for them (bounded 3 s). A recording is finalized through the recorder's
`StoppedState` signal and only then renamed from `*.part` to its final name; a
failure keeps the partial file rather than silently deleting it.

## Lifecycle

- Startup: load settings, enumerate devices, choose the saved device or the
  default, build the format catalog, validate the saved selection (nearest
  supported resolution/pixel format/frame rate), apply it and start the camera.
- Device switch: recording is refused while active; camera stops, catalog
  rebuilds, selection is re-validated and persisted.
- Close: `shutdown()` stops the recorder, waits up to 5 s for finalization and
  finalizes the rename, stops the camera, waits for pending photo saves and
  writes settings atomically. Window close reaches this path through the
  `CameraModel` destructor after the event loop returns. Error paths keep
  partial files and never leave a stale lock.
- Focus loss: preview and recording continue; shortcuts are disabled while the
  settings panel has an active text field so typing never triggers capture.
- Failure: camera errors stop the camera and switch to an explicit error state
  with a Retry action; a missing device switches to `NoCamera` with a rescan
  action. Capture calls made while the camera is not streaming fail with a
  message instead of hanging.

## Known backend behavior

- `QImageCapture::captureToFile` returns `ResourceError` ("Cannot open device
  for writing") on the FFmpeg backend here; capture uses `capture()` plus an
  asynchronous `QImageWriter` save instead.
- The camera reports 30 fps formats but delivers about 20 fps; the model counts
  preview frames and shows a measured rate instead of trusting the format.
- The desktop portal registration warning that appeared at startup before the
  identity fix is gone; folder opening uses `QDesktopServices` and was
  exercised in an earlier run (see [VERIFICATION.md](VERIFICATION.md)).

## Verification

Core regressions are in [VERIFICATION.md](VERIFICATION.md): format catalog,
settings corruption/versioning, output naming, theme parsing, plus the camera
self-test and the quit-during-recording lifecycle check. Live desktop items
(icon rendering, a full `omarchy theme set` switch, portal folder selection)
remain explicitly pending.

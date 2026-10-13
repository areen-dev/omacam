# Omacam

<img alt="Built for Omarchy: App" height="20" src="https://raw.githubusercontent.com/tcballard/omarchy-badges/75975e5b5bf75e7ede3764bcd2950046f7abfe2c/badges/v1/omarchy-app.svg">

A native webcam app for Omarchy: live preview, photo capture and video recording
with device, resolution, pixel-format and frame-rate settings taken from the
camera's own reported capabilities.

Status: implemented vertical slice; unit tests, automated camera self-test and
local install/uninstall reproduced on Omarchy 4.0.4-1 (see
[verification](VERIFICATION.md)). The app launches through the desktop entry
with the expected Wayland app_id, and preview frame delivery is checked by the
automated `--ui-check` mode. The high-severity defects from the first audit
(recording start, error banner, timer during recording, portal app ID) are
fixed and verified on 2026-10-13. Icon rendering, a full live
`omarchy theme set` switch and a README screenshot remain pending (the
screenshot was deliberately omitted for privacy).
Version 0.1.1 still has open medium- and low-severity items; review
[known issues](KNOWN-ISSUES.md) before you publish or deploy this app.
Intended Omarchy target: 4 / Hyprland; this is the intended target, not a tested
compatibility range.
Tested Omarchy versions: 4.0.4-1 on Hyprland (Wayland), Qt 6.11.2, GCC 16.2.1.
Renamed from `dev.areen.camera` to `dev.areen.omacam`; existing settings migrate on first start.

## Features

- Live preview, mirrored by default (toggleable).
- Photo capture to `IMG_<timestamp>.jpg`, default `~/Pictures/Camera`.
- Video recording to `VID_<timestamp>.mp4` (H.264 + AAC, WebM/VP9 fallback) with
  a microphone toggle and a recording timer.
- Self-timer with Off, 3 s and 10 s delays and a countdown overlay; tap the
  countdown or press the shutter key again to cancel. Press T to cycle the
  timer 3 s, 10 s, off.
- The window opens on the camera preview; the settings panel is closed by
  default and opens with S or the Settings button.
- Settings: camera device, resolution, pixel format, frame rate, timer, mirror,
  microphone, photo and video folders; applied live and remembered.
- Capability-driven options: only resolutions, pixel formats and frame rates
  the device reports; saved selections are validated at startup and on device
  changes, falling back to the nearest supported format.
- Recording writes `*.part` and renames to the final name only after the encoder
  finalizes; a failed recording keeps the partial file.
- Settings are written atomically with a schema version; a malformed file is
  preserved as `camera.conf.corrupt-<timestamp>` instead of being overwritten.
- Read-only Omarchy theme palette consumption via `omarchy-theme-color`, with a
  built-in fallback palette when the theme output is missing or malformed.

## Keyboard

Space photo · R record/stop · P open photos folder · V open videos folder · T cycle timer (3 s, 10 s, off) · M mirror · S settings · F11 fullscreen · Esc leave fullscreen

## Build and run

Build dependencies: cmake, ninja, qt6-base, qt6-declarative, qt6-multimedia,
qt6-wayland.

```sh
omarchy pkg add cmake ninja qt6-base qt6-declarative qt6-multimedia qt6-wayland
cmake -S . -B target -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build target
ctest --test-dir target --output-on-failure
./target/dev.areen.omacam
```

Automated camera self-test (no UI; drives the real capture code paths and writes
evidence into a directory you choose):

```sh
./target/dev.areen.omacam --self-test /tmp/camera-check --format 1280x720 --seconds 5
```

Self-test options: `--format <WxH>`, `--seconds <n>`, `--timer-seconds <3|10>`,
`--cycle-timer`, `--quit-during-record`, and `--record-during-timer` (starts a
recording while the self-timer counts down and checks that no photo is saved
during recording). `--ui-check <seconds>` loads the UI and reports measured
preview fps, camera state, settings visibility, error-banner visibility and the
active format; `--rescan-every <seconds>` retries camera recovery while the UI
is open. Self-test and ui-check exit 0 on success and 2 on failure; `--ui-check`
exits 3 when no frames were measured (for example with the camera busy). The
self-test does not write the settings file.

## Install and rollback

```sh
./install.sh              # builds Release, runs tests, installs to ~/.local
./install.sh --uninstall  # removes app files; keeps settings and media
```

The installer places the binary in `~/.local/bin`, the desktop entry in
`~/.local/share/applications` and the icon in
`~/.local/share/icons/hicolor/scalable/apps`, using the same
`dev.areen.omacam` identity for the executable, Wayland app_id, desktop file
and icon. Rollback is `--uninstall` plus restoring a previous binary or commit.

Kept on uninstall: `~/.config/dev.areen.omacam/camera.conf`,
`~/Pictures/Camera`, `~/Videos/Camera` (or the configured folders).

## Limitations

- Photo capture uses the in-memory frame path because `captureToFile` fails on
  the Qt FFmpeg backend on this system (recorded in
  [verification](VERIFICATION.md)).
- This webcam advertises 30 fps but delivers about 20 fps; the UI shows both the
  requested format and a measured fps readout.
- Folder paths are edited as text; there is no portal folder picker yet.
- The Qt Multimedia FFmpeg backend reports GPL licensing; see [credits](CREDITS.md).

## Evidence

See [verification](VERIFICATION.md) and [credits](CREDITS.md). Architecture is
documented in [ARCHITECTURE.md](ARCHITECTURE.md); the source is [MIT](LICENSE)
licensed. This community badge does not imply official acceptance.

# Verification

Input identity: source and evidence files recorded in
[EVIDENCE.sha256](EVIDENCE.sha256) (generated with the Omarchy app helper
`snapshot`; build outputs under `target/` are excluded by the helper).
Upstream revision: not applicable; original project, no upstream revision.
Toolchain/platform: Omarchy 4.0.4-1, Hyprland/Wayland, Linux 7.2.5-3-omarchy,
GCC 16.2.1, CMake 4.4.3, Ninja 1.13.2, Qt 6.11.2, FFmpeg n9.0.1.
Camera: Integrated_Webcam_FHD (UVC, `/dev/video0` capture node).

Commands below were run from the project root unless noted. Raw logs are stored
under `evidence/acceptance/` and `evidence/m1/` and are covered by the input
manifest.

Rename note: the application was renamed from `dev.areen.camera` to
`dev.areen.omacam` (display name `Omacam`) on 2026-09-23. Table rows that name
`dev.areen.camera` are historical evidence recorded before the rename and keep
their original input identity. The post-rename checks are the rename rows
below.

## Reproduced now

| Check | Command | Result |
| --- | --- | --- |
| Release build | `cmake -S . -B target -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build target` | exit 0, executable `target/dev.areen.camera` |
| Unit tests | `ctest --test-dir target --output-on-failure` | exit 0, 4/4 tests passed (`evidence/acceptance/ctest.log`) |
| Self-test 1280x720 | `dev.areen.camera --self-test <dir> --format 1280x720 --seconds 5` | exit 0, `photo=ok video=ok` (`evidence/acceptance/selftest-1280x720.log`) |
| Self-test 1920x1080 | `dev.areen.camera --self-test <dir> --format 1920x1080 --seconds 5` | exit 0, `photo=ok video=ok` (`evidence/acceptance/selftest-1920x1080.log`) |
| Quit during recording | `dev.areen.camera --self-test <dir> --format 1280x720 --seconds 4 --quit-during-record` | exit 0; recorder finalized during shutdown, `postShutdown exists=true`, MP4 playable (`evidence/acceptance/quit-during-record.log`) |
| GUI smoke run | `dev.areen.camera` under Wayland, 6 s, stderr captured | no QML errors; theme palette loaded from `omarchy-theme-color` (dark `#090909`, accent `#808080`) (`evidence/acceptance/gui-smoke.log`) |
| Local install | `./install.sh` via `omarchy pkg`-installed toolchain equivalents | exit 0; binary, desktop entry and icon installed under `~/.local`; `ctest` ran as part of the install (`evidence/acceptance/install.log`) |
| Install over existing (upgrade) | second `./install.sh` run | exit 0; files replaced, installed desktop entry byte-identical to `packaging/` (`evidence/acceptance/install-upgrade.log`) |
| Uninstall | `./install.sh --uninstall` | exit 0; binary, desktop entry and icon removed; settings and media paths reported as kept (`evidence/acceptance/uninstall.log`) |
| Launcher and app identity | `gtk-launch dev.areen.camera`, then `hyprctl clients` | app process started; Hyprland reports `class: dev.areen.camera`, `title: Camera`; palette loaded in the launched instance (`evidence/acceptance/launcher.log`) |
| Theme reload while open | app running, `touch ~/.local/state/omarchy/current/theme/colors.toml` | watcher fired and the palette was re-queried; two palette loads in the log (`evidence/acceptance/theme-reload.log`) |
| QML lint | `qmllint -I /usr/lib/qt6/qml src/qml/*.qml` | exit 0; warnings are only "unqualified access" for the `CameraModel`/`Theme` context properties, no syntax or type errors (`evidence/acceptance/qmllint.txt`) |
| Camera busy | `ffmpeg` holding `/dev/video0`, then `--self-test --format 640x480` | app reported `state Camera error`, photo and video failed with messages, exit 2, no hang (`evidence/acceptance/camera-busy.log`) |
| Regression re-run | `--self-test --format 1280x720 --seconds 3` after the camera-error fix | exit 0, `photo=ok video=ok` (`evidence/acceptance/selftest-regression-1280x720.log`) |
| Preview frame delivery | `dev.areen.camera --ui-check 6` (loads the UI, reports after 6 s) | exit 0, `measuredFps=21 state=Streaming settingsVisible=false format=1920x1080 · MJPEG · 30 fps` (`evidence/acceptance/ui-check.log`) |
| Self-timer on | `--self-test --timer-seconds 3` | exit 0; `timerCancel wasActive=true activeAfter=false`; `timerElapsed ms=3007 expected=3000`; `result photo=ok video=ok timer=ok` (`evidence/acceptance/timer-on.log`) |
| Self-timer off | `--self-test` without `--timer-seconds` | exit 0; `result photo=ok video=ok timer=ok` (`evidence/acceptance/timer-off.log`) |
| Timer cycle logic | `--self-test --cycle-timer` | exit 0; `timerCycle 3,10,0,3`; timed capture with the cycled 3 s value at `timerElapsed ms=3056 expected=3000`; `result ... timer=ok cycle=ok` (`evidence/acceptance/timer-cycle.log`) |
| T key | `wtype t` three times, focus checked on the app before each press | timer setting went 3, then 10, then 0 (`evidence/acceptance/timer-key.txt`) |
| Default screen | `--ui-check 6` | exit 0; `settingsVisible=false`, so the window opens on the camera preview (`evidence/acceptance/ui-check-default-screen.log`) |
| Capture and folder keys | `wtype ' '`, `wtype r` twice, then `wtype p` and `wtype v`, with the app window focused | Space saved a 640x480 JPEG; R,R saved a 640x480 H.264 MP4; P opened the file manager on the photos folder and V on the videos folder (`evidence/acceptance/shortcut-keys.txt`, `folder-keys.log`) |
| Desktop entry | `desktop-file-validate packaging/dev.areen.camera.desktop` | exit 0; one non-blocking hint about extending `Photography` (`evidence/acceptance/desktop-validate.txt`) |
| Artifact inspection | `ffprobe` on produced videos, `identify` on produced photos | 1280x720 and 1920x1080 JPEG photos; H.264 + AAC MP4 videos at the captured resolution, about 20 fps delivered |
| Rename build | configure and build after the `dev.areen.omacam` rename | exit 0; executable `target/dev.areen.omacam`; `ctest` 4/4 including the new migration test |
| Legacy settings migration | legacy `dev.areen.camera` config plus `--ui-check 4` | exit 0; new config written at `dev.areen.omacam` with the legacy values (`640x480`, timer 3, custom folders); legacy file preserved (`evidence/acceptance/rename-migration.log`) |
| Post-rename self-test | `dev.areen.omacam --self-test <dir> --format 1280x720 --seconds 3` | exit 0; `result photo=ok video=ok timer=ok cycle=ok` (`evidence/acceptance/selftest-post-rename.log`) |
| Post-rename launcher identity | install the renamed app, then `gtk-launch dev.areen.omacam` | Hyprland reports `class: dev.areen.omacam`, `title: Omacam`; old `dev.areen.camera` install removed (`evidence/acceptance/rename-launcher.txt`) |

Observed capture behavior in the reproduced self-tests:

- Device enumeration returned `/dev/video0` (`Integrated_Webcam_FHD`) with
  MJPEG 640x360/640x480/1280x720/1280x960/1920x1080 all reporting 30 fps, YUYV
  640x360/640x480 at 30 fps, 1280x720 at 10 fps and 1920x1080 at 5 fps, and a
  second node `/dev/video2` reporting a single 360x360 GRAY format.
- Format switching to 1280x720 MJPEG and 1920x1080 MJPEG was applied and
  reflected in the active-format label.
- Recordings deliver about 20 fps although the device advertises 30 fps; the
  same rate is measured with `ffmpeg -f v4l2` directly, so the limit is the
  camera, not the encoder. The UI reports the requested format plus a measured
  fps readout.
- Settings for both self-test runs were isolated under a temporary
  `XDG_CONFIG_HOME`; the file was written with `schema_version=1` and quoted
  paths.

## Final verification run (2026-10-13)

Full re-run on the renamed application (`dev.areen.omacam`, display name
`Omacam`) with a fresh build directory. Logs: `evidence/final/`.

| Check | Result |
| --- | --- |
| Clean configure and build | exit 0 (`configure.log`, `build.log`) |
| Unit tests | 4/4 pass (`ctest.log`) |
| QML lint | exit 0, unqualified-access warnings only (`qmllint.txt`) |
| Desktop entry | `desktop-file-validate` exit 0 (`desktop-validate.txt`) |
| Scaffold contract | `check`, `snapshot` and `verify` pass |
| Self-test 1280x720 and 1920x1080 | exit 0, photo and video saved (`selftest-1280x720.log`, `selftest-1920x1080.log`, `media-inspection.txt`) |
| Timer cycle and cancel | exit 0, `timerCycle 3,10,0,3`, cancel ok, `timerElapsed ms=3049` (`cycle.log`) |
| Preview and default screen | exit 0, 20 fps, state Streaming, `settingsVisible=false` (`ui-check.log`) |
| Quit during recording | exit 0, file finalized and playable (`quit-record.log`) |
| Camera busy | exit 2, graceful failures, no hang (`busy.log`) |
| Theme reload while open | two palette loads after touching the theme file (`theme.log`) |
| Install upgrade and launcher | exit 0; Hyprland `class=dev.areen.omacam`, `title=Omacam` (`install.log`, `launcher.txt`) |

Open defects carried by this verification are recorded in
[KNOWN-ISSUES.md](KNOWN-ISSUES.md) with evidence under
`evidence/known-issues/`. The recording-start failure and the invisible error
banner were re-confirmed on this build; the timer-during-recording check was
aborted safely in the final pass because another window held focus, so it
remains confirmed by the earlier run only.

## Fix verification run (2026-10-13, after the high-defect fixes)

High-severity defects 1-4 from [KNOWN-ISSUES.md](KNOWN-ISSUES.md) were fixed
and this run re-verified them on the same machine and camera. Logs:
`evidence/fix/`.

| Check | Result |
| --- | --- |
| Clean configure and build | exit 0 in a fresh directory (`configure.log`, `build.log`) |
| Unit tests | 4/4 pass (`ctest.log`) |
| QML lint | exit 0, unqualified-access warnings only (`qmllint.txt`) |
| Desktop entry | `desktop-file-validate` exit 0 (`desktop-validate.txt`) |
| Self-test 1280x720 and 1920x1080 | exit 0, photo and video saved (`selftest-1280x720.log`, `selftest-1920x1080.log`, `media-inspection.txt`) |
| H1 recording start failure (non-writable videos folder) | exit 2; `videoFailed` and an error message in about 2 s, no watchdog wait (`record-fail.log`) |
| H3 timer during recording | exit 0, `result timerDuringRecord=ok video=ok`, no photo saved during the recording (`timer-during-record.log`) |
| Timer cycle and cancel | exit 0, `timerCycle 3,10,0,3`, cancel ok, `timerElapsed ms=3051` (`cycle.log`) |
| Quit during recording | exit 0, file finalized and playable (`quit-record.log`) |
| Preview and default screen | exit 0, 20 fps, `state=Streaming`, `errorBanner=false` (`ui-check.log`) |
| H2 error banner with busy camera | `state=Camera error`, `errorBanner=true` (exit 3 because no frames are measured; expected with a busy camera) (`ui-check-busy.log`) |
| Busy camera self-test | exit 2, graceful failures, no hang (`camera-busy.log`) |
| H4 portal registration | no "register app ID" warning in any fix-run log (`ui-check.log`, `ui-check-busy.log`) |
| Theme reload while open | two palette loads after touching the theme file (`theme.log`) |
| Install and launcher | install exit 0; Hyprland `class=dev.areen.omacam`, `title=Omacam` on the installed binary (`install.log`, `launcher.txt`) |
| Scaffold contract | `check`, `snapshot` and `verify` pass |

The four high defects were closed; the remaining items are statused in the
second fix verification run below.

## Second fix verification run (2026-10-13)

Medium/low items 5-9, 11, 12, 15 and 17 were fixed and re-verified in a second
pass. Logs: `evidence/fix2/`.

| Check | Result |
| --- | --- |
| Unit tests | 4/4 pass, including the new collision-fallback test (`ctest.log`) |
| QML lint | exit 0, unqualified-access warnings only (`qmllint.txt`) |
| Self-test 1280x720 | exit 0, photo and video saved (`selftest-1280x720.log`) |
| Self-test settings isolation | fresh config: no settings file created; existing config with custom folders: byte-identical after the run (`settings-session.txt`, `selftest-session-fresh.log`, `selftest-session-custom.log`) |
| Same-camera recovery | busy camera released while `--rescan-every 3` was active: `state=Streaming`, 20 fps, no restart (`rescan-recovery.log`) |
| Preview and error banner | exit 0, `state=Streaming`, `errorBanner=false` (`ui-check.log`) |

Items 5-9, 11, 12, 15 and 17 are closed; items 10, 13, 14 and 16 remain open.
The device-combo resync, timer-combo resync, control disablement and folder
open failure paths are covered by inspection and lint; live interaction checks
were not run because the controls are not keyboard-reachable (item 16).

## Third fix verification run (2026-10-13)

Items 10, 13 and 16 were fixed and re-verified in a third pass. Logs:
`evidence/fix3/`.

| Check | Result |
| --- | --- |
| Unit tests | 4/4 pass (`ctest.log`) |
| QML lint | exit 0, unqualified-access warnings only (`qmllint.txt`) |
| Self-test 1280x720 | exit 0, photo and video saved (`selftest-1280x720.log`) |
| Error-state format chip | busy camera: `state=Camera error`, `errorBanner=true`, empty `format=` (exit 3 because no frames are measured, expected) (`ui-check-busy.log`) |
| Preview and defaults | exit 0, `state=Streaming`, format reported (`ui-check.log`) |
| Theme helper timeout | helper killed after 5 s (6 heartbeats instead of 12), fallback palette kept, app exits cleanly (`theme-timeout.log`) |
| Late theme state directory | directory created while the app was open: two palette loads (`theme-late-dir.log`) |
| Accessibility traversal | synthetic Tab steps 1-6 reach Mirror, shutter, Record, Settings, Photos, Videos; live keys reach the app (`a11y-tab.txt`, `a11y-tab-wide.log`, `tab-steps/`, `a11y-live-keys.log`) |

Items 10, 13 and 16 are closed; item 14 (frame-rate list) remains open and
needs a product decision. The tab traversal result also unblocks the live
interaction checks for items 7-9 in a later pass.

## Live acceptance pass (2026-10-13)

| Check | Result |
| --- | --- |
| Controls disabled during recording (item 7) | while recording, the device/resolution/pixel-format/frame-rate combos and the microphone switch drop out of the Tab chain; Timer, mirror and the folder fields remain; the session recording is saved (`controls-recording.log`, `controls-baseline.log`) |
| Terminal start | installed v0.1.2 binary launched from a shell; window `class=dev.areen.omacam`, `title=Omacam` (`terminal-spaces.log`, `terminal-window.txt`) |
| Paths with spaces | the Space shortcut saved a photo into `space photos` (`terminal-spaces.log`) |
| Corrupt settings | original preserved as `camera.conf.corrupt-*`, defaults rewritten, exit 0 (`corruption.log`) |
| Newer schema | config kept byte-identical (read-only), exit 0 (`newer-schema.log`) |
| Unwritable settings directory | app ran, config kept byte-identical, exit 0 (`readonly-config.log`) |

The recording-control check used the instrumented build with `--tab-check`
traces; the remaining checks used the installed v0.1.2 binary. Still not run:
icon rendering, a full `omarchy theme set` switch, multi-monitor placement,
HiDPI scaling, IME/paste, camera-disconnect during streaming and the "open
photos/videos folder" behavior on the renamed build.

## Historical

The M1 risk spike logs (`evidence/m1/run2.log`, `run3.log`, `run1080.log`) were
produced by pre-M2 probe code that no longer exists in the tree; keep them
associated with that earlier input identity. They established: Qt FFmpeg backend
camera access, format enumeration and switching, `QImageCapture::captureToFile`
failure, in-memory capture success, and H.264 + AAC recording at 640x480 and
1920x1080. `evidence/m1/rec*.txt` are ffprobe summaries of those recordings.

## Failed

- `QImageCapture::captureToFile` on the FFmpeg backend returns `ResourceError`
  ("Cannot open device for writing: No such file or directory") for a writable
  path. Reproduced with both 640x480 and 1920x1080. Workaround: `capture()` in
  memory plus an asynchronous `QImageWriter` save, which is what the app uses.
- `Failed to open VDPAU backend libvdpau_nvidia.so` on stderr: harmless
  hardware-decode probe noise on this machine; playback is unaffected.
- Portal registration warning at startup
  (`Could not register app ID: Connection already associated with an
  application ID`). The app identity causes this (KNOWN-ISSUES item 4); portal
  features can be affected.
- The first live GUI capture showed a black preview with 0 measured fps: the
  QML `VideoOutput` sink was never attached to the C++ capture session, so the
  camera streamed but nothing rendered. Fixed with
  `CameraModel::attachVideoOutput`; the new `--ui-check` mode guards it. The
  bug was invisible to the capture self-test, which uses the C++ session
  directly.
- The same capture exposed clipped combo boxes and an overflowing control bar
  at narrow tiled widths. Fixed with `Layout.fillWidth` on the setting combos
  and responsive visibility in the control bar.
- Captures of an unfocused window show the desktop behind it (Omarchy window
  transparency), which looked like ghosted text; focusing the window before
  capture removes it. This is compositor behavior, not an app defect.
- The first folder-key test showed no file manager window. The test shell's
  `PATH` put a mise-installed Python before the system Python, and Nautilus
  failed its `gobject` import and exited. The same test with a system `PATH`
  passed. This is a test-environment condition, not an app fault.
- One initial unit-test expectation was wrong (25 fps is a valid standard rate
  within the camera range); the test was corrected to expect 24 fps for a
  23 fps request. No product code was changed for this.

## Not run

- Package build, install, upgrade and removal as a pacman package (no PKGBUILD;
  packaging is deliberately deferred). The local `./install.sh` lifecycle was
  reproduced; see the table above.
- Live desktop acceptance beyond launcher identity: icon rendering in the
  launcher/menu, a README screenshot (deliberately skipped by the user; no
  media is published with this tree), a full `omarchy theme set` switch while
  the app is open (file-change reload is reproduced above; terminal start and
  paths with spaces are covered by the live acceptance pass below), "Open
  photos/videos folder" behavior on the renamed build (an earlier run passed
  it; see the Capture and folder keys row), multi-monitor placement, HiDPI
  scaling, and IME/paste behavior.
- Camera-disconnect during streaming (the settings-corruption and read-only
  paths in the running app are covered by the live acceptance pass below; a
  busy camera is covered above).

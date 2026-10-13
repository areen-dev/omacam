# Known issues

Status of this list: defects found by a code and live-test audit on 2026-09-21
and re-checked during the final verification on 2026-10-13. Items 1-4 were
fixed and re-verified in the first fix pass; items 5-9, 11, 12, 15 and 17 were
fixed in a second pass; items 10, 13 and 16 were fixed in a third pass; item 14
was closed as documented (no code change possible). Evidence: first pass under
`evidence/fix/`, second pass under `evidence/fix2/`, third pass under
`evidence/fix3/`, live acceptance under `evidence/acceptance-2/` and
`evidence/acceptance-3/`, audit files under `evidence/known-issues/`.

## High (fixed 2026-10-13)

1. **Fixed: a failed recording start left the app stuck in "Recording".**
   The recorder must confirm the start before the app enters the Recording
   state; a failure while starting surfaces the recorder error, and a Stop
   request during a pending start reports the failure. A non-writable videos
   folder now reports `videoFailed` and `video=fail` in about two seconds with
   no watchdog wait (`evidence/fix/record-fail.log`). Location:
   `src/camera/CameraModel.cpp` `startRecording()`, `stopRecording()`,
   `failRecordingStart()`.

2. **Fixed: the error banner never displayed, so the Retry action was
   unreachable.** `CameraModel` now exposes a `hasError` property (QML cannot
   read enum values through a context-property instance) and the banner binds
   to it. A busy-camera run reports
   `state=Camera error ... errorBanner=true` (`evidence/fix/ui-check-busy.log`).
   Location: `src/qml/PreviewPane.qml`, `src/camera/CameraModel.h`.

3. **Fixed: a running self-timer could capture a photo during a recording.**
   Starting a recording now cancels the countdown, and countdown expiry
   refuses to capture while the recorder is running. `--record-during-timer`
   verifies that no photo is saved during a recording
   (`evidence/fix/timer-during-record.log`). Location:
   `src/camera/CameraModel.cpp` timer expiry, `startRecording()`.

4. **Fixed: portal app-ID registration failed at every start.** The
   application identity is now set before the `QGuiApplication` object is
   constructed, and the portal warning no longer appears in start logs
   (`evidence/fix/ui-check-busy.log`). Location: `src/main.cpp`.

## Medium (fixed 2026-10-13)

5. **Fixed: no recovery for the same camera after an error.**
   `refreshDevices()` now retries the camera when the device is unchanged and
   the app is in the error state. A busy-camera run whose holder is released
   recovers to `state=Streaming` without an app restart
   (`evidence/fix2/rescan-recovery.log`). The same recovery was reproduced
   after the camera's device nodes disappeared and returned (USB
   unbind/rebind) in `evidence/acceptance-4/usb-recovery.log`.
6. **Fixed: `--self-test` wrote the temporary folders into the user settings
   file.** Self-test mode no longer writes the settings file at all: a fresh
   `XDG_CONFIG_HOME` gets no file and an existing config stays byte-identical
   (`evidence/fix2/settings-session.txt`, `selftest-session-fresh.log`,
   `selftest-session-custom.log`).
7. **Fixed: format, device and microphone controls stayed enabled during
   recording.** The combos and the microphone switch are disabled while
   recording, and the format setters refuse with a message. Live-verified:
   with recording active the disabled controls drop out of the Tab chain
   while Timer, mirror and the folder fields remain
   (`evidence/acceptance-2/controls-recording.log`, baseline
   `controls-baseline.log`).
8. **Fixed: the device combo desynchronized when a switch was refused.** The
   settings panel re-syncs the combos after a device selection. Covered by
   inspection and `qmllint`.
9. **Fixed: the Timer combo did not follow the T key.** The panel re-syncs on
   `timerSecondsChanged`. Covered by inspection and `qmllint`.
10. **Fixed: stale format chip.** The active format label is cleared when the
    app enters the error or no-camera state, and the selection strings are
    cleared when the last camera disappears; the chip hides when there is no
    format to show. A busy-camera run reports `state=Camera error` with an
    empty `format=` field (`evidence/fix3/ui-check-busy.log`).

## Low (fixed or documented 2026-10-13)

11. **Fixed: `QDesktopServices::openUrl()` results are ignored.** A failed
    folder open now emits an error message. Covered by inspection.
12. **Fixed: microphone availability is sampled once.** `audioInputsChanged`
    now refreshes the availability flag. Covered by inspection; a live
    hot-plug check was not run.
13. **Fixed: the theme helper process had no timeout, and a late state
    directory was not watched.** The helper is killed after 5 s with the
    fallback or last-good palette kept (the timeout test records 6 heartbeats
    instead of 12 and no hang), and the watcher now attaches to the nearest
    existing ancestor so a state directory created after startup is picked up
    (two palette loads after the directory appears).
    See `evidence/fix3/theme-timeout.log` and
    `evidence/fix3/theme-late-dir.log`.
14. **Closed as documented: the frame-rate list can offer standard rates
    inside a device range.** The list already includes the exact reported
    minimum and maximum plus standard rates within the range, and Qt exposes
    only ranges, so discrete-capability detection is not possible; cameras
    that quantize internally snap to the nearest supported rate. See the
    README limitations.
15. **Fixed: output file naming stops checking after 1000 attempts.** After
    the sequential attempts a randomized suffix keeps checking; the new unit
    test `avoidsCollisionCap` covers the fallback (`evidence/fix2/ctest.log`).
16. **Fixed: accessibility.** `AppButton` and the shutter are Tab-reachable
    and activate with Return/Enter, controls carry accessible names
    (buttons, `Take photo`, `Microphone`, `Mirror preview`, combo labels) and
    `--ui-check` reports the focused item. Traversal verified over six
    controls: Mirror, shutter, Record, Settings, Photos, Videos
    (`evidence/fix3/a11y-tab.txt`, `a11y-tab-wide.log`, `tab-steps/`).
17. **Fixed: recording time formatting past 99 minutes.** Durations of an hour
    or more now format as `H:MM:SS`. Covered by inspection; a real hour-long
    recording was not run.

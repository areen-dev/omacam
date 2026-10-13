# Known issues

Status of this list: defects found by a code and live-test audit on 2026-09-21
and re-checked during the final verification on 2026-10-13. Items 1-4 were
fixed and re-verified on 2026-10-13; logs are under `evidence/fix/`. Items 5-17
remain open. Audit evidence is under `evidence/known-issues/`.

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

## Medium

5. **No recovery for the same camera after an error.** "Rescan cameras" only
   restarts the device when the selection changes, and selecting the same
   device is a no-op. With one camera, only a restart recovers.
6. **`--self-test` writes the temporary photo and video folders into the user
   settings file.** Run it with an isolated `XDG_CONFIG_HOME` or accept that
   the folders change. Documented behavior is missing.
7. **Format, device and microphone controls stay enabled during recording.**
   Changing resolution mid-recording calls `setCameraFormat()`; the microphone
   switch is silently ignored by the recorder, so the switch can show a state
   the recording is not using.
8. **The device combo desynchronizes when a switch is refused** (for example
   during recording): the combo shows the new device while the model keeps the
   old one.
9. **The Timer combo does not follow the T key** while the settings panel is
   open, because it is not connected to `timerSecondsChanged`.
10. **Stale format chip.** During an error the status chip still shows the
    last format at 0 fps, and when all cameras disappear the selection strings
    are not cleared.

## Low

11. `QDesktopServices::openUrl()` results are ignored; a failed folder open
    gives no feedback.
12. Microphone availability is sampled once; a microphone connected later is
    not noticed.
13. The theme helper process has no timeout, and live theme reload is not
    installed when the Omarchy state directory is absent at startup.
14. The frame-rate list can offer standard rates inside a device range that
    the device does not support discretely.
15. Output file naming stops checking for collisions after 1000 attempts.
16. Accessibility: the custom buttons and the shutter have no accessible
    names or roles and cannot be reached with Tab; switches have no
    accessible labels.
17. Documentation: recording time formatting breaks past 99 minutes. The
    self-test settings side effect and the `--ui-check` exit codes are now
    documented in `README.md`.

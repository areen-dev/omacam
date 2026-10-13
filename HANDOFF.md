# Handoff (2026-10-13)

## Where things stand

- The four high-severity defects from the first audit are fixed and
  re-verified on 2026-10-13. Recording start is gated on recorder
  confirmation (bounded 5 s start wait; failures surface with an error
  message), the error banner binds to a `hasError` property (busy-camera
  `--ui-check` reports `state=Camera error ... errorBanner=true`), starting a
  recording cancels the self-timer countdown (countdown expiry also refuses to
  capture while recording), and the application identity is set before
  `QGuiApplication` construction (portal registration warning gone).
  Logs under `evidence/fix/`; results in `VERIFICATION.md`
  ("Fix verification run").
- New deterministic regression mode `--record-during-timer`: starts a
  recording while the self-timer runs and checks that no photo is saved
  during the recording.
- Manifest regenerated for the fix run (`EVIDENCE.sha256`); scaffold `check`,
  `snapshot` and `verify` pass.
- Destination decided and taken: public GitHub source repository
  `https://github.com/areen-dev/omacam`, tag `v0.1.0`, release with a source
  archive and checksums.
- Focused CI (`.github/workflows/ci.yml`) builds, runs the unit tests and
  validates the desktop entry and QML in an Arch container; live desktop
  acceptance stays out of CI.
- Installer on this machine updated to the fixed build
  (`~/.local/bin/dev.areen.omacam`).

## Open items

- `KNOWN-ISSUES.md` items 5-17 (medium/low): camera rescan after an error,
  self-test settings side effect, controls enabled during recording, combo
  desync, timer combo sync, stale format chip, folder-open feedback, hot-plug
  microphone, theme helper timeout, frame-rate list, naming collision cap,
  accessibility, documentation residue.
- Live desktop acceptance still pending: icon rendering in the launcher/menu,
  a full `omarchy theme set` switch while open, terminal start, paths with
  spaces through the UI, multi-monitor, HiDPI scaling, IME/paste.
- Packaging is deliberately deferred (no PKGBUILD); AUR or omarchy-pkgs
  contribution can follow the package workflow when wanted.
- No demo media is published; a README screenshot was deliberately omitted
  for privacy.

## Suggested next steps

1. Medium items: #5 (rescan the same camera after an error), #6 (isolate the
   self-test settings or keep documenting the wrapper), #7-#10 (UI state
   consistency during recording/errors).
2. Accessibility pass (#16) with a real focus/Tab traversal check.
3. Optional: packaging (PKGBUILD or omarchy-pkgs), demo capture with isolated
   data directories, then a full desktop acceptance run.

See `AGENTS.md` for environment specifics (mise toolchain, full lint path,
manifest regeneration).

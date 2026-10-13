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
- A second pass fixed medium/low items 5-9, 11, 12, 15 and 17: camera rescan
  recovery after an error, self-test settings isolation (the self-test no
  longer writes the settings file), controls disabled during recording, combo
  resync after refused switches and T-key changes, folder-open error
  feedback, microphone hot-plug detection, randomized collision fallback and
  `H:MM:SS` duration formatting. Logs: `evidence/fix2/`; results:
  `VERIFICATION.md` ("Second fix verification run"). Published as `v0.1.1`.
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

- `KNOWN-ISSUES.md` items 10, 13, 14 and 16 (medium/low): stale format chip
  during errors, theme helper timeout and late state directory, the
  frame-rate list question (needs a decision; Qt exposes only ranges), and
  accessibility.
- Live desktop acceptance still pending: icon rendering in the launcher/menu,
  a full `omarchy theme set` switch while open, terminal start, paths with
  spaces through the UI, multi-monitor, HiDPI scaling, IME/paste.
- Live checks not yet run: microphone hot-plug, controls-disabled interaction
  during recording, folder-open failure feedback, recording past one hour.
- Packaging is deliberately deferred (no PKGBUILD); AUR or omarchy-pkgs
  contribution can follow the package workflow when wanted.
- No demo media is published; a README screenshot was deliberately omitted
  for privacy.

## Suggested next steps

1. Remaining defects: #10 (clear the format chip on errors), #13 (process
   timeout plus watching the nearest existing ancestor of the theme state
   directory), #14 (decide restrict vs document), #16 (accessibility pass,
   which also unlocks live keyboard checks for #7-#9).
2. A wider live acceptance run on the released build (icon, theme switch,
   terminal start, spaces, scaling, multi-monitor, IME) plus the corruption/
   read-only settings path with an isolated `XDG_CONFIG_HOME`.
3. Optional: packaging (PKGBUILD or omarchy-pkgs), demo capture with isolated
   data directories.

See `AGENTS.md` for environment specifics (mise toolchain, full lint path,
manifest regeneration).

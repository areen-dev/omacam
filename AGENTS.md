# AGENTS.md

Guidance for agent sessions in this repository.

## Project

Omacam — small native camera app for Omarchy. Qt 6 Quick + Multimedia (FFmpeg
backend), C++20, CMake + Ninja. Identity: `dev.areen.omacam`, name `Omacam`.

## Environment (this machine)

- `cmake`/`ninja` are NOT installed system-wide; run via mise:
  `mise exec cmake@latest ninja@latest -- cmake ...` (install.sh the same way).
  To install properly: `omarchy pkg add cmake ninja`.
- Scaffold/evidence tool:
  `mise exec rust@latest -- cargo run --manifest-path /home/areen/.config/opencode/skills/omarchy-app-scaffold/scripts/app-tool/Cargo.toml -- check|snapshot|verify <dir>`.
  `verify` fails when evidence changed but `EVIDENCE.sha256` was not
  regenerated — snapshot again after collecting evidence.
- Qt logs reach stderr only with `QT_FORCE_STDERR_LOGGING=1` (agent shells run
  under systemd/journald). QML `console.log` is stripped in Release builds.
- Key-injection tests use `wtype` and require focus on the app window. Verify
  focus via `hyprctl activewindow -j` immediately before injecting and abort
  otherwise — never type into other windows (see
  `evidence/known-issues/timer-during-recording.txt`).
- Hyprland dispatches:
  `hyprctl dispatch 'hl.dsp.window.float({ class = "dev.areen.omacam" })'`,
  `hyprctl dispatch 'hl.dsp.focus({ window = "class:dev.areen.omacam" })'`.

## Build / test / verify

- Build (fresh dir, e.g. `/tmp/opencode/build`):
  `mise exec cmake@latest ninja@latest -- cmake -S . -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Release`
  then `... -- cmake --build <dir>`.
- Unit tests: `ctest` (mise shim without a default version: run
  `mise exec cmake@latest -- ctest --test-dir <dir>`). Lint: `qmllint` is not
  on `PATH`; use `/usr/lib/qt6/bin/qmllint`. Entry validation:
  `desktop-file-validate packaging/dev.areen.omacam.desktop`.
- Live checks: `--self-test <dir> --format <WxH>`, `--cycle-timer`,
  `--quit-during-record`, `--record-during-timer`, `--ui-check <seconds>`,
  `--rescan-every <seconds>`, `--tab-check <steps>` (see README).
- Install: `./install.sh` (upgrade-safe); launcher check with `gtk-launch` and
  `hyprctl clients`.
- Record evidence under `evidence/<stage>/`, update `VERIFICATION.md`, then
  regenerate the manifest (check, snapshot, verify).

## Conventions

- No code comments unless necessary; match existing style.
- Never hand-edit `EVIDENCE.sha256`; always regenerate via the scaffold tool.
- `target/` (build dir) is excluded from the manifest.
- Keep `KNOWN-ISSUES.md` current when defects are fixed.

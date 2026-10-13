# Security

## Reporting a vulnerability

Report privately through GitHub's private vulnerability reporting:
**Security → Advisories → "Report a vulnerability"** on
https://github.com/areen-dev/omacam, or open
https://github.com/areen-dev/omacam/security/advisories/new directly.

Include what you found, how to reproduce it, and the affected version. You
can expect an acknowledgement within a few days. Fixes ship as patch releases
and reporters are credited unless they prefer otherwise.

## Supported versions

Only the latest release is supported. Security fixes are released as patch
versions (for example 0.1.3).

## What Omacam is

A local desktop camera application. It runs unprivileged as the logged-in
user, captures from the system camera, writes photos and videos into
user-chosen folders, and reads the Omarchy theme palette. It has no network
functionality, no service, no setuid component and no telemetry.

## Threat model

In scope:

- A local attacker who can write into a folder the user selects for photos
  or videos (for example a shared or world-writable directory), or who can
  place files in the user's settings directory.
- Hostile or malformed settings files and unusual folder paths.
- Supply-chain risk in the build, CI and release process.

Out of scope:

- An attacker who already runs code as the same user; no boundary is
  crossed.
- Vulnerabilities in Qt, FFmpeg or the kernel; report those upstream.
- Physical attacks and malicious camera hardware.

## Security properties

- No network code in the application. `libQt6Network`, `libcurl` and `libssl`
  are linked transitively through Qt Multimedia/FFmpeg; the app issues no
  network requests.
- Settings values are escaped on write and validated on read; malformed
  files are preserved, oversized files and non-finite numbers fall back to
  defaults, and folder paths are stripped of control characters.
- Media files are written with symlink-aware, atomic file handling: planted
  symlinks are treated as occupied names, photos are written through
  `QSaveFile` (exclusive temporary file plus rename, never following a
  destination symlink), and the `*.part` recording file is created with
  `QIODevice::NewOnly` before the recorder opens it. `QFile::rename` replaces
  a destination symlink rather than following it.
- Release binaries are built with PIE, full RELRO and BIND_NOW,
  `-fstack-protector-strong`, `-fstack-clash-protection`,
  `_FORTIFY_SOURCE=3` (optimized builds) and `-fcf-protection=full` on
  x86_64.
- GitHub private vulnerability reporting, Dependabot alerts, automated
  security fixes, Dependabot version updates for GitHub Actions and CodeQL
  scanning of the workflows are enabled for this repository.

## Review history

### 2026-10-14

Adversarial review of 0.1.2 with a local-attacker threat model.

1. **Symlink-following writes to attacker-chosen paths (Medium, fixed).**
   Output-name selection only checked `QFileInfo::exists()`, which follows
   symlinks, so a broken symlink planted in the output folder
   (`IMG_<timestamp>.jpg` or `VID_<timestamp>.mp4.part`) counted as a free
   name and was followed on write. A proof of concept against 0.1.2 wrote an
   81 KB JPEG and a 185 KB MP4 to `/tmp` targets of the attacker's choosing
   while the app reported success — a camera/recording exfiltration and
   file-clobbering primitive on shared machines. Fixed by treating symlinks
   as occupied names (`output::pathTaken`), writing photos through
   `QSaveFile`, and creating the recording file exclusively. Re-running the
   same proof of concept leaves the targets untouched and saves under
   collision-suffixed names. A deliberately racing attacker could still
   replace the just-created recording file before FFmpeg opens it; that
   residual window is accepted and requires continuous write access plus
   precise timing. Evidence: `evidence/security/`.
2. **Settings robustness (Low, fixed).** Settings files larger than 1 MiB are
   rejected with the defaults instead of being read; `inf`/`nan` numeric
   values are kept as strings; folder paths from settings and the UI have
   control characters stripped.
3. **Binary hardening (added).** Compiler and linker hardening flags are now
   enforced in `CMakeLists.txt` and verified with `readelf`
   (`evidence/security/hardening.txt`).
4. **Theme helper PATH lookup (accepted).** `omarchy-theme-color` is started
   via `PATH`. A same-user attacker who controls `PATH` can already execute
   code as the user, so no boundary is crossed; kept for Omarchy
   compatibility and documented here.
5. **Supply chain (process).** Dependabot watches GitHub Actions, CodeQL
   scans the workflows, and GitHub's private vulnerability reporting,
   vulnerability alerts and automated security fixes are enabled. Third-party
   actions are referenced by major version rather than commit SHA; SHA
   pinning is a recommended future hardening step. Release tags are not
   GPG-signed; signing with the maintainer's key is recommended before any
   broader distribution.
6. **Evidence logs (informational).** Verification logs under `evidence/`
   contain local paths and machine details by design; no credentials or
   camera imagery are published.

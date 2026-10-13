# Credits

Community App badge: https://github.com/tcballard/omarchy-badges (MIT layout;
Omarchy name and icon rights retained by their owners). The badge is linked
remotely; no other artwork is bundled.

Application source: MIT, see [LICENSE](LICENSE).

Icon: `packaging/dev.areen.omacam.svg`, original work for this project (MIT).

Runtime dependencies, not vendored:

- Qt 6 (LGPLv3), dynamically linked as packaged by Arch: `qt6-base`,
  `qt6-declarative`, `qt6-multimedia`, `qt6-wayland`.
- FFmpeg n9.0.1 through the Qt Multimedia FFmpeg backend, reported at runtime as
  "GPL version 3 or later". Any binary redistribution must account for that
  backend's GPL obligations.
- Omarchy theme colors are read from `omarchy-theme-color` output at runtime;
  no Omarchy assets are copied into this project.

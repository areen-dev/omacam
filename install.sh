#!/usr/bin/env bash
set -euo pipefail

APP_ID="dev.areen.omacam"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${PREFIX:-$HOME/.local}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/target}"
ACTION="install"

usage() {
  cat <<EOF
Usage: ./install.sh [options]

Options:
  --prefix=<dir>     Install prefix (default: \$HOME/.local)
  --build-dir=<dir>  Build directory (default: ./target)
  --uninstall        Remove installed files, keeping user data
  -h, --help         Show this help
EOF
}

for arg in "$@"; do
  case "$arg" in
    --prefix=*) PREFIX="${arg#*=}" ;;
    --build-dir=*) BUILD_DIR="${arg#*=}" ;;
    --uninstall) ACTION="uninstall" ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $arg" >&2; usage >&2; exit 1 ;;
  esac
done

require_build_tools() {
  local missing=()
  command -v cmake >/dev/null 2>&1 || missing+=(cmake)
  command -v ninja >/dev/null 2>&1 || missing+=(ninja)
  command -v ctest >/dev/null 2>&1 || missing+=(cmake)
  if ((${#missing[@]})); then
    echo "Missing build tools: ${missing[*]}" >&2
    echo "Install them with:" >&2
    echo "  omarchy pkg add cmake ninja" >&2
    exit 1
  fi
}

require_runtime_deps() {
  command -v omarchy >/dev/null 2>&1 || return 0
  local runtime=(qt6-base qt6-declarative qt6-multimedia qt6-wayland)
  if ! omarchy pkg present "${runtime[@]}" >/dev/null 2>&1; then
    echo "Missing runtime dependencies." >&2
    echo "Install them with:" >&2
    echo "  omarchy pkg add ${runtime[*]}" >&2
    exit 1
  fi
}

refresh_caches() {
  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$PREFIX/share/applications" >/dev/null 2>&1 || true
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f "$PREFIX/share/icons/hicolor" >/dev/null 2>&1 || true
  fi
}

install_app() {
  require_build_tools
  require_runtime_deps

  if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$SCRIPT_DIR/packaging/$APP_ID.desktop"
  else
    echo "desktop-file-validate not found; skipping desktop entry validation" >&2
  fi

  cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
  cmake --build "$BUILD_DIR"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
  cmake --install "$BUILD_DIR"
  refresh_caches

  echo
  echo "Installed $APP_ID"
  echo "  binary:  $PREFIX/bin/$APP_ID"
  echo "  desktop: $PREFIX/share/applications/$APP_ID.desktop"
  echo "  icon:    $PREFIX/share/icons/hicolor/scalable/apps/$APP_ID.svg"
  echo "Uninstall with: ./install.sh --uninstall"
}

uninstall_app() {
  rm -f "$PREFIX/bin/$APP_ID"
  rm -f "$PREFIX/share/applications/$APP_ID.desktop"
  rm -f "$PREFIX/share/icons/hicolor/scalable/apps/$APP_ID.svg"
  refresh_caches

  echo "Removed $APP_ID application files."
  echo "Kept user data:"
  echo "  settings: ~/.config/$APP_ID/camera.conf"
  echo "  photos:   ~/Pictures/Camera (or the configured folder)"
  echo "  videos:   ~/Videos/Camera (or the configured folder)"
}

case "$ACTION" in
  install) install_app ;;
  uninstall) uninstall_app ;;
esac

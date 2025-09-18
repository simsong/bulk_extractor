#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "============================================================"
echo " Bulk Extractor Windows/MSYS2 setup"
echo "============================================================"

# ------------------------------------------------------------
# Setup libewf
# ------------------------------------------------------------

# Default libewf URL (used if paths.bash is missing)
DEFAULT_LIBEWF_URL="https://github.com/libyal/libewf/releases/download/20240506/libewf-experimental-20240506.tar.gz"

# Source paths.bash if available
if [[ -f "$SCRIPT_DIR/paths.bash" ]]; then
  source "$SCRIPT_DIR/paths.bash"
  LIBEWF_URL="${LIBEWF_URL:-$DEFAULT_LIBEWF_URL}"
else
  LIBEWF_URL="$DEFAULT_LIBEWF_URL"
fi

echo "Installing libewf from source..."
echo "Using URL: $LIBEWF_URL"

curl -LO "$LIBEWF_URL" || { echo "could not download $LIBEWF_URL"; exit 1; }

LIBEWF_DIR=$(tar tzf libewf-*.tar.gz | head -1 | cut -f1 -d"/")
cd "$LIBEWF_DIR" || { echo "libewf source dir missing"; exit 1; }

./configure --prefix=/ucrt64
make -j"$(nproc)"
make install
cd ..

# Verify installation
if command -v ewfinfo >/dev/null 2>&1; then
  ewfinfo -h >/dev/null
  echo "libewf successfully installed."
else
  echo "ERROR: libewf installation failed."
  exit 1
fi

# ------------------------------------------------------------
# Setup required MSYS2 packages
# ------------------------------------------------------------

pacman -Syu --noconfirm

PKGS+="base-devel automake autoconf pkgconf
         mingw-w64-ucrt-x86_64-gcc
         mingw-w64-ucrt-x86_64-make
         mingw-w64-ucrt-x86_64-re2
         mingw-w64-ucrt-x86_64-abseil-cpp
         mingw-w64-ucrt-x86_64-sqlite3
         mingw-w64-ucrt-x86_64-openssl
         mingw-w64-ucrt-x86_64-expat
         mingw-w64-ucrt-x86_64-ncurses"

pacman -S --needed --noconfirm $PKGS || (echo msys package install failed; exit 1)

echo "============================================================"
echo " Setup complete"
echo "============================================================"

exit 0

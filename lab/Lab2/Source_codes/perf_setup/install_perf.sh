#!/usr/bin/env bash
#
# Makes `perf` available on an AUP-ZU3 board (PYNQ, Ubuntu 22.04, kernel 6.6).
# If a working perf is already installed, it changes nothing.
# It needs no sudo password: perf goes in ~/.local/bin, for the current user only.
#
# The PYNQ image has no perf and no package to install it from: the kernel
# sources on the board have tools/perf stripped out, and the board is offline.
# So perf comes from the Ubuntu package that sits next to this script, which is
# the one `apt install linux-tools-generic` would pick on the board.
#
# Usage:
#   1. Copy the whole perf_setup folder to your home directory on the board.
#      If you copy through the microSD card, eject the card safely first.
#   2. On the board:  cd ~/perf_setup
#   3. Run:  ./install_perf.sh
#   4. Run:  source ~/.profile   (only once, in the terminal you installed from)
#   5. Check that perf works:  ./test_perf.sh

set -u

DEB_NAME="linux-tools-5.15.0-25_5.15.0-25.25_arm64.deb"
DEB_SHA256="d686cc487244511f9048878656ef1d825e44d6eee9ca286cfa55eca5993c7a1e"
PERF_IN_DEB="usr/lib/linux-tools-5.15.0-25/perf"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEB_PATH="$SCRIPT_DIR/$DEB_NAME"
TARGET="$HOME/.local/bin/perf"

die() {
    echo "ERROR: $*" >&2
    exit 1
}

# Being on the PATH is not enough: Ubuntu's /usr/bin/perf is a wrapper that
# fails on kernels it has no matching perf for, like this board's.
perf_works() {
    "$1" --version >/dev/null 2>&1 && "$1" stat -e task-clock -- true >/dev/null 2>&1
}

if command -v perf >/dev/null 2>&1 && perf_works perf; then
    echo "perf is already installed: $(command -v perf) ($(perf --version))"
    exit 0
fi

[ "$(uname -m)" = aarch64 ] || die "this installs the arm64 perf, so run it on the board, not on $(uname -m)."
[ -f "$DEB_PATH" ] || die "$DEB_NAME not found. Copy it next to this script ($SCRIPT_DIR)."

echo "perf is not installed. Installing it from $DEB_PATH"

# A card pulled out without ejecting can leave a truncated copy on the FAT partition.
echo "$DEB_SHA256  $DEB_PATH" | sha256sum --check --status \
    || die "$DEB_NAME is corrupted. Copy it to the card again and eject the card safely."

TMP_DIR="$(mktemp -d)" || die "cannot create a temporary directory."
trap 'rm -rf "$TMP_DIR"' EXIT

dpkg-deb -x "$DEB_PATH" "$TMP_DIR" || die "cannot unpack $DEB_NAME."
NEW_PERF="$TMP_DIR/$PERF_IN_DEB"
[ -x "$NEW_PERF" ] || die "$PERF_IN_DEB is missing from $DEB_NAME."

MISSING_LIBS="$(ldd "$NEW_PERF" | grep "not found")"
[ -z "$MISSING_LIBS" ] || die "perf needs libraries this image does not have:
$MISSING_LIBS"

install -D -m 0755 "$NEW_PERF" "$TARGET" || die "cannot install $TARGET."

perf_works "$TARGET" || {
    "$TARGET" stat -e task-clock -- true >&2
    die "$TARGET is installed but does not run correctly (see the output above)."
}

echo "Installed $TARGET ($("$TARGET" --version))"

# Ubuntu's ~/.profile adds ~/.local/bin to the PATH at login, once the folder exists.
hash -r
if [ "$(command -v perf)" != "$TARGET" ]; then
    echo "Run this once so this terminal can find perf:  source ~/.profile"
fi
echo "Then check that it works with:  ./test_perf.sh"

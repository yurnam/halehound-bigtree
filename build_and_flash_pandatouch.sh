#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
# build_and_flash_pandatouch.sh
# Build HaleHound firmware for BigTreeTech PandaTouch and flash via USB.
#
# Usage:
#   ./build_and_flash_pandatouch.sh           # build + flash
#   ./build_and_flash_pandatouch.sh build     # build only
#   ./build_and_flash_pandatouch.sh flash     # flash only (reuses last build)
#   ./build_and_flash_pandatouch.sh monitor   # open serial monitor after flash
#
# Requirements:
#   - PlatformIO Core (pio) installed and on PATH
#   - PandaTouch connected via USB-C to /dev/ttyUSB0
#   - User in 'dialout' group (or run with sudo)
#
# Target device:  BigTreeTech PandaTouch
#   MCU:          ESP32-S3 (dual-core LX7, 240 MHz)
#   PSRAM:        8 MB OPI
#   Display:      7" 800×480 RGB-parallel LCD (16-bit)
#   Touch:        GT911 capacitive (I²C)
#   Features:     WiFi + Bluetooth (no external radio modules)
# ═══════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/HaleHound-CYD"
ENV="pandatouch"
PORT="/dev/ttyUSB0"

# ── colour helpers ──────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERROR]${RESET} $*" >&2; }

# ── sanity checks ───────────────────────────────────────────────────────────
check_pio() {
    if ! command -v pio &>/dev/null; then
        error "PlatformIO (pio) not found."
        echo "  Install with:  pip install platformio"
        echo "  Or visit:      https://platformio.org/install/cli"
        exit 1
    fi
}

check_port() {
    if [ ! -e "$PORT" ]; then
        warn "Device not found at $PORT."
        warn "Make sure the PandaTouch is connected via USB-C and recognised as a serial device."
        warn "On some systems the port may appear as /dev/ttyACM0 — edit PORT= in this script."
        exit 1
    fi
}

# ── build ───────────────────────────────────────────────────────────────────
do_build() {
    info "Building HaleHound firmware for PandaTouch (env: $ENV) ..."
    cd "$PROJECT_DIR"
    pio run --environment "$ENV"
    success "Build complete."
}

# ── flash ───────────────────────────────────────────────────────────────────
do_flash() {
    check_port
    info "Flashing to PandaTouch at $PORT ..."
    cd "$PROJECT_DIR"
    pio run --environment "$ENV" --target upload --upload-port "$PORT"
    success "Flash complete."
}

# ── monitor ─────────────────────────────────────────────────────────────────
do_monitor() {
    check_port
    info "Opening serial monitor on $PORT (115200 baud) — Ctrl-C to exit ..."
    cd "$PROJECT_DIR"
    pio device monitor --port "$PORT" --baud 115200 --filter esp32_exception_decoder
}

# ── main ────────────────────────────────────────────────────────────────────
echo -e "${BOLD}"
echo "  ██╗  ██╗ █████╗ ██╗     ███████╗██╗  ██╗ ██████╗ ██╗   ██╗███╗   ██╗██████╗ "
echo "  ██║  ██║██╔══██╗██║     ██╔════╝██║  ██║██╔═══██╗██║   ██║████╗  ██║██╔══██╗"
echo "  ███████║███████║██║     █████╗  ███████║██║   ██║██║   ██║██╔██╗ ██║██║  ██║"
echo "  ██╔══██║██╔══██║██║     ██╔══╝  ██╔══██║██║   ██║██║   ██║██║╚██╗██║██║  ██║"
echo "  ██║  ██║██║  ██║███████╗███████╗██║  ██║╚██████╔╝╚██████╔╝██║ ╚████║██████╔╝"
echo "  ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝ "
echo "  PandaTouch Edition  —  ESP32-S3  |  7\" 800×480  |  GT911  |  WiFi + BT"
echo -e "${RESET}"

check_pio

ACTION="${1:-all}"

case "$ACTION" in
    build)
        do_build
        ;;
    flash)
        do_flash
        ;;
    monitor)
        do_monitor
        ;;
    all|"")
        do_build
        do_flash
        info "To open the serial monitor run:"
        echo "    $0 monitor"
        ;;
    *)
        error "Unknown action: $ACTION"
        echo "Usage: $0 [build|flash|monitor]"
        exit 1
        ;;
esac

// ═══════════════════════════════════════════════════════════════════════════
// pt_usb_storage.h  —  USB flash-drive adapter for PandaTouch
//
// The PandaTouch's ESP32-S3 USB-C port (GPIO 19 / 20) can operate as a USB
// OTG Host. This adapter initialises the ESP-IDF USB host stack, installs
// the MSC class driver, and mounts the FAT filesystem from the first USB
// drive found to "/usb".  It exposes an SD-class–compatible API (FS base
// class + begin()/end()) so every SD.open / SD.exists / SD.mkdir / etc. call
// in the rest of the firmware just works transparently.
//
// Usage:
//   • pt_tft_compat.h #defines SD to ptSD and includes this header — no other
//     changes are required in the feature modules.
//   • SD_CS == -1 on PandaTouch; any SD.begin(SD_CS, ...) call reaches
//     ptSD.begin() which ignores the SPI parameters and initialises USB host.
//
// Requires: espressif32@6.12.0+ (ESP-IDF 5.3.x), ESP32-S3 board.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <FSImpl.h>
#include <SPI.h>   // for SPIClass type in begin() signature
extern "C" {
#include <sys/stat.h>
#include <dirent.h>
}

// ── Availability guard ────────────────────────────────────────────────────
// If the USB host MSC headers are not present (e.g. wrong IDF version),
// we compile a stub that always returns false / empty results so the feature
// modules degrade gracefully instead of crashing.
#if __has_include("usb/msc_host_vfs.h")
  #define PT_USB_MSC_AVAIL 1
#else
  #define PT_USB_MSC_AVAIL 0
  #warning "[HH] USB MSC host headers not found — USB storage will not be available"
#endif

using namespace fs;

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbFileImpl  — wraps a POSIX FILE* or DIR* as a FileImpl
// ─────────────────────────────────────────────────────────────────────────────

class PtUsbFileImpl : public FileImpl {
public:
    // Regular file
    PtUsbFileImpl(const char* fullPath, const char* mode);
    // Directory (takes ownership of DIR*)
    PtUsbFileImpl(const char* fullPath, DIR* dir);
    ~PtUsbFileImpl() override;

    size_t  write(const uint8_t* buf, size_t size) override;
    size_t  read(uint8_t* buf, size_t size) override;
    void    flush() override;
    bool    seek(uint32_t pos, SeekMode mode) override;
    size_t  position() const override;
    size_t  size() const override;
    bool    setBufferSize(size_t size) override { return false; }
    void    close() override;
    time_t  getLastWrite() override;
    const char* path() const override { return _path; }
    const char* name() const override;
    boolean isDirectory() override { return _isDir; }
    FileImplPtr openNextFile(const char* mode) override;
    boolean     seekDir(long position) override;
    String      getNextFileName() override;
    String      getNextFileName(bool* isDir) override;
    void        rewindDirectory() override;
    operator bool() override { return _isDir ? (_dir != nullptr) : (_file != nullptr); }

private:
    FILE*   _file;
    DIR*    _dir;
    char    _path[512];
    bool    _isDir;

    void _getStat(struct stat* st) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbFSImpl  — FSImpl backed by the /usb VFS mount point
// Strips any leading mountpoint prefix from caller-supplied paths so that
// paths returned by File::path() (which include the prefix) can be passed
// back to SD.remove() / SD.rmdir() without producing a double-prefix.
// ─────────────────────────────────────────────────────────────────────────────

class PtUsbFSImpl : public FSImpl {
public:
    explicit PtUsbFSImpl(const char* mountPt);

    FileImplPtr open(const char* path, const char* mode, const bool create) override;
    bool exists(const char* path) override;
    bool rename(const char* from, const char* to) override;
    bool remove(const char* path) override;
    bool mkdir(const char* path) override;
    bool rmdir(const char* path) override;

private:
    char _mp[64];   // mount point, e.g. "/usb"

    // Build the canonical POSIX path, stripping any existing mount-prefix.
    void _makePath(const char* p, char* buf, size_t bufsz) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbStorageClass — SD-compatible class backed by a USB flash drive
// ─────────────────────────────────────────────────────────────────────────────

class PtUsbStorageClass : public FS {
public:
    PtUsbStorageClass();

    // begin() — matches the Arduino SD.begin() signature so callers need no changes.
    // csPin, spi, and freq are ignored; mountpoint defaults to "/usb".
    bool begin(uint8_t csPin = 0, SPIClass& spi = SPI, uint32_t freq = 4000000,
               const char* mountpoint = "/usb", uint8_t maxFiles = 5,
               bool formatIfEmpty = false);

    void end();
    bool isMounted() const { return _mounted; }

private:
    bool   _mounted;
    char   _mountPoint[64];

#if PT_USB_MSC_AVAIL
    bool _initUsb(const char* mountpoint);
    void _deinitUsb();
#endif
};

// Global USB storage object — SD is #defined to this in pt_tft_compat.h
extern PtUsbStorageClass ptSD;

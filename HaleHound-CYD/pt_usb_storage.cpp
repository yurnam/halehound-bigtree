// ═══════════════════════════════════════════════════════════════════════════
// pt_usb_storage.cpp  —  USB flash-drive adapter implementation
// Requires espressif32@6.12.0 (ESP-IDF 5.3.x), ESP32-S3.
// ═══════════════════════════════════════════════════════════════════════════

#ifdef PANDATOUCH

#include "pt_usb_storage.h"

#include <string.h>
#include <errno.h>
extern "C" {
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
}

// ── USB MSC host stack (ESP-IDF 5.3.x) ────────────────────────────────────
#if PT_USB_MSC_AVAIL
extern "C" {
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "esp_vfs_fat.h"
}
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// File-static USB host state (shared between the host task and begin()/end())
static SemaphoreHandle_t        s_mountSem    = nullptr;
static volatile bool            s_driveMounted = false;
static volatile bool            s_hostRunning  = false;
static msc_host_device_handle_t s_mscDevice   = nullptr;
static msc_host_vfs_handle_t    s_vfsHandle   = nullptr;
static TaskHandle_t             s_hostTaskHdl = nullptr;
static char                     s_mountPt[64] = "/usb";

// ── MSC event callback — correct C-linkage signature ─────────────────────
// Declared as a file-static free function so its type exactly matches
// msc_host_event_cb_t = void(*)(const msc_host_event_t*, void*).
static void _pt_msc_event_cb(const msc_host_event_t* event, void* /*arg*/) {
    if (event->event == MSC_DEVICE_CONNECTED) {
        Serial.printf("[USB] MSC device connected (addr %u)\n",
                      event->device.address);
        s_mscDevice = event->device.handle;

        esp_vfs_fat_mount_config_t mc = {};
        mc.format_if_mount_failed = false;
        mc.max_files               = 5;

        esp_err_t err = msc_host_vfs_register(s_mscDevice, s_mountPt, &mc,
                                              &s_vfsHandle);
        if (err == ESP_OK) {
            s_driveMounted = true;
            Serial.printf("[USB] FAT mounted at %s\n", s_mountPt);
        } else {
            Serial.printf("[USB] msc_host_vfs_register error: 0x%x\n", err);
            s_vfsHandle = nullptr;
            s_mscDevice = nullptr;
        }
        if (s_mountSem) xSemaphoreGive(s_mountSem);

    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        Serial.println("[USB] MSC device disconnected");
        s_driveMounted = false;
        if (s_vfsHandle) {
            msc_host_vfs_unregister(s_vfsHandle);
            s_vfsHandle = nullptr;
        }
        s_mscDevice = nullptr;
    }
}

// ── USB host daemon task ──────────────────────────────────────────────────
static void _pt_usb_host_task(void* /*arg*/) {
    while (s_hostRunning) {
        uint32_t event_flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            usb_host_uninstall();
            s_hostRunning = false;
            break;
        }
    }
    vTaskDelete(nullptr);
}
#endif // PT_USB_MSC_AVAIL

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbFileImpl implementation
// ─────────────────────────────────────────────────────────────────────────────

PtUsbFileImpl::PtUsbFileImpl(const char* fullPath, const char* mode)
    : _file(nullptr), _dir(nullptr), _isDir(false)
{
    strncpy(_path, fullPath ? fullPath : "", sizeof(_path) - 1);
    _path[sizeof(_path) - 1] = '\0';
    if (fullPath && mode) {
        _file = ::fopen(fullPath, mode);
    }
}

PtUsbFileImpl::PtUsbFileImpl(const char* fullPath, DIR* dir)
    : _file(nullptr), _dir(dir), _isDir(true)
{
    strncpy(_path, fullPath ? fullPath : "", sizeof(_path) - 1);
    _path[sizeof(_path) - 1] = '\0';
}

PtUsbFileImpl::~PtUsbFileImpl() {
    close();
}

void PtUsbFileImpl::close() {
    if (_file) { ::fclose(_file); _file = nullptr; }
    if (_dir)  { ::closedir(_dir); _dir = nullptr; }
}

size_t PtUsbFileImpl::write(const uint8_t* buf, size_t size) {
    if (!_file) return 0;
    return ::fwrite(buf, 1, size, _file);
}

size_t PtUsbFileImpl::read(uint8_t* buf, size_t size) {
    if (!_file) return 0;
    return ::fread(buf, 1, size, _file);
}

void PtUsbFileImpl::flush() {
    if (_file) ::fflush(_file);
}

bool PtUsbFileImpl::seek(uint32_t pos, SeekMode mode) {
    if (!_file) return false;
    int whence = SEEK_SET;
    if (mode == SeekCur) whence = SEEK_CUR;
    else if (mode == SeekEnd) whence = SEEK_END;
    return ::fseek(_file, (long)pos, whence) == 0;
}

size_t PtUsbFileImpl::position() const {
    if (!_file) return 0;
    long p = ::ftell(_file);
    return p < 0 ? 0 : (size_t)p;
}

void PtUsbFileImpl::_getStat(struct stat* st) const {
    memset(st, 0, sizeof(*st));
    ::stat(_path, st);
}

size_t PtUsbFileImpl::size() const {
    struct stat st;
    _getStat(&st);
    return (size_t)st.st_size;
}

time_t PtUsbFileImpl::getLastWrite() {
    struct stat st;
    _getStat(&st);
    return st.st_mtime;
}

const char* PtUsbFileImpl::name() const {
    const char* n = strrchr(_path, '/');
    return n ? n + 1 : _path;
}

boolean PtUsbFileImpl::seekDir(long position) {
    if (!_dir) return false;
    ::seekdir(_dir, position);
    return true;
}

void PtUsbFileImpl::rewindDirectory() {
    if (_dir) ::rewinddir(_dir);
}

// openNextFile: return a FileImpl for the next directory entry
FileImplPtr PtUsbFileImpl::openNextFile(const char* mode) {
    if (!_dir) return FileImplPtr();
    while (true) {
        struct dirent* de = ::readdir(_dir);
        if (!de) return FileImplPtr();
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char childPath[512];
        size_t len = strlen(_path);
        if (len > 0 && _path[len - 1] == '/') {
            snprintf(childPath, sizeof(childPath), "%s%s", _path, de->d_name);
        } else {
            snprintf(childPath, sizeof(childPath), "%s/%s", _path, de->d_name);
        }

        bool isDir = (de->d_type == DT_DIR);
        if (isDir) {
            DIR* childDir = ::opendir(childPath);
            if (!childDir) continue;  // skip inaccessible dirs
            return std::make_shared<PtUsbFileImpl>(childPath, childDir);
        } else {
            return std::make_shared<PtUsbFileImpl>(childPath, mode);
        }
    }
}

String PtUsbFileImpl::getNextFileName() {
    if (!_dir) return String();
    while (true) {
        struct dirent* de = ::readdir(_dir);
        if (!de) return String();
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        return String(de->d_name);
    }
}

String PtUsbFileImpl::getNextFileName(bool* isDir) {
    if (!_dir) { if (isDir) *isDir = false; return String(); }
    while (true) {
        struct dirent* de = ::readdir(_dir);
        if (!de) { if (isDir) *isDir = false; return String(); }
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        if (isDir) *isDir = (de->d_type == DT_DIR);
        return String(de->d_name);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbFSImpl implementation
// ─────────────────────────────────────────────────────────────────────────────

PtUsbFSImpl::PtUsbFSImpl(const char* mountPt) {
    strncpy(_mp, mountPt ? mountPt : "/usb", sizeof(_mp) - 1);
    _mp[sizeof(_mp) - 1] = '\0';
    FSImpl::mountpoint(_mp);
}

// Build a canonical POSIX path. Strip any existing mountpoint prefix so that
// paths returned by File::path() (which include the prefix) don't double it
// when passed back to SD.remove() / SD.rmdir() etc.
void PtUsbFSImpl::_makePath(const char* p, char* buf, size_t bufsz) const {
    const char* rel = p;
    size_t mpLen = strlen(_mp);
    if (mpLen > 0 &&
        strncmp(p, _mp, mpLen) == 0 &&
        (p[mpLen] == '/' || p[mpLen] == '\0')) {
        rel = p + mpLen;
    }
    if (!rel || rel[0] == '\0') {
        strncpy(buf, _mp, bufsz - 1);
        buf[bufsz - 1] = '\0';
    } else if (rel[0] == '/') {
        snprintf(buf, bufsz, "%s%s", _mp, rel);
    } else {
        snprintf(buf, bufsz, "%s/%s", _mp, rel);
    }
}

FileImplPtr PtUsbFSImpl::open(const char* path, const char* mode, const bool create) {
    char fullPath[512];
    _makePath(path, fullPath, sizeof(fullPath));

    struct stat st;
    bool pathExists = (::stat(fullPath, &st) == 0);

    if (pathExists && S_ISDIR(st.st_mode)) {
        DIR* d = ::opendir(fullPath);
        if (!d) return FileImplPtr();
        return std::make_shared<PtUsbFileImpl>(fullPath, d);
    }

    // Regular file — optionally create parent directory tree
    if (create && !pathExists) {
        char parent[512];
        strncpy(parent, fullPath, sizeof(parent) - 1);
        parent[sizeof(parent) - 1] = '\0';
        char* slash = strrchr(parent, '/');
        if (slash && slash != parent) {
            *slash = '\0';
            ::mkdir(parent, 0775);
        }
    }

    return std::make_shared<PtUsbFileImpl>(fullPath, mode);
}

bool PtUsbFSImpl::exists(const char* path) {
    char fullPath[512];
    _makePath(path, fullPath, sizeof(fullPath));
    struct stat st;
    return ::stat(fullPath, &st) == 0;
}

bool PtUsbFSImpl::rename(const char* from, const char* to) {
    char f[512], t[512];
    _makePath(from, f, sizeof(f));
    _makePath(to,   t, sizeof(t));
    return ::rename(f, t) == 0;
}

bool PtUsbFSImpl::remove(const char* path) {
    char fullPath[512];
    _makePath(path, fullPath, sizeof(fullPath));
    return ::unlink(fullPath) == 0;
}

bool PtUsbFSImpl::mkdir(const char* path) {
    char fullPath[512];
    _makePath(path, fullPath, sizeof(fullPath));
    return ::mkdir(fullPath, 0775) == 0;
}

bool PtUsbFSImpl::rmdir(const char* path) {
    char fullPath[512];
    _makePath(path, fullPath, sizeof(fullPath));
    return ::rmdir(fullPath) == 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// PtUsbStorageClass implementation
// ─────────────────────────────────────────────────────────────────────────────

PtUsbStorageClass::PtUsbStorageClass()
    : FS(FSImplPtr(new PtUsbFSImpl("/usb"))), _mounted(false)
{
    strncpy(_mountPoint, "/usb", sizeof(_mountPoint) - 1);
    _mountPoint[sizeof(_mountPoint) - 1] = '\0';
}

// ── USB host implementation ───────────────────────────────────────────────

#if PT_USB_MSC_AVAIL

bool PtUsbStorageClass::_initUsb(const char* mountpoint) {
    strncpy(s_mountPt, mountpoint, sizeof(s_mountPt) - 1);
    s_mountPt[sizeof(s_mountPt) - 1] = '\0';

    // 1. Install USB host library (idempotent — safe to call again)
    usb_host_config_t hostCfg = {};
    hostCfg.skip_phy_setup = false;
    hostCfg.intr_flags     = ESP_INTR_FLAG_LEVEL1;
    esp_err_t err = usb_host_install(&hostCfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        Serial.printf("[USB] usb_host_install failed: 0x%x\n", err);
        return false;
    }

    // 2. Start the USB host daemon task
    s_hostRunning = true;
    s_mountSem    = xSemaphoreCreateBinary();
    xTaskCreate(_pt_usb_host_task, "pt_usb_host", 4096, nullptr, 5, &s_hostTaskHdl);

    // 3. Install MSC class driver (creates its own background task)
    msc_host_driver_config_t mscCfg = {};
    mscCfg.create_backround_task = true;  // Note: ESP-IDF field name has a typo (single 'n')
    mscCfg.task_priority         = 5;
    mscCfg.stack_size            = 4096;
    mscCfg.core_id               = -1;    // no affinity
    mscCfg.callback              = _pt_msc_event_cb;
    mscCfg.callback_arg          = nullptr;
    err = msc_host_install(&mscCfg);
    if (err != ESP_OK) {
        Serial.printf("[USB] msc_host_install failed: 0x%x\n", err);
        s_hostRunning = false;
        return false;
    }

    // 4. Wait up to 3 s for a drive to mount
    bool mounted = (xSemaphoreTake(s_mountSem, pdMS_TO_TICKS(3000)) == pdTRUE)
                   && s_driveMounted;
    // Even if the semaphore timed out, the drive may already be mounted
    return mounted || s_driveMounted;
}

void PtUsbStorageClass::_deinitUsb() {
    if (s_vfsHandle) {
        msc_host_vfs_unregister(s_vfsHandle);
        s_vfsHandle = nullptr;
    }
    msc_host_uninstall();
    s_driveMounted = false;
    s_mscDevice    = nullptr;
    s_hostRunning  = false;
    if (s_mountSem) {
        vSemaphoreDelete(s_mountSem);
        s_mountSem = nullptr;
    }
}

bool PtUsbStorageClass::begin(uint8_t /*csPin*/, SPIClass& /*spi*/,
                               uint32_t /*freq*/, const char* mountpoint,
                               uint8_t /*maxFiles*/, bool /*format*/)
{
    if (_mounted) return true;    // already mounted

    const char* mp = (mountpoint && *mountpoint) ? mountpoint : "/usb";
    _mounted = _initUsb(mp);

    if (_mounted) {
        strncpy(_mountPoint, mp, sizeof(_mountPoint) - 1);
        // Rebuild the FSImpl with the correct mountpoint so FS::open() works
        _impl = FSImplPtr(new PtUsbFSImpl(mp));
        Serial.printf("[USB] Storage ready at %s\n", mp);
    } else {
        Serial.println("[USB] No USB drive found within timeout");
    }
    return _mounted;
}

void PtUsbStorageClass::end() {
    if (!_mounted) return;
    _deinitUsb();
    _mounted = false;
}

#else // PT_USB_MSC_AVAIL == 0  ── compile-time stub ────────────────────────

bool PtUsbStorageClass::begin(uint8_t /*csPin*/, SPIClass& /*spi*/,
                               uint32_t /*freq*/, const char* /*mountpoint*/,
                               uint8_t /*maxFiles*/, bool /*format*/)
{
    Serial.println("[USB] USB MSC host unavailable — missing usb/msc_host_vfs.h");
    return false;
}

void PtUsbStorageClass::end() { _mounted = false; }

#endif // PT_USB_MSC_AVAIL

// Global instance referenced by the #define SD ptSD in pt_tft_compat.h
PtUsbStorageClass ptSD;

#endif // PANDATOUCH

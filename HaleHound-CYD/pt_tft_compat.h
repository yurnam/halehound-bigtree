// ═══════════════════════════════════════════════════════════════════════════
// pt_tft_compat.h  —  TFT_eSPI compatibility shim for PandaTouch
//
// Pre-included via  build_src_flags: -include pt_tft_compat.h  so it applies
// only to project source files, not to library compilation units.
// Provides the same  TFT_eSPI  class API used throughout HaleHound-CYD but
// backed by Arduino_GFX (RGB-parallel LCD) and TAMC_GT911 (I²C touch).
//
// Defines  _TFT_eSPIH_  (the actual include guard in TFT_eSPI.h v2.5.43 —
// note: no underscore before H) so that any  #include <TFT_eSPI.h>  in
// project source files is blocked and our replacement class is used instead.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

// Claim the TFT_eSPI header guard so that any later #include <TFT_eSPI.h>
// in source files skips the real library class (we provide our own below).
// NOTE: TFT_eSPI.h v2.5.43 uses _TFT_eSPIH_ (no underscore before H) as its
// guard — this is the macro we must define.  _TFT_eSPI_H_ (with underscore)
// is a different macro and has no effect on TFT_eSPI.h's include guard.
#define _TFT_eSPIH_

// ── Standard includes ───────────────────────────────────────────────────────
#include <Arduino.h>
#include <driver/ledc.h>
#include <esp_heap_caps.h>
#include <stdarg.h>

// ── Arduino_GFX (RGB-parallel display + GFXfont type) ──────────────────────
#include <Arduino_GFX_Library.h>

// ── Undefine Arduino_GFX short-name colour macros ──────────────────────────
// Arduino_GFX.h defines generic colour names (RED, GREEN, BLUE, …) as
// function-like macro calls such as  #define RED RGB565(255,0,0).
// These clash with  const uint16_t RED = …  declarations in shared.h and
// with the #define aliases in that file, causing compile errors in every
// project source file.  We keep the RGB565_* long names; only the short
// aliases are removed here.  The TFT_* palette added by our class below
// covers all colours the firmware actually references.
#undef RED
#undef GREEN
#undef BLUE
#undef BLACK
#undef WHITE
#undef GRAY
#undef ORANGE
#undef YELLOW
#undef CYAN
#undef MAGENTA
#undef PURPLE
#undef PINK

// ── GT911 capacitive touch ──────────────────────────────────────────────────
#include <TAMC_GT911.h>

// ── PandaTouch hardware pin definitions ────────────────────────────────────
#include "pt_config.h"

// ── Free fonts bundled with the TFT_eSPI package ──────────────────────────
// These are referenced by subghz_attacks, bluetooth_attacks, gps_module, etc.
// The files define plain GFXfont data structures — compatible with Arduino_GFX.
#include <Fonts/GFXFF/FreeMono9pt7b.h>
#include <Fonts/GFXFF/FreeMonoBold9pt7b.h>
#include <Fonts/GFXFF/FreeMonoBold12pt7b.h>
#include <Fonts/GFXFF/FreeMonoBold18pt7b.h>

// ── LEDC / backlight compatibility macros ─────────────────────────────────
// The shim's _initBacklight() configures LEDC_CHANNEL_0 directly via the
// ESP-IDF API (11-bit resolution, 30 kHz, LEDC_TIMER_1).  The Arduino-layer
// ledcSetup()/ledcAttachPin() calls in setup() would conflict with that
// configuration and, if allowed to execute, also reconfigure the GPIO matrix
// for the backlight pin which can disrupt the IDF-level setup.
// Solution:
//   • ledcSetup / ledcAttachPin → no-ops (backlight already configured)
//   • ledcWrite(ch, 8-bit duty) → IDF ledc_set_duty (scales 0-255 → 0-2047)
//     so brightness changes throughout the firmware continue to work.
static inline void _pt_set_brightness(uint32_t duty8) {
    uint32_t duty11 = duty8 * 2047UL / 255UL;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty11);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
#define ledcSetup(ch, freq, bits)   ((void)0)
#define ledcAttachPin(pin, ch)      ((void)0)
#define ledcWrite(ch, duty)         _pt_set_brightness((uint32_t)(duty))

// ── TFT colour constants (RGB565) ──────────────────────────────────────────
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFE19
#define TFT_BROWN       0x9A60
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#define TFT_SKYBLUE     0x867D
#define TFT_VIOLET      0x915C

// ── Text datum constants (mirrors TFT_eSPI) ─────────────────────────────────
#define TL_DATUM  0   // Top Left       (default)
#define TC_DATUM  1   // Top Centre
#define TR_DATUM  2   // Top Right
#define ML_DATUM  3   // Middle Left
#define MC_DATUM  4   // Middle Centre
#define MR_DATUM  5   // Middle Right
#define BL_DATUM  6   // Bottom Left
#define BC_DATUM  7   // Bottom Centre
#define BR_DATUM  8   // Bottom Right

// ── Misc TFT_eSPI compatibility ─────────────────────────────────────────────
#ifndef PROGMEM
#define PROGMEM
#endif

// ═══════════════════════════════════════════════════════════════════════════
// TFT_eSPI compatibility class
// ═══════════════════════════════════════════════════════════════════════════

class TFT_eSPI {
public:
    // ── Constructor ───────────────────────────────────────────────────────
    TFT_eSPI(int16_t /*w*/ = 800, int16_t /*h*/ = 480)
        : _gfx(nullptr), _touch(nullptr),
          _rotation(1), _textDatum(TL_DATUM),
          _fgColor(TFT_WHITE), _bgColor(TFT_BLACK), _bgColorSet(false),
          _textSize(1), _freeFont(nullptr), _initialized(false) {}

    // ── Initialisation ────────────────────────────────────────────────────

    void init()  { _init(); }
    void begin() { _init(); }

    // ── Screen dimensions ─────────────────────────────────────────────────

    int16_t width()  { return _gfx ? (int16_t)_gfx->width()  : PT_LCD_H_RES; }
    int16_t height() { return _gfx ? (int16_t)_gfx->height() : PT_LCD_V_RES; }

    // ── Rotation ─────────────────────────────────────────────────────────
    void    setRotation(uint8_t r) { _rotation = r; if (_gfx) _gfx->setRotation(r); }
    uint8_t getRotation()          { return _rotation; }

    // ── Basic drawing ────────────────────────────────────────────────────

    void fillScreen(uint16_t color) {
        if (_gfx) _gfx->fillScreen(color);
    }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        if (_gfx) _gfx->fillRect((int16_t)x,(int16_t)y,(int16_t)w,(int16_t)h,color);
    }

    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        if (_gfx) _gfx->drawRect((int16_t)x,(int16_t)y,(int16_t)w,(int16_t)h,color);
    }

    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t r, uint16_t color) {
        if (_gfx) _gfx->fillRoundRect((int16_t)x,(int16_t)y,
                                       (int16_t)w,(int16_t)h,(int16_t)r,color);
    }

    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t r, uint16_t color) {
        if (_gfx) _gfx->drawRoundRect((int16_t)x,(int16_t)y,
                                       (int16_t)w,(int16_t)h,(int16_t)r,color);
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
        if (_gfx) _gfx->drawLine((int16_t)x0,(int16_t)y0,
                                  (int16_t)x1,(int16_t)y1,color);
    }

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
        if (_gfx) _gfx->drawFastHLine((int16_t)x,(int16_t)y,(int16_t)w,color);
    }

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
        if (_gfx) _gfx->drawFastVLine((int16_t)x,(int16_t)y,(int16_t)h,color);
    }

    void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
        if (_gfx) _gfx->fillCircle((int16_t)x,(int16_t)y,(int16_t)r,color);
    }

    void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
        if (_gfx) _gfx->drawCircle((int16_t)x,(int16_t)y,(int16_t)r,color);
    }

    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                      int32_t x2, int32_t y2, uint16_t color) {
        if (_gfx) _gfx->fillTriangle((int16_t)x0,(int16_t)y0,
                                      (int16_t)x1,(int16_t)y1,
                                      (int16_t)x2,(int16_t)y2,color);
    }

    void drawPixel(int32_t x, int32_t y, uint16_t color) {
        if (_gfx) _gfx->drawPixel((int16_t)x,(int16_t)y,color);
    }

    // Monochrome bitmap — foreground colour only (transparent background)
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
                    int16_t w, int16_t h, uint16_t fgcolor) {
        if (_gfx) _gfx->drawBitmap(x, y, bitmap, w, h, fgcolor);
    }

    // Monochrome bitmap — foreground + background colours
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
                    int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor) {
        if (_gfx) _gfx->drawBitmap(x, y, bitmap, w, h, fgcolor, bgcolor);
    }

    // ── Colour helper ─────────────────────────────────────────────────────

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint16_t)(r & 0xF8) << 8) |
               ((uint16_t)(g & 0xFC) << 3) |
               (b >> 3);
    }

    // ── Display control ───────────────────────────────────────────────────

    void invertDisplay(bool /*i*/) { /* no-op: RGB panel does not support inversion */ }
    void writecommand(uint8_t /*c*/) {}
    void writedata(uint8_t /*d*/)    {}

    // ── Touch ─────────────────────────────────────────────────────────────
    // getTouch() is used in touch_buttons.cpp to read raw touch coordinates.
    // The threshold parameter is ignored (GT911 is capacitive).

    bool getTouch(uint16_t *x, uint16_t *y, uint16_t /*threshold*/ = 0) {
        if (!_touch) return false;
        _touch->read();
        if (_touch->isTouched && _touch->touches > 0) {
            *x = (uint16_t)_touch->points[0].x;
            *y = (uint16_t)_touch->points[0].y;
            return true;
        }
        return false;
    }

    // setTouch / calibrateTouch: no-op — GT911 needs no resistive calibration
    void setTouch(uint16_t * /*data*/) {}
    bool calibrateTouch(uint16_t * /*params*/,
                        uint16_t /*fg*/, uint16_t /*bg*/,
                        uint8_t  /*size*/) { return true; }

    // ── Text — cursor and colour ──────────────────────────────────────────

    void setCursor(int16_t x, int16_t y) {
        if (_gfx) _gfx->setCursor(x, y);
    }

    void setTextColor(uint16_t fgcolor) {
        _fgColor    = fgcolor;
        _bgColorSet = false;
        if (_gfx) _gfx->setTextColor(fgcolor);
    }

    void setTextColor(uint16_t fgcolor, uint16_t bgcolor) {
        _fgColor    = fgcolor;
        _bgColor    = bgcolor;
        _bgColorSet = true;
        if (_gfx) _gfx->setTextColor(fgcolor, bgcolor);
    }

    void setTextSize(uint8_t s) {
        _textSize = s ? s : 1;
        if (_gfx) _gfx->setTextSize(_textSize);
    }

    // setTextFont: map TFT_eSPI font numbers to approximate text sizes
    void setTextFont(uint8_t font) {
        _freeFont = nullptr;
        if (_gfx) _gfx->setFont(nullptr);
        switch (font) {
            case 1: setTextSize(1); break;
            case 2: setTextSize(1); break;
            case 4: setTextSize(2); break;
            case 6: setTextSize(3); break;
            case 7: setTextSize(3); break;
            case 8: setTextSize(4); break;
            default: setTextSize(1); break;
        }
    }

    // setFreeFont: selects an Adafruit GFXfont (nullptr = return to built-in)
    void setFreeFont(const GFXfont *f) {
        _freeFont = f;
        if (_gfx) _gfx->setFont(f);
    }

    void setTextDatum(uint8_t datum) { _textDatum = datum; }

    // ── Text measurement ─────────────────────────────────────────────────

    int16_t textWidth(const char *str, uint8_t /*font*/ = 1) {
        if (!_gfx || !str) return (int16_t)(strlen(str) * 6);
        int16_t x1, y1;
        uint16_t w, h;
        _gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return (int16_t)w;
    }

    int16_t textWidth(const String &str, uint8_t font = 1) {
        return textWidth(str.c_str(), font);
    }

    // ── drawString: draw text respecting the current text datum ───────────

    int16_t drawString(const char *str, int32_t x, int32_t y, uint8_t font = 1) {
        if (!_gfx || !str) return (int16_t)x;

        // Apply font for this call
        if (_freeFont == nullptr) setTextFont(font);

        _applyTextColor();

        // Measure text for datum alignment
        int16_t x1, y1;
        uint16_t tw, th;
        _gfx->getTextBounds(str, 0, 0, &x1, &y1, &tw, &th);

        // Datum-adjusted top-left cursor
        int16_t ax = (int16_t)x, ay = (int16_t)y;
        _applyDatum(ax, ay, (int16_t)tw, (int16_t)th);

        if (_freeFont) {
            // For free fonts the GFX cursor is at the text baseline
            _gfx->setCursor(ax - x1, ay - y1);
        } else {
            // For built-in fonts the GFX cursor is the top-left corner
            _gfx->setCursor(ax, ay);
        }
        _gfx->print(str);
        return ax + (int16_t)tw;
    }

    int16_t drawString(const String &str, int32_t x, int32_t y, uint8_t font = 1) {
        return drawString(str.c_str(), x, y, font);
    }

    // ── print / println / printf passthrough ─────────────────────────────

    size_t print(const char *str)    { return _gfx ? _gfx->print(str)    : 0; }
    size_t print(const String &str)  { return _gfx ? _gfx->print(str)    : 0; }
    size_t print(int            v, int base=10){ return _gfx ? _gfx->print(v,base) : 0; }
    size_t print(long           v, int base=10){ return _gfx ? _gfx->print(v,base) : 0; }
    size_t print(unsigned       v, int base=10){ return _gfx ? _gfx->print(v,base) : 0; }
    size_t print(unsigned long  v, int base=10){ return _gfx ? _gfx->print(v,base) : 0; }
    size_t print(float  v, int d=2)            { return _gfx ? _gfx->print(v,d)    : 0; }
    size_t print(double v, int d=2)            { return _gfx ? _gfx->print(v,d)    : 0; }
    size_t print(char   c)                     { return _gfx ? _gfx->print(c)      : 0; }

    size_t println(const char *str)            { return _gfx ? _gfx->println(str)   : 0; }
    size_t println(const String &str)          { return _gfx ? _gfx->println(str)   : 0; }
    size_t println(int           v, int b=10)  { return _gfx ? _gfx->println(v,b)   : 0; }
    size_t println(long          v, int b=10)  { return _gfx ? _gfx->println(v,b)   : 0; }
    size_t println(unsigned      v, int b=10)  { return _gfx ? _gfx->println(v,b)   : 0; }
    size_t println(unsigned long v, int b=10)  { return _gfx ? _gfx->println(v,b)   : 0; }
    size_t println(float  v, int d=2)          { return _gfx ? _gfx->println(v,d)   : 0; }
    size_t println()                           { return _gfx ? _gfx->println()       : 0; }

    size_t printf(const char *fmt, ...) {
        if (!_gfx) return 0;
        char buf[256];
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        return _gfx->print(buf);
    }

private:
    Arduino_RGB_Display *_gfx;
    TAMC_GT911          *_touch;
    uint8_t              _rotation;
    uint8_t              _textDatum;
    uint16_t             _fgColor;
    uint16_t             _bgColor;
    bool                 _bgColorSet;
    uint8_t              _textSize;
    const GFXfont       *_freeFont;
    bool                 _initialized;

    // ── Hardware initialisation (called once from init()/begin()) ─────────
    void _init() {
        if (_initialized) return;

        // LCD hard reset
        pinMode(PT_LCD_RESET_PIN, OUTPUT);
        digitalWrite(PT_LCD_RESET_PIN, LOW);
        delay(100);
        digitalWrite(PT_LCD_RESET_PIN, HIGH);
        delay(10);

        // Backlight via ESP-IDF LEDC
        _initBacklight(100);

        // Create the RGB panel and display objects in static storage so they
        // outlive this call and are shared by all TFT_eSPI instances.
        static Arduino_ESP32RGBPanel rgbpanel(
            PT_LCD_DE_PIN,    PT_LCD_VSYNC_PIN, PT_LCD_HSYNC_PIN, PT_LCD_PCLK_PIN,
            PT_LCD_B3_PIN, PT_LCD_B4_PIN, PT_LCD_B5_PIN, PT_LCD_B6_PIN, PT_LCD_B7_PIN,
            PT_LCD_G2_PIN, PT_LCD_G3_PIN, PT_LCD_G4_PIN, PT_LCD_G5_PIN,
            PT_LCD_G6_PIN, PT_LCD_G7_PIN,
            PT_LCD_R3_PIN, PT_LCD_R4_PIN, PT_LCD_R5_PIN, PT_LCD_R6_PIN, PT_LCD_R7_PIN,
            0 /*hsync_pol*/, PT_LCD_HSYNC_PULSE_WIDTH,
            PT_LCD_HSYNC_BACK_PORCH, PT_LCD_HSYNC_FRONT_PORCH,
            0 /*vsync_pol*/, PT_LCD_VSYNC_PULSE_WIDTH,
            PT_LCD_VSYNC_BACK_PORCH, PT_LCD_VSYNC_FRONT_PORCH,
            1 /*pclk_active_neg*/,
            PT_LCD_PCLK_HZ, false /*useBigEndian*/
        );
        static Arduino_RGB_Display gfx(PT_LCD_H_RES, PT_LCD_V_RES,
                                       &rgbpanel, 0, true);
        _gfx = &gfx;
        bool ok = _gfx->begin();
        if (!ok) {
            // Most common cause: PSRAM not accessible — ps_malloc(768KB) failed.
            // Ensure board_build.arduino.memory_type = qio_opi and
            // espressif32@6.12.0+ are used so OPI PSRAM initialises correctly.
            Serial.println("[TFT] ERROR: Arduino_RGB_Display::begin() failed — "
                           "check PSRAM (need espressif32@6.12.0+, memory_type=qio_opi)");
            return;
        }
        Serial.println("[TFT] RGB display initialised OK");
        _gfx->fillScreen(TFT_BLACK);
        _gfx->setRotation(_rotation);
        _gfx->setTextSize(_textSize);

        // GT911 touch
        static TAMC_GT911 touch(PT_I2C0_SDA_PIN, PT_I2C0_SCL_PIN,
                                PT_GT911_IRQ_PIN, PT_GT911_RST_PIN,
                                PT_LCD_H_RES,    PT_LCD_V_RES);
        _touch = &touch;
        _touch->begin();
        _touch->setRotation(ROTATION_NORMAL);

        _initialized = true;
    }

    // ── Backlight initialisation ──────────────────────────────────────────
    static void _initBacklight(uint8_t percent) {
        ledc_timer_config_t timer_cfg = {};
        timer_cfg.speed_mode      = LEDC_LOW_SPEED_MODE;
        timer_cfg.duty_resolution = LEDC_TIMER_11_BIT;
        timer_cfg.timer_num       = LEDC_TIMER_1;
        timer_cfg.freq_hz         = PT_LCD_BL_FREQUENCY_HZ;
        timer_cfg.clk_cfg         = LEDC_USE_APB_CLK;
        ledc_timer_config(&timer_cfg);

        ledc_channel_config_t ch_cfg = {};
        ch_cfg.gpio_num   = PT_LCD_BL_PIN;
        ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
        ch_cfg.channel    = LEDC_CHANNEL_0;
        ch_cfg.timer_sel  = LEDC_TIMER_1;
        ch_cfg.duty       = 0;
        ch_cfg.hpoint     = 0;
        ledc_channel_config(&ch_cfg);

        ledc_fade_func_install(0);

        uint32_t duty = (uint32_t)(((float)percent / 100.0f) *
                        ((1 << LEDC_TIMER_11_BIT) - 1));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }

    // ── Apply current text colour to the GFX object ───────────────────────
    void _applyTextColor() {
        if (!_gfx) return;
        if (_bgColorSet) _gfx->setTextColor(_fgColor, _bgColor);
        else             _gfx->setTextColor(_fgColor);
    }

    // ── Adjust (ax, ay) from datum-reference point to top-left of text ────
    void _applyDatum(int16_t &ax, int16_t &ay, int16_t tw, int16_t th) {
        switch (_textDatum) {
            case TC_DATUM: ax -= tw / 2;               break;
            case TR_DATUM: ax -= tw;                   break;
            case ML_DATUM:               ay -= th / 2; break;
            case MC_DATUM: ax -= tw / 2; ay -= th / 2; break;
            case MR_DATUM: ax -= tw;     ay -= th / 2; break;
            case BL_DATUM:               ay -= th;     break;
            case BC_DATUM: ax -= tw / 2; ay -= th;     break;
            case BR_DATUM: ax -= tw;     ay -= th;     break;
            default: break; // TL_DATUM: (ax, ay) already is top-left
        }
    }
};

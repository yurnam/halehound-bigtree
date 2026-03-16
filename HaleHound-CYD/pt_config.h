// ═══════════════════════════════════════════════════════════════════════════
// HaleHound PandaTouch Hardware Configuration
// BigTreeTech PandaTouch — ESP32-S3, 7" 800×480 RGB LCD, GT911 capacitive touch
//
// WiFi + Bluetooth only.  No SPI radio modules are wired — the PandaTouch
// exposes only a secondary I²C header which is not used in this release.
//
// To add external radios in a future revision, use the I²C1 expansion header
// (GPIO 3 / GPIO 4) for additional I²C peripherals.
// ═══════════════════════════════════════════════════════════════════════════

#ifndef PT_CONFIG_H
#define PT_CONFIG_H

// ── Identity ────────────────────────────────────────────────────────────────
#define CYD_BOARD_NAME  "HaleHound-PandaTouch 7\""
#define FW_EDITION      "PandaTouch Edition"
#define FW_DEVICE       "HaleHound-PandaTouch"

// ── Screen resolution (landscape) ──────────────────────────────────────────
#define CYD_SCREEN_WIDTH   800
#define CYD_SCREEN_HEIGHT  480

// ── LCD RGB-parallel panel (from BigTreeTech PINOUT.md) ────────────────────
#define PT_LCD_H_RES             800
#define PT_LCD_V_RES             480
#define PT_LCD_HSYNC_PULSE_WIDTH   4
#define PT_LCD_VSYNC_PULSE_WIDTH   4
#define PT_LCD_HSYNC_BACK_PORCH   16
#define PT_LCD_VSYNC_BACK_PORCH   32
#define PT_LCD_HSYNC_FRONT_PORCH  16
#define PT_LCD_VSYNC_FRONT_PORCH  32
#define PT_LCD_PCLK_HZ       14800000

#define PT_LCD_PCLK_PIN   5
#define PT_LCD_DE_PIN    38
#define PT_LCD_HSYNC_PIN -1
#define PT_LCD_VSYNC_PIN -1

// Blue data bits
#define PT_LCD_B3_PIN 17
#define PT_LCD_B4_PIN 18
#define PT_LCD_B5_PIN 48
#define PT_LCD_B6_PIN 47
#define PT_LCD_B7_PIN 39
// Green data bits
#define PT_LCD_G2_PIN 11
#define PT_LCD_G3_PIN 12
#define PT_LCD_G4_PIN 13
#define PT_LCD_G5_PIN 14
#define PT_LCD_G6_PIN 15
#define PT_LCD_G7_PIN 16
// Red data bits
#define PT_LCD_R3_PIN  6
#define PT_LCD_R4_PIN  7
#define PT_LCD_R5_PIN  8
#define PT_LCD_R6_PIN  9
#define PT_LCD_R7_PIN 10

#define PT_LCD_RESET_PIN     46
#define PT_LCD_BL_PIN        21
#define PT_LCD_BL_FREQUENCY_HZ 30000
#define PT_LCD_RENDER_BOUNCE_LINES 10

// ── GT911 Capacitive Touch (I²C0) ──────────────────────────────────────────
#define PT_I2C0_SPEED      400000
#define PT_I2C0_SCL_PIN         1
#define PT_I2C0_SDA_PIN         2
#define PT_GT911_IRQ_PIN       40
#define PT_GT911_RST_PIN       41

// ── BOOT button ─────────────────────────────────────────────────────────────
// GPIO0 is safe to use as the BOOT button on PandaTouch (no PA radio conflict)
#define BOOT_BUTTON        0
#define BOOT_BUTTON_USABLE 1
#define IS_BOOT_PRESSED()  (digitalRead(BOOT_BUTTON) == LOW)

// ── Touch button zones (landscape 800×480) ──────────────────────────────────
// Zones are scaled proportionally from the CYD 2.8" 240×320 reference layout.
// UP: top-left  DOWN: bottom-left  SELECT: centre  BACK: top-right

#define TOUCH_BTN_UP_X1     0
#define TOUCH_BTN_UP_Y1     0
#define TOUCH_BTN_UP_X2   200
#define TOUCH_BTN_UP_Y2    90

#define TOUCH_BTN_DOWN_X1   0
#define TOUCH_BTN_DOWN_Y1 390
#define TOUCH_BTN_DOWN_X2 200
#define TOUCH_BTN_DOWN_Y2 480

#define TOUCH_BTN_SEL_X1  267
#define TOUCH_BTN_SEL_Y1  195
#define TOUCH_BTN_SEL_X2  533
#define TOUCH_BTN_SEL_Y2  285

#define TOUCH_BTN_BACK_X1  600
#define TOUCH_BTN_BACK_Y1    0
#define TOUCH_BTN_BACK_X2  800
#define TOUCH_BTN_BACK_Y2   90

// ── Feature flags — WiFi + BT only ─────────────────────────────────────────
// No SPI radio modules: PandaTouch lacks free GPIO pins for an SPI bus.
// WiFi and Bluetooth are provided natively by the ESP32-S3.
#define CYD_HAS_CC1101    0   // No SubGHz radio
#define CYD_HAS_NRF24     0   // No 2.4 GHz radio
#define CYD_HAS_GPS       0   // No GPS module
#define CYD_HAS_SDCARD    0   // No SD card
#define CYD_HAS_RGB_LED   0   // No RGB LED
#define CYD_HAS_SPEAKER   0   // No speaker
#define CYD_HAS_PCF8574   0   // No I²C button expander
#define CYD_HAS_PN532     0   // No NFC/RFID module
#define CYD_HAS_SERIAL_MON 0  // Serial-monitor passthrough disabled

// ── UART pins (ESP32-S3 debug UART) ────────────────────────────────────────
#define UART_MON_P1_RX        43
#define UART_MON_P1_TX        44
#define UART_MON_SPK_RX       -1
#define UART_MON_DEFAULT_BAUD 115200

// ── GPS (disabled) ──────────────────────────────────────────────────────────
#define GPS_RX_PIN -1
#define GPS_TX_PIN -1
#define GPS_BAUD   9600

// ── SPI bus stubs (needed to compile spi_manager; unused at runtime) ────────
#define VSPI_SCK  -1
#define VSPI_MOSI -1
#define VSPI_MISO -1
#define SD_CS     -1
#define SD_SCK    VSPI_SCK
#define SD_MOSI   VSPI_MOSI
#define SD_MISO   VSPI_MISO

#define RADIO_SPI_SCK  VSPI_SCK
#define RADIO_SPI_MOSI VSPI_MOSI
#define RADIO_SPI_MISO VSPI_MISO

// ── Radio CS stubs ──────────────────────────────────────────────────────────
#define CC1101_CS    -1
#define CC1101_GDO0  -1
#define CC1101_GDO2  -1
#define CC1101_SCK   RADIO_SPI_SCK
#define CC1101_MOSI  RADIO_SPI_MOSI
#define CC1101_MISO  RADIO_SPI_MISO
#define TX_PIN       -1
#define RX_PIN       -1
#define CC1101_TX_EN -1
#define CC1101_RX_EN -1

#define NRF24_CSN  -1
#define NRF24_CE   -1
#define NRF24_IRQ  -1
#define NRF24_SCK  RADIO_SPI_SCK
#define NRF24_MOSI RADIO_SPI_MOSI
#define NRF24_MISO RADIO_SPI_MISO

#define PN532_CS   -1
#define PN532_SCK  RADIO_SPI_SCK
#define PN532_MOSI RADIO_SPI_MOSI
#define PN532_MISO RADIO_SPI_MISO

// ── Layout scaling macros (relative to CYD 2.8" 240×320 reference) ─────────
#define SCALE_Y(y)  ((y) * CYD_SCREEN_HEIGHT / 320)
#define SCALE_X(x)  ((x) * CYD_SCREEN_WIDTH  / 240)
#define SCALE_W(w)  ((w) * CYD_SCREEN_WIDTH  / 240)
#define SCALE_H(h)  ((h) * CYD_SCREEN_HEIGHT / 320)

// ── Menu / text layout (matches E32R35T style at the larger resolution) ─────
#define MENU_BTN_W         (CYD_SCREEN_WIDTH / 2 - 20)
#define MENU_BTN_H         SCALE_H(60)
#define MENU_ICON_OFFSET_X ((MENU_BTN_W - 16) / 2)
#define MENU_TEXT_OFFSET_Y SCALE_H(30)

#define SUBMENU_Y_START    SCALE_Y(30)
#define SUBMENU_Y_SPACING  SCALE_Y(28)
#define SUBMENU_LAST_GAP   SCALE_Y(10)
#define SUBMENU_TOUCH_W    (CYD_SCREEN_WIDTH - 20)
#define SUBMENU_TOUCH_H    SCALE_Y(25)

#define TEXT_SIZE_BODY      1
#define TEXT_SIZE_SMALL     1
#define TEXT_LINE_H        12
#define TEXT_LINE_H_SMALL  12
#define TEXT_CHAR_W         6

// ── Debug ───────────────────────────────────────────────────────────────────
#define CYD_DEBUG      1
#define CYD_DEBUG_BAUD 115200

// ── Firmware version ────────────────────────────────────────────────────────
#define FW_VERSION      "v3.4.0"
#define FW_FULL_VERSION FW_VERSION " " FW_EDITION

// ── Backlight alias (code references CYD_TFT_BL; shim drives it via LEDC) ──
#define CYD_TFT_BL      PT_LCD_BL_PIN

// ── Icon bar (top navigation strip) — same Y values as CYD_35 (480px tall) ─
#define ICON_BAR_TOP      19
#define ICON_BAR_Y        20
#define ICON_BAR_BOTTOM   36
#define ICON_BAR_H        16
#define CONTENT_Y_START   38

// Icon bar touch zones — generous (large 7" screen, fat-finger friendly)
#define ICON_BAR_TOUCH_TOP     0
#define ICON_BAR_TOUCH_BOTTOM  55

// ── Padded content area ──────────────────────────────────────────────────────
#define CONTENT_PADDED_X    5
#define CONTENT_PADDED_W    (CYD_SCREEN_WIDTH - 10)
#define CONTENT_INNER_X     10
#define CONTENT_INNER_W     (CYD_SCREEN_WIDTH - 20)

// ── Graph / visualisation areas ──────────────────────────────────────────────
#define GRAPH_FULL_W        (CYD_SCREEN_WIDTH - 4)
#define GRAPH_PADDED_W      (CYD_SCREEN_WIDTH - 10)

// ── Menu layout ──────────────────────────────────────────────────────────────
#define MENU_COLUMN_W       (CYD_SCREEN_WIDTH / 2)
#define MENU_COL_LEFT_X     10
#define MENU_COL_RIGHT_X    (MENU_COL_LEFT_X + MENU_COLUMN_W)

// ── Dialog boxes ─────────────────────────────────────────────────────────────
#define DIALOG_W            (CYD_SCREEN_WIDTH - 20)
#define DIALOG_X            10
#define DIALOG_CENTER_X     (CYD_SCREEN_WIDTH / 2)

// ── Bottom area positions ─────────────────────────────────────────────────────
#define BOTTOM_HINT_Y       (CYD_SCREEN_HEIGHT - 45)
#define BOTTOM_NAV_Y        (CYD_SCREEN_HEIGHT - 33)

// ── Button bar ───────────────────────────────────────────────────────────────
#define BUTTON_BAR_Y        (CYD_SCREEN_HEIGHT - 37)
#define BUTTON_BAR_H        37
#define STATUS_LINE_Y       (CYD_SCREEN_HEIGHT - 18)
#define CONTENT_BOTTOM      (BUTTON_BAR_Y - 2)

#endif // PT_CONFIG_H

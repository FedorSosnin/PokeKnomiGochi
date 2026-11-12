#pragma once

#ifdef BOARD_KNOMI_V2

// BIGTREETECH Knomi V2 (ESP32-S3R8, 16 MB of flash). Pins from BTT's official firmware
// (firmware branch, src/pinout_knomi_v2.h). No SD, audio, RTC or battery.

// The game still draws on a 466x466 logical canvas; ScaledCanvas scales it down to the panel.
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define PANEL_SIZE 240

// GC9A01 240x240 SPI display, single-wire AW9346 backlight
#define LCD_MOSI 14
#define LCD_SCLK 18
#define LCD_CS 20
#define LCD_DC 19
#define LCD_RESET 21
#define LCD_BL 12

// CST816S touch over I2C
#define IIC_SDA 2
#define IIC_SCL 1
#define TP_INT 17
#define TP_RESET 16

// Tilt: accelerometer axis parallel to the screen width, and its sign
#define KNOMI_TILT_AXIS 0
#define KNOMI_TILT_SIGN -1.0f  // measured on the board: tilting left gives positive X

// BOOT button
#define KNOMI_BTN_PIN 0

#else  // Waveshare ESP32-S3-Touch-AMOLED-1.75

// Official Waveshare ESP32-S3-Touch-AMOLED-1.75 pins
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)

#define XPOWERS_CHIP_AXP2101

// AMOLED 466x466 display, CO5300 driver over QSPI
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Capacitive touch CST9217 over I2C
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// Audio ES8311. NOTE: the real MCLK is GPIO42 (verified on the board with the
// PlaneRadar2.0 project); the 16 that was listed was wrong. Even so the codec is
// configured with a clock derived from BCLK, so MCLK hardly matters.
#define I2S_MCK_IO 42
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

// TF slot (not used yet)
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41

#endif  // BOARD_KNOMI_V2

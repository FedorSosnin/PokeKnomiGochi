#pragma once

#ifdef BOARD_KNOMI_V2

// BIGTREETECH Knomi V2 (ESP32-S3R8, 16 MB de flash). Pines del firmware oficial de BTT
// (rama firmware, src/pinout_knomi_v2.h). Sin SD, audio, RTC ni bateria.

// El juego sigue dibujando en un lienzo logico de 466x466; ScaledCanvas lo reduce al panel.
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define PANEL_SIZE 240

// Pantalla GC9A01 240x240 por SPI, retroiluminacion AW9346 de un hilo
#define LCD_MOSI 14
#define LCD_SCLK 18
#define LCD_CS 20
#define LCD_DC 19
#define LCD_RESET 21
#define LCD_BL 12

// Tactil CST816S por I2C
#define IIC_SDA 2
#define IIC_SCL 1
#define TP_INT 17
#define TP_RESET 16

// Inclinar: eje del acelerometro paralelo al ancho de la pantalla, y su signo
#define KNOMI_TILT_AXIS 0
#define KNOMI_TILT_SIGN -1.0f  // medido en la placa: inclinar a la izquierda da X positivo

// Boton BOOT
#define KNOMI_BTN_PIN 0

#else  // Waveshare ESP32-S3-Touch-AMOLED-1.75

// Pines oficiales de la Waveshare ESP32-S3-Touch-AMOLED-1.75
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)

#define XPOWERS_CHIP_AXP2101

// Pantalla AMOLED 466x466, driver CO5300 por QSPI
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Táctil capacitivo CST9217 por I2C
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// Audio ES8311. NOTA: el MCLK real es GPIO42 (verificado en placa con el
// proyecto PlaneRadar2.0); el 16 que figuraba era erroneo. Aun asi el codec se
// configura con reloj derivado del BCLK, asi que el MCLK apenas importa.
#define I2S_MCK_IO 42
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

// Ranura TF (no usada todavía)
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41

#endif  // BOARD_KNOMI_V2

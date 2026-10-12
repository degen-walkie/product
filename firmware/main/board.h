/* Degen Walkie mainboard rev 7: pins and the chips around the ESP32-S3.
 * See hardware/mainboard/README.md for the full pin map. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9
#define PIN_LCD_CS 10
#define PIN_LCD_MOSI 11
#define PIN_LCD_SCK 12
#define PIN_LCD_DC 13
#define PIN_LCD_TE 14
#define PIN_BACKLIGHT 0     /* also the BOOT strap: start the PWM only after boot */
#define PIN_EXP_INT 1
#define PIN_WHEEL_A 3

#define LCD_W 320
#define LCD_H 480

/* TCA9555 port bits (port 0 = bits 0..7, port 1 = bits 8..15) */
#define EXP_LCD_RST (1u << 0)
#define EXP_TP_RST (1u << 1)
#define EXP_TP_INT (1u << 2)
#define EXP_PA_EN (1u << 3)
#define EXP_LED_R (1u << 5)       /* active low */
#define EXP_KEY_TRADE (1u << 6)   /* active low */
#define EXP_WHEEL_B (1u << 8)
#define EXP_WHEEL_SW (1u << 9)    /* active low */
#define EXP_AXP_IRQ (1u << 10)
#define EXP_LED_G (1u << 11)      /* active low */
#define EXP_CLIP_DET (1u << 12)
#define EXP_CLIP_RI (1u << 13)
#define EXP_CLIP_CHG_EN (1u << 14)
#define EXP_VIBE (1u << 15)

extern i2c_master_bus_handle_t g_i2c;
extern esp_lcd_panel_handle_t g_panel;
extern esp_lcd_panel_io_handle_t g_lcd_io;
extern esp_lcd_touch_handle_t g_touch;

void board_init(void);
void board_backlight(uint8_t percent);

/* TCA9555 */
uint16_t exp_read(void);
void exp_write(uint16_t mask, bool high);

/* AXP2101 */
int axp_battery_percent(void);
bool axp_charging(void);
bool axp_vbus(void);
int axp_power_key(void);   /* 0 none, 1 short press, 2 long press, since last call */

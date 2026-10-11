/* Board bring-up: I2C, AXP2101 power rails, TCA9555 expander, ST7796U display,
 * GT911 touch and the backlight.
 *
 * NOT YET VALIDATED ON HARDWARE. Register values follow the AXP2101 and TCA9555
 * datasheets and the rev 7 schematic; the display orientation, colour order and
 * the GT911 address need checking at bring-up (see docs in hardware/). */
#include "board.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board";

i2c_master_bus_handle_t g_i2c;
esp_lcd_panel_handle_t g_panel;
esp_lcd_panel_io_handle_t g_lcd_io;
esp_lcd_touch_handle_t g_touch;

static i2c_master_dev_handle_t s_axp, s_exp;
static uint16_t s_exp_out;

/* ---------------- I2C helpers ---------------- */

static esp_err_t wr(i2c_master_dev_handle_t d, uint8_t reg, uint8_t v)
{
    uint8_t b[2] = {reg, v};
    return i2c_master_transmit(d, b, 2, 50);
}

static int rd(i2c_master_dev_handle_t d, uint8_t reg)
{
    uint8_t v;
    if (i2c_master_transmit_receive(d, &reg, 1, &v, 1, 50) != ESP_OK) return -1;
    return v;
}

/* ---------------- AXP2101 ---------------- */

#define AXP_ADDR 0x34
#define AXP_STATUS1 0x00
#define AXP_STATUS2 0x01
#define AXP_ADC_CTRL 0x30
#define AXP_IRQ_EN1 0x41
#define AXP_IRQ_ST1 0x49
#define AXP_TS_CTRL 0x50
#define AXP_ICC 0x62
#define AXP_CV 0x64
#define AXP_LDO_EN 0x90
#define AXP_ALDO1_V 0x92
#define AXP_GAUGE 0xA4

static uint8_t ldo_mv(int mv) { return (uint8_t)((mv - 500) / 100); }

static void axp_init(void)
{
    /* DCDC1 (3V3 for the ESP32) is on from power-up; leave it alone. */
    wr(s_axp, AXP_ALDO1_V + 0, ldo_mv(3300));   /* ALDO1 AVDD: codec analog, mic */
    wr(s_axp, AXP_ALDO1_V + 1, ldo_mv(3300));   /* ALDO2 LCD_3V3: display + touch */
    wr(s_axp, AXP_ALDO1_V + 2, ldo_mv(2800));   /* ALDO3 CAM_2V8 */
    wr(s_axp, AXP_ALDO1_V + 3, ldo_mv(1200));   /* ALDO4 CAM_DVDD: 1.2 V, not 1.3 */
    int en = rd(s_axp, AXP_LDO_EN);
    wr(s_axp, AXP_LDO_EN, (uint8_t)((en < 0 ? 0 : en) | 0x0F));
    /* TS is tied to a fixed 10 k: stop the TS measurement or the charger sees a bad battery. */
    int adc = rd(s_axp, AXP_ADC_CTRL);
    if (adc >= 0) wr(s_axp, AXP_ADC_CTRL, (uint8_t)(adc & ~0x02));
    wr(s_axp, AXP_TS_CTRL, 0x10);
    wr(s_axp, AXP_CV, 0x03);    /* charge to 4.20 V */
    wr(s_axp, AXP_ICC, 0x0A);   /* 400 mA for the 1000 mAh cell */
    /* Power key short press interrupt */
    int irq = rd(s_axp, AXP_IRQ_EN1);
    if (irq >= 0) wr(s_axp, AXP_IRQ_EN1, (uint8_t)(irq | 0x08));
    wr(s_axp, AXP_IRQ_ST1, 0xFF);
    ESP_LOGI(TAG, "AXP2101 rails up, battery %d%%", axp_battery_percent());
}

int axp_battery_percent(void)
{
    int v = rd(s_axp, AXP_GAUGE);
    return v < 0 ? 0 : v > 100 ? 100 : v;
}

bool axp_charging(void)
{
    int v = rd(s_axp, AXP_STATUS2);
    return v >= 0 && ((v >> 5) & 0x3) == 0x1;
}

bool axp_vbus(void)
{
    int v = rd(s_axp, AXP_STATUS1);
    return v >= 0 && (v & 0x20);
}

bool axp_power_key_pressed(void)
{
    int v = rd(s_axp, AXP_IRQ_ST1);
    if (v > 0) wr(s_axp, AXP_IRQ_ST1, (uint8_t)v);
    return v > 0 && (v & 0x08);
}

/* ---------------- TCA9555 ---------------- */

#define EXP_ADDR 0x20

static void exp_init(void)
{
    /* inputs: TP_INT, KEY_TRADE, unused P04/P07; WHEEL_B, WHEEL_SW, AXP_IRQ, CLIP_DET, CLIP_RI */
    s_exp_out = EXP_LCD_RST | EXP_TP_RST | EXP_LED_R | EXP_LED_G;
    wr(s_exp, 0x02, (uint8_t)s_exp_out);
    wr(s_exp, 0x03, (uint8_t)(s_exp_out >> 8));
    wr(s_exp, 0x06, 0xD4);
    wr(s_exp, 0x07, 0x37);
}

uint16_t exp_read(void)
{
    uint8_t reg = 0x00, v[2] = {0xFF, 0xFF};
    i2c_master_transmit_receive(s_exp, &reg, 1, v, 2, 20);
    return (uint16_t)(v[0] | v[1] << 8);
}

void exp_write(uint16_t mask, bool high)
{
    if (high) s_exp_out |= mask;
    else s_exp_out &= (uint16_t)~mask;
    if (mask & 0x00FF) wr(s_exp, 0x02, (uint8_t)s_exp_out);
    if (mask & 0xFF00) wr(s_exp, 0x03, (uint8_t)(s_exp_out >> 8));
}

/* ---------------- display ---------------- */

static void lcd_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_LCD_MOSI, .miso_io_num = -1, .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = LCD_W * 40 * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC, .cs_gpio_num = PIN_LCD_CS, .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8, .spi_mode = 0, .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &g_lcd_io));
    /* Reset is on the expander, not a GPIO. */
    exp_write(EXP_LCD_RST, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    exp_write(EXP_LCD_RST, true);
    vTaskDelay(pdMS_TO_TICKS(120));
    esp_lcd_panel_dev_config_t dev = {.reset_gpio_num = -1, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, .bits_per_pixel = 16};
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(g_lcd_io, &dev, &g_panel));
    esp_lcd_panel_init(g_panel);
    esp_lcd_panel_invert_color(g_panel, true);
    esp_lcd_panel_disp_on_off(g_panel, true);
}

static void touch_init(void)
{
    exp_write(EXP_TP_RST, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    exp_write(EXP_TP_RST, true);
    vTaskDelay(pdMS_TO_TICKS(60));
    /* TP_INT is pulled up and only readable through the expander, so the GT911
       comes up on its backup address 0x14. */
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
    io_cfg.scl_speed_hz = 400000;
    if (esp_lcd_new_panel_io_i2c(g_i2c, &io_cfg, &io) != ESP_OK) return;
    esp_lcd_touch_config_t cfg = {
        .x_max = LCD_W, .y_max = LCD_H, .rst_gpio_num = GPIO_NUM_NC, .int_gpio_num = GPIO_NUM_NC,
    };
    if (esp_lcd_touch_new_i2c_gt911(io, &cfg, &g_touch) != ESP_OK) {
        ESP_LOGW(TAG, "GT911 not found on 0x14");
        g_touch = NULL;
    }
}

void board_backlight(uint8_t percent)
{
    uint32_t duty = (uint32_t)percent * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void backlight_init(void)
{
    ledc_timer_config_t t = {.speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
                             .timer_num = LEDC_TIMER_0, .freq_hz = 20000, .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&t);
    ledc_channel_config_t c = {.gpio_num = PIN_BACKLIGHT, .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0,
                               .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0};
    ledc_channel_config(&c);
}

void board_init(void)
{
    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0, .sda_io_num = PIN_I2C_SDA, .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &g_i2c));
    i2c_device_config_t d = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .scl_speed_hz = 400000};
    d.device_address = AXP_ADDR;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c, &d, &s_axp));
    d.device_address = EXP_ADDR;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c, &d, &s_exp));

    axp_init();
    exp_init();
    vTaskDelay(pdMS_TO_TICKS(20));  /* let LCD_3V3 settle */
    gpio_config_t a = {.pin_bit_mask = 1ULL << PIN_WHEEL_A, .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    gpio_config(&a);
    lcd_init();
    touch_init();
    backlight_init();
}

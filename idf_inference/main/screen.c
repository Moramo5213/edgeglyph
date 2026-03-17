#include "screen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GC9A01_MIN";

#define TFT_BL   3
#define TFT_DC   2
#define TFT_CS   10
#define TFT_RST  1
#define TFT_SCK  6
#define TFT_MOSI 7

#define LCD_HOST SPI2_HOST

#define BL_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define BL_LEDC_TIMER      LEDC_TIMER_0
#define BL_LEDC_CHANNEL    LEDC_CHANNEL_0
#define BL_LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define BL_LEDC_MAX_DUTY   ((1 << 10) - 1)
#define BL_LEDC_DUTY       (BL_LEDC_MAX_DUTY / 5)

static spi_device_handle_t spi;
DMA_ATTR static uint16_t line_buf[SCREEN_W];

typedef struct {
    uint8_t cmd;
    const uint8_t *data;
    uint8_t data_len;
    uint16_t delay_ms;
} lcd_init_cmd_t;

static const uint8_t d_eb_14[] = {0x14};
static const uint8_t d_84[] = {0x40};
static const uint8_t d_85[] = {0xFF};
static const uint8_t d_86[] = {0xFF};
static const uint8_t d_87[] = {0xFF};
static const uint8_t d_88[] = {0x0A};
static const uint8_t d_89[] = {0x21};
static const uint8_t d_8a[] = {0x00};
static const uint8_t d_8b[] = {0x80};
static const uint8_t d_8c[] = {0x01};
static const uint8_t d_8d[] = {0x01};
static const uint8_t d_8e[] = {0xFF};
static const uint8_t d_8f[] = {0xFF};
static const uint8_t d_b6[] = {0x00, 0x20};
static const uint8_t d_3a[] = {0x05};
static const uint8_t d_90[] = {0x08, 0x08, 0x08, 0x08};
static const uint8_t d_bd[] = {0x06};
static const uint8_t d_bc[] = {0x00};
static const uint8_t d_ff[] = {0x60, 0x01, 0x04};
static const uint8_t d_c3[] = {0x13};
static const uint8_t d_c4[] = {0x13};
static const uint8_t d_c9[] = {0x22};
static const uint8_t d_be[] = {0x11};
static const uint8_t d_e1[] = {0x10, 0x0E};
static const uint8_t d_df[] = {0x21, 0x0C, 0x02};
static const uint8_t d_f0[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
static const uint8_t d_f1[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
static const uint8_t d_f2[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
static const uint8_t d_f3[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
static const uint8_t d_ed[] = {0x1B, 0x0B};
static const uint8_t d_ae[] = {0x77};
static const uint8_t d_cd[] = {0x63};
static const uint8_t d_70[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
static const uint8_t d_e8[] = {0x34};
static const uint8_t d_62[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
static const uint8_t d_63[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
static const uint8_t d_64[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
static const uint8_t d_66[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
static const uint8_t d_67[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
static const uint8_t d_74[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
static const uint8_t d_98[] = {0x3E, 0x07};

static const lcd_init_cmd_t gc9a01_init[] = {
    {0xEF, NULL,    0, 0}, {0xEB, d_eb_14, 1, 0}, {0xFE, NULL,    0, 0}, {0xEF, NULL,    0, 0},
    {0xEB, d_eb_14, 1, 0}, {0x84, d_84,    1, 0}, {0x85, d_85,    1, 0}, {0x86, d_86,    1, 0},
    {0x87, d_87,    1, 0}, {0x88, d_88,    1, 0}, {0x89, d_89,    1, 0}, {0x8A, d_8a,    1, 0},
    {0x8B, d_8b,    1, 0}, {0x8C, d_8c,    1, 0}, {0x8D, d_8d,    1, 0}, {0x8E, d_8e,    1, 0},
    {0x8F, d_8f,    1, 0}, {0xB6, d_b6,    2, 0}, {0x3A, d_3a,    1, 0}, {0x90, d_90,    4, 0},
    {0xBD, d_bd,    1, 0}, {0xBC, d_bc,    1, 0}, {0xFF, d_ff,    3, 0}, {0xC3, d_c3,    1, 0},
    {0xC4, d_c4,    1, 0}, {0xC9, d_c9,    1, 0}, {0xBE, d_be,    1, 0}, {0xE1, d_e1,    2, 0},
    {0xDF, d_df,    3, 0}, {0xF0, d_f0,    6, 0}, {0xF1, d_f1,    6, 0}, {0xF2, d_f2,    6, 0},
    {0xF3, d_f3,    6, 0}, {0xED, d_ed,    2, 0}, {0xAE, d_ae,    1, 0}, {0xCD, d_cd,    1, 0},
    {0x70, d_70,    9, 0}, {0xE8, d_e8,    1, 0}, {0x62, d_62,   12, 0}, {0x63, d_63,   12, 0},
    {0x64, d_64,    7, 0}, {0x66, d_66,   10, 0}, {0x67, d_67,   10, 0}, {0x74, d_74,    7, 0},
    {0x98, d_98,    2, 0}, {0x35, NULL,    0, 0}, {0x11, NULL,    0, 120}, {0x29, NULL,    0, 20},
};

static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }

static esp_err_t spi_tx_bytes(const void *data, size_t len_bytes) {
    spi_transaction_t t = {0};
    t.length = len_bytes * 8;
    t.tx_buffer = data;
    return spi_device_polling_transmit(spi, &t);
}

static inline void cs_low(void)  { gpio_set_level(TFT_CS, 0); }
static inline void cs_high(void) { gpio_set_level(TFT_CS, 1); }
static inline void dc_low(void)  { gpio_set_level(TFT_DC, 0); }
static inline void dc_high(void) { gpio_set_level(TFT_DC, 1); }

static void lcd_begin_write(void) { cs_low(); }
static void lcd_end_write(void) { cs_high(); }

static esp_err_t lcd_write_cmd(uint8_t cmd) {
    dc_low();
    return spi_tx_bytes(&cmd, 1);
}

static esp_err_t lcd_write_data(const void *data, size_t len) {
    if (len == 0) return ESP_OK;
    dc_high();
    return spi_tx_bytes(data, len);
}

static esp_err_t lcd_write_cmd_data(uint8_t cmd, const void *data, size_t len) {
    ESP_ERROR_CHECK(lcd_write_cmd(cmd));
    if (len) ESP_ERROR_CHECK(lcd_write_data(data, len));
    return ESP_OK;
}

static void backlight_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BL_LEDC_MODE,
        .timer_num = BL_LEDC_TIMER,
        .duty_resolution = BL_LEDC_DUTY_RES,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num = TFT_BL,
        .speed_mode = BL_LEDC_MODE,
        .channel = BL_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BL_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, BL_LEDC_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
}

static void lcd_gpio_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TFT_CS) | (1ULL << TFT_DC) | (1ULL << TFT_RST) |
                        (1ULL << TFT_SCK) | (1ULL << TFT_MOSI),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    gpio_set_level(TFT_CS, 1);
    gpio_set_level(TFT_DC, 1);
    gpio_set_level(TFT_RST, 1);
    gpio_set_level(TFT_SCK, 0);
    gpio_set_level(TFT_MOSI, 0);
}

static void lcd_spi_init(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = TFT_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
#endif
        .max_transfer_sz = SCREEN_W * 2,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi));
}

static void lcd_hard_reset_like_arduino_gfx(void) {
    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void lcd_set_rotation0_and_ips_true(void) {
    uint8_t madctl = 0x08;
    lcd_begin_write();
    ESP_ERROR_CHECK(lcd_write_cmd_data(0x36, &madctl, 1));
    ESP_ERROR_CHECK(lcd_write_cmd(0x21));
    lcd_end_write();
}

static void lcd_init_match_arduino_gfx(void) {
    lcd_begin_write();
    for (size_t i = 0; i < sizeof(gc9a01_init) / sizeof(gc9a01_init[0]); ++i) {
        ESP_ERROR_CHECK(lcd_write_cmd_data(gc9a01_init[i].cmd, gc9a01_init[i].data,
                                           gc9a01_init[i].data_len));
        if (gc9a01_init[i].delay_ms) {
            lcd_end_write();
            vTaskDelay(pdMS_TO_TICKS(gc9a01_init[i].delay_ms));
            lcd_begin_write();
        }
    }
    lcd_end_write();
    lcd_set_rotation0_and_ips_true();
}

static void lcd_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint8_t caset[4] = {
        (uint8_t)(x >> 8), (uint8_t)(x & 0xFF),
        (uint8_t)((x + w - 1) >> 8), (uint8_t)((x + w - 1) & 0xFF)
    };
    uint8_t raset[4] = {
        (uint8_t)(y >> 8), (uint8_t)(y & 0xFF),
        (uint8_t)((y + h - 1) >> 8), (uint8_t)((y + h - 1) & 0xFF)
    };

    ESP_ERROR_CHECK(lcd_write_cmd_data(0x2A, caset, 4));
    ESP_ERROR_CHECK(lcd_write_cmd_data(0x2B, raset, 4));
    ESP_ERROR_CHECK(lcd_write_cmd(0x2C));
}

void screen_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    uint16_t c = swap16(color);
    for (int i = 0; i < w; ++i) line_buf[i] = c;

    lcd_begin_write();
    lcd_set_addr_window((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
    dc_high();
    for (int row = 0; row < h; ++row) {
        ESP_ERROR_CHECK(spi_tx_bytes(line_buf, (size_t)w * 2));
    }
    lcd_end_write();
}

void screen_fill_screen(uint16_t color) {
    screen_fill_rect(0, 0, SCREEN_W, SCREEN_H, color);
}

void screen_draw_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    uint16_t c = swap16(color);
    lcd_begin_write();
    lcd_set_addr_window((uint16_t)x, (uint16_t)y, 1, 1);
    dc_high();
    ESP_ERROR_CHECK(spi_tx_bytes(&c, 2));
    lcd_end_write();
}

void screen_fill_circle(int cx, int cy, int r, uint16_t color) {
    int rr = r * r;
    for (int y = -r; y <= r; ++y) {
        for (int x = -r; x <= r; ++x) {
            if (x * x + y * y <= rr) screen_draw_pixel(cx + x, cy + y, color);
        }
    }
}

void screen_draw_rgb565_block(int x, int y, int w, int h, const uint16_t *pixels_be) {
    if (w <= 0 || h <= 0) return;
    if (x < 0 || y < 0) return;
    if (x + w > SCREEN_W || y + h > SCREEN_H) return;

    lcd_begin_write();
    lcd_set_addr_window((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
    dc_high();
    ESP_ERROR_CHECK(spi_tx_bytes(pixels_be, (size_t)w * h * 2));
    lcd_end_write();
}

void screen_init(void) {
    ESP_LOGI(TAG, "init GPIO/backlight");
    lcd_gpio_init();
    backlight_init();

    ESP_LOGI(TAG, "init SPI");
    lcd_spi_init();

    ESP_LOGI(TAG, "hardware reset like Arduino_GFX");
    lcd_hard_reset_like_arduino_gfx();

    ESP_LOGI(TAG, "init panel with Arduino_GFX sequence");
    lcd_init_match_arduino_gfx();

    vTaskDelay(pdMS_TO_TICKS(120));
    screen_fill_screen(RGB565_BLACK);
    vTaskDelay(pdMS_TO_TICKS(20));
    screen_fill_screen(RGB565_BLACK);

    ESP_LOGI(TAG, "screen init done");
}

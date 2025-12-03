#include "st7735_lcd.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <stdarg.h>
#include <string.h>

extern const uint8_t font8x8[96][8];
static const char *TAG = "ST7735";

// Small DMA-able buffer
static uint16_t line_buf[64];

// ------------------------------------------------------
// Low-level helpers
// ------------------------------------------------------
static inline void st_cmd(St7735Lcd *lcd, uint8_t cmd) {
  gpio_set_level(lcd->dc_pin, 0);
  spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
  spi_device_transmit(lcd->spi, &t);
}

static inline void st_data(St7735Lcd *lcd, uint8_t data) {
  gpio_set_level(lcd->dc_pin, 1);
  spi_transaction_t t = {.length = 8, .tx_buffer = &data};
  spi_device_transmit(lcd->spi, &t);
}

static inline void st_data16(St7735Lcd *lcd, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data & 0xFF};
  gpio_set_level(lcd->dc_pin, 1);
  spi_transaction_t t = {.length = 16, .tx_buffer = buf};
  spi_device_transmit(lcd->spi, &t);
}

// ------------------------------------------------------
// Set draw window (with offsets applied)
// ------------------------------------------------------
static void st_set_addr(St7735Lcd *lcd, uint16_t x0, uint16_t y0, uint16_t x1,
                        uint16_t y1) {
  st_cmd(lcd, ST7735_CASET);
  st_data16(lcd, x0 + ST7735_XSTART);
  st_data16(lcd, x1 + ST7735_XSTART);

  st_cmd(lcd, ST7735_RASET);
  st_data16(lcd, y0 + ST7735_YSTART);
  st_data16(lcd, y1 + ST7735_YSTART);

  st_cmd(lcd, ST7735_RAMWR);
}

// ------------------------------------------------------
// Initialization sequence (verified working)
// ------------------------------------------------------
bool st7735_begin(St7735Lcd *lcd, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst,
                  gpio_num_t mosi, gpio_num_t sclk) {
  memset(lcd, 0, sizeof(*lcd));

  lcd->cs_pin = cs;
  lcd->dc_pin = dc;
  lcd->rst_pin = rst;
  lcd->mosi_pin = mosi;
  lcd->sck_pin = sclk;
  lcd->width = ST7735_WIDTH;
  lcd->height = ST7735_HEIGHT;

  ESP_LOGI(TAG, "Initializing ST7735 LCD");

  // Configure pins
  gpio_config_t io_conf = {.pin_bit_mask =
                               (1ULL << cs) | (1ULL << dc) | (1ULL << rst),
                           .mode = GPIO_MODE_OUTPUT};
  gpio_config(&io_conf);

  // SPI bus
  spi_bus_config_t buscfg = {.mosi_io_num = mosi,
                             .miso_io_num = -1,
                             .sclk_io_num = sclk,
                             .max_transfer_sz = sizeof(line_buf) * 2};
  spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t devcfg = {.clock_speed_hz = 26 * 1000 * 1000,
                                          .mode = 0,
                                          .spics_io_num = cs,
                                          .queue_size = 3};
  spi_bus_add_device(SPI3_HOST, &devcfg, &lcd->spi);

  // Hardware reset
  gpio_set_level(rst, 0);
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(rst, 1);
  vTaskDelay(pdMS_TO_TICKS(150));

  // ------------------------------------------------------
  // Full known-good init sequence
  // ------------------------------------------------------
  st_cmd(lcd, ST7735_SWRESET);
  vTaskDelay(pdMS_TO_TICKS(150));

  st_cmd(lcd, ST7735_SLPOUT);
  vTaskDelay(pdMS_TO_TICKS(150));

  st_cmd(lcd, ST7735_COLMOD);
  st_data(lcd, 0x05);

  st_cmd(lcd, ST7735_MADCTL);
  // st_data(lcd, 0x60); // 90 degree
  // st_data(lcd, 0x40); // fliped
  // st_data(lcd, 0x80); // 180 degree
  st_data(lcd, 0xC0);
  // st_data(lcd, 0x00);
  // st_data(lcd, 0xA0);

  st_cmd(lcd, ST7735_DISPON);
  vTaskDelay(pdMS_TO_TICKS(100));

  lcd->initialized = true;
  ESP_LOGI(TAG, "ST7735 init OK");
  return true;
}

// ------------------------------------------------------
// Clear screen
// ------------------------------------------------------
void st7735_clear(St7735Lcd *lcd, uint16_t color) {
  if (!lcd->initialized)
    return;

  uint16_t c = __builtin_bswap16(color);
  for (int i = 0; i < 64; i++)
    line_buf[i] = c;

  st_set_addr(lcd, 0, 0, lcd->width - 1, lcd->height - 1);

  int pixels = lcd->width * lcd->height;
  while (pixels > 0) {
    int chunk = (pixels > 64) ? 64 : pixels;
    spi_transaction_t t = {.length = chunk * 16, .tx_buffer = line_buf};
    gpio_set_level(lcd->dc_pin, 1);
    spi_device_transmit(lcd->spi, &t);
    pixels -= chunk;
  }
}

// ------------------------------------------------------
// Pixel
// ------------------------------------------------------
void st7735_set_pixel(St7735Lcd *lcd, uint16_t x, uint16_t y, uint16_t color) {
  if (!lcd->initialized)
    return;
  if (x >= lcd->width || y >= lcd->height)
    return;

  st_set_addr(lcd, x, y, x, y);
  st_data16(lcd, color);
}

// ------------------------------------------------------
// Rectangle
// ------------------------------------------------------
void st7735_draw_rect(St7735Lcd *lcd, int x, int y, int w, int h,
                      uint16_t color) {
  if (!lcd->initialized)
    return;

  st_set_addr(lcd, x, y, x + w - 1, y + h - 1);

  uint16_t c = __builtin_bswap16(color);
  for (int i = 0; i < 64; i++)
    line_buf[i] = c;

  int pixels = w * h;
  while (pixels > 0) {
    int chunk = (pixels > 64) ? 64 : pixels;
    spi_transaction_t t = {.length = chunk * 16, .tx_buffer = line_buf};
    gpio_set_level(lcd->dc_pin, 1);
    spi_device_transmit(lcd->spi, &t);
    pixels -= chunk;
  }
}

// ------------------------------------------------------
// Font (8x8) — minimal, same as before
// ------------------------------------------------------
extern const uint8_t font8x8[96][8];

// ------------------------------------------------------
// Draw character
// ------------------------------------------------------
void st7735_draw_char(St7735Lcd *lcd, int x, int y, char c, uint16_t fg,
                      uint16_t bg, uint8_t size) {
  if (!lcd->initialized)
    return;
  if (c < 32 || c > 127)
    c = ' ';

  const uint8_t *glyph = font8x8[c - 32];

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {

      // FIXED — bit0 = left pixel, bit7 = right pixel
      uint16_t color = (glyph[row] & (1 << col)) ? fg : bg;

      if (size == 1)
        st7735_set_pixel(lcd, x + col, y + row, color);
      else
        st7735_draw_rect(lcd, x + col * size, y + row * size, size, size,
                         color);
    }
  }
}

// ------------------------------------------------------
// Print string
// ------------------------------------------------------
void st7735_print(St7735Lcd *lcd, int x, int y, uint16_t fg, uint16_t bg,
                  uint8_t size, const char *text) {
  int cx = x;
  while (*text) {
    st7735_draw_char(lcd, cx, y, *text, fg, bg, size);
    cx += 8 * size;
    text++;
  }
}

// ------------------------------------------------------
// printf-like
// ------------------------------------------------------
void st7735_printf(St7735Lcd *lcd, int x, int y, uint16_t fg, uint16_t bg,
                   uint8_t size, const char *fmt, ...) {
  char buf[128];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  st7735_print(lcd, x, y, fg, bg, size, buf);
}

// ------------------------------------------------------
// Test pattern
// ------------------------------------------------------
void st7735_test_pattern(St7735Lcd *lcd) {
  st7735_clear(lcd, ST7735_BLACK);

  st7735_draw_rect(lcd, 0, 0, ST7735_WIDTH / 3, ST7735_HEIGHT / 2, ST7735_RED);
  st7735_draw_rect(lcd, ST7735_WIDTH / 3, 0, ST7735_WIDTH / 3,
                   ST7735_HEIGHT / 2, ST7735_GREEN);
  st7735_draw_rect(lcd, 2 * ST7735_WIDTH / 3, 0, ST7735_WIDTH / 3,
                   ST7735_HEIGHT / 2, ST7735_BLUE);

  st7735_draw_rect(lcd, 3, 3, ST7735_WIDTH - 6, ST7735_HEIGHT - 6,
                   ST7735_WHITE);

  st7735_printf(lcd, 10, ST7735_HEIGHT / 2 + 10, ST7735_WHITE, ST7735_BLACK, 1,
                "ST7735 OK");
}

// ------------------------------------------------------
uint16_t st7735_color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

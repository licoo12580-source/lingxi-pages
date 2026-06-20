/**
 * 灵犀 · 最简亮屏测试  v0622A-mini
 * 只亮LCD + 显示颜色，无LVGL、无触摸、无字库
 * 目的：确认ESP32-S3 + ILI9488硬件正常
 */
#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#define LCD_CS   45
#define LCD_DC   47
#define LCD_RST  48
#define LCD_MOSI 21
#define LCD_SCLK 20
#define LCD_BL   19

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;
public:
    LGFX(void) {
        { auto cfg = _bus.config();
          cfg.spi_host = SPI2_HOST; cfg.spi_mode = 0;
          cfg.freq_write = 40000000; cfg.freq_read = 16000000;
          cfg.spi_3wire = false; cfg.use_lock = true; cfg.dma_channel = 1;
          cfg.pin_sclk = LCD_SCLK; cfg.pin_mosi = LCD_MOSI;
          cfg.pin_miso = -1; cfg.pin_dc = LCD_DC;
          _bus.config(cfg); _panel.setBus(&_bus); }
        { auto cfg = _panel.config();
          cfg.pin_cs = LCD_CS; cfg.pin_rst = LCD_RST; cfg.pin_busy = -1;
          cfg.memory_width = 320; cfg.memory_height = 480;
          cfg.panel_width = 320; cfg.panel_height = 480;
          cfg.offset_x = 0; cfg.offset_y = 0; cfg.offset_rotation = 0;
          cfg.dummy_read_pixel = 8; cfg.dummy_read_bits = 1;
          cfg.readable = false; cfg.invert = true;
          cfg.rgb_order = true; cfg.dlen_16bit = false; cfg.bus_shared = false;
          _panel.config(cfg); }
        { auto cfg = _light.config();
          cfg.pin_bl = LCD_BL; cfg.invert = false;
          cfg.freq = 44100; cfg.pwm_channel = 7;
          _light.config(cfg); _panel.setLight(&_light); }
        setPanel(&_panel);
    }
};

LGFX tft;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

    Serial.begin(115200);
    delay(100);
    Serial.println("[Boot] Lumina v0622A-mini - 最简亮屏测试");

    tft.begin();
    tft.setBrightness(255);

    // 依次显示红绿蓝白黑，每2秒轮换
    Serial.println("[Boot] LCD初始化完成，开始颜色测试");
}

int color_idx = 0;
const lgfx::rgb565_t colors[] = {
    lgfx::rgb565_t(255, 0, 0),    // 红
    lgfx::rgb565_t(0, 255, 0),    // 绿
    lgfx::rgb565_t(0, 0, 255),    // 蓝
    lgfx::rgb565_t(255, 255, 255), // 白
    lgfx::rgb565_t(0, 0, 0),      // 黑
    lgfx::rgb565_t(11, 14, 23),   // 灵犀背景色
};
unsigned long last_change = 0;

void loop() {
    if (millis() - last_change > 2000) {
        last_change = millis();
        if (color_idx < 6) {
            tft.fillScreen(colors[color_idx]);
            Serial.printf("[Color] #%d: R=%d G=%d B=%d\n",
                color_idx, colors[color_idx].R8(), colors[color_idx].G8(), colors[color_idx].B8());
            color_idx++;
        }
    }
    delay(10);
}

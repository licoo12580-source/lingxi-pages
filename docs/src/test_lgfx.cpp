/**
 * 灵犀 · 空间感知 — 中文三页 + 触摸  v0621Z
 * LovyanGFX + LVGL 9.5 + ILI9488 + FT6336U(软件I2C bitbang) + 中文20px字库
 * 完整UI + 软件I2C驱动(无Wire依赖,内部上拉,50ms超时)
 */
#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include <lvgl.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <FT6336U.h>
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

static void tg1_feed() {
    TIMERG1.wdtwprotect.val = 0x50D83AA1;
    TIMERG1.wdtfeed.val = 1;
    TIMERG1.wdtwprotect.val = 0;
}

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
FT6336U touch;

extern lv_font_t font_cjk_20;

static void factory_init() {
    struct Cmd { uint8_t cmd; const uint8_t *data; uint8_t len; };
    static const uint8_t d_F7[] = {0xA9, 0x51, 0x2C, 0x82};
    static const uint8_t d_EC[] = {0x00, 0x02, 0x03, 0x7A};
    static const uint8_t d_C0[] = {0x13, 0x13};
    static const uint8_t d_C1[] = {0x41};
    static const uint8_t d_C5[] = {0x00, 0x28, 0x80};
    static const uint8_t d_B0[] = {0x00};
    static const uint8_t d_B1[] = {0xB0, 0x11};
    static const uint8_t d_B4[] = {0x02};
    static const uint8_t d_B6[] = {0x02, 0x22};
    static const uint8_t d_B7[] = {0xC6};
    static const uint8_t d_BE[] = {0x00, 0x04};
    static const uint8_t d_E9[] = {0x00};
    static const uint8_t d_F4[] = {0x00, 0x00, 0x0F};
    static const uint8_t d_E0[] = {0x00, 0x04, 0x0E, 0x08, 0x17, 0x0A, 0x40, 0x79, 0x4D, 0x07, 0x0E, 0x0A, 0x1A, 0x1D, 0x0F};
    static const uint8_t d_E1[] = {0x00, 0x1B, 0x1F, 0x02, 0x10, 0x05, 0x32, 0x34, 0x43, 0x02, 0x0A, 0x09, 0x33, 0x37, 0x0F};
    static const uint8_t d_36[] = {0x08};
    static const uint8_t d_3A[] = {0x55};

    static const Cmd seq[] = {
        {0xF7, d_F7, 4}, {0xEC, d_EC, 4}, {0xC0, d_C0, 2},
        {0xC1, d_C1, 1}, {0xC5, d_C5, 3}, {0xB0, d_B0, 1},
        {0xB1, d_B1, 2}, {0xB4, d_B4, 1}, {0xB6, d_B6, 2},
        {0xB7, d_B7, 1}, {0xBE, d_BE, 2}, {0xE9, d_E9, 1},
        {0xF4, d_F4, 3}, {0xE0, d_E0, 15}, {0xE1, d_E1, 15},
        {0xF4, d_F4, 3}, {0x36, d_36, 1}, {0x3A, d_3A, 1},
    };
    for (auto &c : seq) {
        tft.writeCommand(c.cmd);
        for (int i = 0; i < c.len; i++) tft.writeData(c.data[i]);
    }
    tft.writeCommand(0x11); delay(120);
    tft.writeCommand(0x29); delay(50);
    tft.startWrite();
    tft.writeCommand(0x36); tft.writeData(0x08);
    tft.writeCommand(0x21); tft.endWrite();
}

// ====== LVGL ======
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// 触摸读取函数
static void touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch.getPoint(x, y)) {
        data->point.x = x; data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

#define LV_BUF_SIZE (320 * 40)
static lv_color_t buf1[LV_BUF_SIZE];
static lv_color_t buf2[LV_BUF_SIZE];

#define C_BG     lv_color_make(11, 14, 23)
#define C_CARD   lv_color_make(19, 23, 37)
#define C_GREEN  lv_color_make(76, 175, 80)
#define C_BLUE   lv_color_make(33, 150, 243)
#define C_ORANGE lv_color_make(255, 152, 0)
#define C_GRAY   lv_color_make(107, 122, 153)
#define C_WHITE  lv_color_make(200, 208, 224)

static unsigned long last_switch = 0;
static int current_page = 0;
static lv_obj_t *pages[3];
static lv_obj_t *page_indicators[3];
static lv_obj_t *nav_btns[3];

static void switch_to_page(int page) {
    lv_obj_add_flag(pages[current_page], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(pages[page], LV_OBJ_FLAG_HIDDEN);

    const lv_color_t colors[3] = {C_GREEN, C_BLUE, C_ORANGE};
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(page_indicators[i], (i == page) ? colors[i] : C_CARD, 0);
        if (i == page) {
            lv_obj_set_style_bg_color(nav_btns[i], colors[i], 0);
            lv_obj_set_style_bg_opa(nav_btns[i], 255, 0);
            lv_obj_set_style_border_width(nav_btns[i], 0, 0);
        } else {
            lv_obj_set_style_bg_opa(nav_btns[i], 0, 0);
            lv_obj_set_style_border_color(nav_btns[i], C_GRAY, 0);
            lv_obj_set_style_border_width(nav_btns[i], 1, 0);
        }
        lv_obj_t *lbl = (lv_obj_t *)lv_obj_get_child(nav_btns[i], 0);
        if (lbl) lv_obj_set_style_text_color(lbl, (i == page) ? lv_color_white() : C_WHITE, 0);
    }
    current_page = page;
}

static void on_nav_click(lv_event_t *e) {
    int target = (int)(intptr_t)lv_event_get_user_data(e);
    if (target == current_page) return;
    switch_to_page(target);
}

static lv_obj_t *make_card(lv_obj_t *parent, int x, int y, int w, int h,
                           const char *label, const char *value,
                           lv_color_t val_color, const lv_font_t *val_font) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, C_CARD, 0);
    lv_obj_set_style_bg_opa(card, 255, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);

    lv_obj_t *lb = lv_label_create(card);
    lv_label_set_text(lb, label);
    lv_obj_set_style_text_color(lb, C_GRAY, 0);
    lv_obj_set_style_text_font(lb, &font_cjk_20, 0);
    lv_obj_set_pos(lb, 10, 6);

    lv_obj_t *vl = lv_label_create(card);
    lv_label_set_text(vl, value);
    lv_obj_set_style_text_color(vl, val_color, 0);
    lv_obj_set_style_text_font(vl, val_font, 0);
    lv_obj_set_pos(vl, 10, h - 28);
    return card;
}

// ====== 页面1: 节能 ======
static void create_page_energy(lv_obj_t *parent) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_remove_style_all(page);
    lv_obj_set_style_bg_color(page, C_BG, 0);
    lv_obj_set_style_bg_opa(page, 255, 0);
    lv_obj_set_size(page, 320, 480);

    lv_obj_t *sb = lv_label_create(page);
    lv_label_set_text(sb, "灵犀");
    lv_obj_set_style_text_color(sb, C_GRAY, 0);
    lv_obj_set_style_text_font(sb, &font_cjk_20, 0);
    lv_obj_set_pos(sb, 12, 8);

    lv_obj_t *dot = lv_label_create(page);
    lv_label_set_text(dot, LV_SYMBOL_WIFI " 在线");
    lv_obj_set_style_text_color(dot, C_GREEN, 0);
    lv_obj_set_style_text_font(dot, &font_cjk_20, 0);
    lv_obj_set_pos(dot, 240, 8);

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "节能");
    lv_obj_set_style_text_color(title, C_GREEN, 0);
    lv_obj_set_style_text_font(title, &font_cjk_20, 0);
    lv_obj_set_pos(title, 12, 36);

    make_card(page, 7, 72, 306, 58, "今日省电", "3.2kWh", C_GREEN, &lv_font_montserrat_30);
    make_card(page, 7, 138, 148, 70, "月省电", "96.8kWh", C_BLUE, &lv_font_montserrat_20);
    make_card(page, 165, 138, 148, 70, "设备在线", "3/5台", C_GREEN, &lv_font_montserrat_20);
    make_card(page, 7, 216, 148, 70, "节能数", "2间", C_ORANGE, &lv_font_montserrat_20);
    make_card(page, 165, 216, 148, 70, "室温", "26°C", C_ORANGE, &lv_font_montserrat_20);

    pages[0] = page;
}

// ====== 页面2: 感知 ======
static void create_page_sense(lv_obj_t *parent) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_remove_style_all(page);
    lv_obj_set_style_bg_color(page, C_BG, 0);
    lv_obj_set_style_bg_opa(page, 255, 0);
    lv_obj_set_size(page, 320, 480);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *sb = lv_label_create(page);
    lv_label_set_text(sb, "灵犀");
    lv_obj_set_style_text_color(sb, C_GRAY, 0);
    lv_obj_set_style_text_font(sb, &font_cjk_20, 0);
    lv_obj_set_pos(sb, 12, 8);

    lv_obj_t *dot = lv_label_create(page);
    lv_label_set_text(dot, LV_SYMBOL_WIFI " 在线");
    lv_obj_set_style_text_color(dot, C_GREEN, 0);
    lv_obj_set_style_text_font(dot, &font_cjk_20, 0);
    lv_obj_set_pos(dot, 240, 8);

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "空间感知");
    lv_obj_set_style_text_color(title, C_BLUE, 0);
    lv_obj_set_style_text_font(title, &font_cjk_20, 0);
    lv_obj_set_pos(title, 12, 36);

    make_card(page, 7, 72, 148, 100, "床", "有人", C_GREEN, &font_cjk_20);
    make_card(page, 165, 72, 148, 100, "门", "无人", C_GRAY, &font_cjk_20);
    make_card(page, 7, 180, 148, 100, "窗", "无人", C_GRAY, &font_cjk_20);
    make_card(page, 165, 180, 148, 100, "浴室", "有人", C_GREEN, &font_cjk_20);

    lv_obj_t *status = lv_label_create(page);
    lv_label_set_text(status, "房间 2人  实时监测");
    lv_obj_set_style_text_color(status, C_WHITE, 0);
    lv_obj_set_style_text_font(status, &font_cjk_20, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 296);

    pages[1] = page;
}

// ====== 页面3: 系统 ======
static void create_page_settings(lv_obj_t *parent) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_remove_style_all(page);
    lv_obj_set_style_bg_color(page, C_BG, 0);
    lv_obj_set_style_bg_opa(page, 255, 0);
    lv_obj_set_size(page, 320, 480);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *sb = lv_label_create(page);
    lv_label_set_text(sb, "灵犀");
    lv_obj_set_style_text_color(sb, C_GRAY, 0);
    lv_obj_set_style_text_font(sb, &font_cjk_20, 0);
    lv_obj_set_pos(sb, 12, 8);

    lv_obj_t *dot = lv_label_create(page);
    lv_label_set_text(dot, LV_SYMBOL_WIFI " 在线");
    lv_obj_set_style_text_color(dot, C_GREEN, 0);
    lv_obj_set_style_text_font(dot, &font_cjk_20, 0);
    lv_obj_set_pos(dot, 240, 8);

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "系统");
    lv_obj_set_style_text_color(title, C_ORANGE, 0);
    lv_obj_set_style_text_font(title, &font_cjk_20, 0);
    lv_obj_set_pos(title, 12, 36);

    make_card(page, 7, 72, 306, 48, "版本号", "v0621Z", C_WHITE, &lv_font_montserrat_16);
    make_card(page, 7, 128, 306, 48, "设备", "ESP32-S3", C_WHITE, &lv_font_montserrat_16);
    make_card(page, 7, 184, 306, 48, "屏幕", "ILI9488 320x480", C_WHITE, &lv_font_montserrat_16);
    // 触摸状态动态更新（在setup中根据touch_ok设置）
    make_card(page, 7, 240, 306, 48, "触摸", "检测中", C_GRAY, &font_cjk_20);

    pages[2] = page;
}

// ====== 底栏 ======
static void create_bottom_bar(lv_obj_t *parent) {
    const char *names[3] = {"节能", "感知", "系统"};
    const lv_color_t colors[3] = {C_GREEN, C_BLUE, C_ORANGE};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);
        lv_obj_set_pos(btn, 8 + i * 104, 306);
        lv_obj_set_size(btn, 96, 32);
        lv_obj_set_style_radius(btn, 6, 0);
        if (i == 0) {
            lv_obj_set_style_bg_color(btn, colors[0], 0);
            lv_obj_set_style_bg_opa(btn, 255, 0);
        } else {
            lv_obj_set_style_bg_opa(btn, 0, 0);
            lv_obj_set_style_border_color(btn, C_GRAY, 0);
            lv_obj_set_style_border_width(btn, 1, 0);
        }
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_color(lbl, (i == 0) ? lv_color_white() : C_WHITE, 0);
        lv_obj_set_style_text_font(lbl, &font_cjk_20, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, on_nav_click, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        nav_btns[i] = btn;
    }

    lv_obj_t *dot_bar = lv_obj_create(parent);
    lv_obj_remove_style_all(dot_bar);
    lv_obj_set_pos(dot_bar, 120, 354);
    lv_obj_set_size(dot_bar, 80, 12);
    lv_obj_set_style_bg_opa(dot_bar, 0, 0);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *dot = lv_obj_create(dot_bar);
        lv_obj_remove_style_all(dot);
        lv_obj_set_pos(dot, i * 28, 0);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, (i == 0) ? colors[0] : C_CARD, 0);
        lv_obj_set_style_bg_opa(dot, 255, 0);
        page_indicators[i] = dot;
    }

    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 7, 376);
    lv_obj_set_size(line, 306, 2);
    lv_obj_set_style_bg_color(line, C_CARD, 0);
    lv_obj_set_style_bg_opa(line, 255, 0);
    lv_obj_set_style_radius(line, 1, 0);

    lv_obj_t *footer = lv_label_create(parent);
    lv_label_set_text(footer, "v0621Z  灵犀  空间感知");
    lv_obj_set_style_text_color(footer, C_GRAY, 0);
    lv_obj_set_style_text_font(footer, &font_cjk_20, 0);
    lv_obj_set_pos(footer, 12, 386);
}

bool g_touch_ok = false;

void setup() {
    // 优先杀掉TG1WDT（在Arduino框架初始化前可能已被配置为短超时）
    TIMERG1.wdtwprotect.val = 0x50D83AA1;
    TIMERG1.wdt_config0.val = 0;  // 完全禁用
    TIMERG1.wdtwprotect.val = 0;

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();
    tg1_feed();

    Serial.begin(115200);
    Serial.println("[Boot] Lingxi v0621Z - 中文三页 + 触摸(软件I2C bitbang)");
    tg1_feed();

    // LCD
    tft.begin();
    factory_init();
    tft.setBrightness(255);
    tg1_feed();

    // 触摸初始化 (厂家时序 + 完整STOP读写协议)
    g_touch_ok = touch.begin(FT6336U_SDA, FT6336U_SCL);
    tg1_feed();

    // LVGL
    lv_init();
    lv_display_t *disp = lv_display_create(320, 480);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(lv_color_t) * LV_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, disp_flush);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, g_touch_ok ? touch_read : NULL);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, 255, 0);

    create_page_energy(scr);
    create_page_sense(scr);
    create_page_settings(scr);
    create_bottom_bar(scr);
    tg1_feed();

    Serial.printf("[Boot] 就绪 - Touch: %s\n", g_touch_ok ? "OK" : "FAIL");
}

void loop() {
    tg1_feed();
    lv_timer_handler();

    // 自动翻页(每5秒) - 如果触摸未启用
    if (!g_touch_ok && millis() - last_switch > 5000) {
        last_switch = millis();
        int next = (current_page + 1) % 3;
        switch_to_page(next);
    }

    delay(5);
}

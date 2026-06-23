/**
 * 灵犀 · 屏端 MQTT 接收 + 三页显示  v0624A
 * =============================================
 * 依赖：LovyanGFX + LVGL 9.5 + ArduinoJson + WiFi + PubSubClient
 * 硬件：ESP32-S3 + ILI9488(320x480) + FT6336U触摸
 * 
 * MQTT Topics（订阅 lingxi/v1/{device_id}/#）
 *   sense  → 人感+温湿度+光照（秒级）
 *   energy → 电量+功率+节能（分钟级）
 *   system → 设备状态+告警（30s）
 * 
 * 三页UI：
 *   页0: 空间感知（zones + 人数 + 温湿度）
 *   页1: 节能数据（功率 + 今日/本月省电）
 *   页2: 系统状态（在线 + 版本 + 告警）
 *   触摸左右滑动切换
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LVGL.h>
#include <LovyanGFX.hpp>

// ====== 硬件配置 ======
#define LCD_CS   45
#define LCD_DC   47
#define LCD_RST  48
#define LCD_MOSI 21
#define LCD_SCLK 20
#define LCD_BL   19

// ====== 编译时默认值（SPIFFS /config.json 可覆盖）=====
#ifndef WIFI_SSID
#define WIFI_SSID "lingxi-iot"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "lingxi2026"
#endif
#ifndef MQTT_BROKER
#define MQTT_BROKER "mqtt.lingxi.local"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef DEVICE_ID
#define DEVICE_ID "lingxi-screen-001"
#endif

// ====== LovyanGFX 初始化 ======
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
          _light.config(cfg); }
        setPanel(&_panel); setLight(&_light);
    }
};
LGFX lcd;

// ====== LVGL 显示驱动 ======
static lv_display_t *disp;
static lv_indev_t *indev_touch;

static void lvgl_flush(lv_display_t *d, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)px_map, w * h);
    lcd.endWrite();
    lv_display_flush_ready(d);
}

static void lvgl_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    // TODO: 集成 FT6336U 触摸驱动
    // 临时：返回无触摸
    data->state = LV_INDEV_STATE_RELEASED;
}

// ====== MQTT ======
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// MQTT topic 前缀
const String TOPIC_PREFIX = "lingxi/v1/";
String deviceTopic;  // lingxi/v1/{device_id}/#

// ====== 数据状态（对齐 lingxi-mqtt-v3.h display 段） ======
struct EnergyData {
    float today_saved = 0;      // 今日省电 kWh
    float month_saved = 0;      // 本月省电 kWh
    int   saved_rooms = 0;      // 节能房间数
    float today_usage = 0;      // 今日用电 kWh
    float month_usage = 0;      // 本月用电 kWh
    float temp_set = 0;         // 空调设定温度 °C
} energyData;

struct ZoneData {
    String status;      // "有人" / "无人" / "检测中"
    String color;       // "green" / "gray" / "orange"
};

struct SenseData {
    ZoneData zones[4];  // bed, door, window, bath
    int total_people = 0;
    String status_text;
    bool has_alarm = false;
    String alarm_text;
} senseData;

struct SystemData {
    String version;
    String device_type;
    String screen_info;
    bool online = false;
    bool touch_ok = false;
    int wifi_rssi = 0;
    int uptime_days = 0;
} systemData;

// ====== UI 对象 ======
#define NUM_PAGES 3
static int currentPage = 0;
static lv_obj_t *pages[NUM_PAGES] = {nullptr};

// 页0: 空间感知
static lv_obj_t *label_people, *label_temp, *label_humidity, *label_light;
static lv_obj_t *zone_indicators[4];

// 页1: 节能数据
static lv_obj_t *label_power, *label_today, *label_month, *label_saving;

// 页2: 系统状态
static lv_obj_t *label_version, *label_online, *label_alarm;

// ====== WebSocket/屏幕更新 ======
static unsigned long lastSenseUpdate = 0;
static unsigned long lastEnergyUpdate = 0;
static unsigned long lastSystemUpdate = 0;

// ====== Zones 名称映射 ======
const char *ZONE_NAMES[4] = {"bed", "door", "window", "bath"};
const char *ZONE_LABELS[4] = {"🛏 床", "🚪 门", "🪟 窗", "🚿 浴室"};

// ====== MQTT 回调（单topic: lingxi/v1/{id}/display） ======
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = String((char *)payload).substring(0, length);

    // 只处理 display topic
    if (!topicStr.endsWith("/display")) return;

    // 解析 JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payloadStr);
    if (err) { Serial.println("[MQTT] JSON parse error"); return; }

    // === energy 段 ===
    JsonObject energy = doc["energy"];
    if (!energy.isNull()) {
        energyData.today_saved = energy["today_saved"] | 0.0f;
        energyData.month_saved = energy["month_saved"] | 0.0f;
        energyData.saved_rooms = energy["saved_rooms"] | 0;
        energyData.today_usage = energy["today_usage"] | 0.0f;
        energyData.month_usage = energy["month_usage"] | 0.0f;
        energyData.temp_set = energy["temp_set"] | 0.0f;
        updateEnergyUI();
    }

    // === sense 段 ===
    JsonObject sense = doc["sense"];
    if (!sense.isNull()) {
        JsonArray zones = sense["zones"];
        if (!zones.isNull()) {
            for (int i = 0; i < 4 && i < zones.size(); i++) {
                senseData.zones[i].status = zones[i]["s"] | "检测中";
                senseData.zones[i].color = zones[i]["c"] | "gray";
            }
        }
        senseData.total_people = sense["total_people"] | 0;
        senseData.status_text = sense["status_text"] | "";
        senseData.has_alarm = sense["has_alarm"] | false;
        senseData.alarm_text = sense["alarm_text"] | "";
        updateSenseUI();
    }

    // === system 段 ===
    JsonObject sys = doc["system"];
    if (!sys.isNull()) {
        systemData.version = sys["version"] | "";
        systemData.device_type = sys["device"] | "";
        systemData.screen_info = sys["screen"] | "";
        systemData.online = sys["online"] | false;
        systemData.touch_ok = sys["touch_ok"] | false;
        systemData.wifi_rssi = sys["wifi_rssi"] | 0;
        systemData.uptime_days = sys["uptime_days"] | 0;
        updateSystemUI();
    }
}

// ====== MQTT 连接 ======
void mqttReconnect() {
    while (!mqttClient.connected()) {
        Serial.print("[MQTT] 连接中...");
        String clientId = "lingxi-screen-" + String(DEVICE_ID);
        if (mqttClient.connect(clientId.c_str())) {
            Serial.println(" 已连接");
            // 订阅 display topic
            deviceTopic = TOPIC_PREFIX + String(DEVICE_ID) + "/display";
            mqttClient.subscribe(deviceTopic.c_str());
            Serial.println("[MQTT] 订阅: " + deviceTopic);
        } else {
            Serial.print(" 失败, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" 5秒后重试");
            delay(5000);
        }
    }
}

// ====== UI 更新函数 ======

void updateSenseUI() {
    if (!pages[0]) return;
    char buf[64];

    // 人数
    snprintf(buf, sizeof(buf), "%d", senseData.total_people);
    lv_label_set_text(label_people, buf);

    // 状态文字
    if (senseData.has_alarm) {
        lv_label_set_text(label_temp, ("⚠ " + senseData.alarm_text).c_str());
        lv_obj_set_style_text_color(label_temp, lv_color_hex(0xef4444), 0);
    } else {
        lv_label_set_text(label_temp, senseData.status_text.c_str());
        lv_obj_set_style_text_color(label_temp, lv_color_hex(0x4ade80), 0);
    }

    // Zones 状态（颜色映射）
    for (int i = 0; i < 4; i++) {
        if (zone_indicators[i]) {
            uint32_t color;
            if (senseData.zones[i].color == "green")      color = 0x22c55e;
            else if (senseData.zones[i].color == "orange") color = 0xf59e0b;
            else                                           color = 0x334155;
            lv_obj_set_style_bg_color(zone_indicators[i], lv_color_hex(color), 0);

            // 更新 zone 内文字
            lv_obj_t *child = lv_obj_get_child(zone_indicators[i], 0);
            if (child) {
                snprintf(buf, sizeof(buf), "%s\n%s", ZONE_LABELS[i], senseData.zones[i].status.c_str());
                lv_label_set_text(child, buf);
            }
        }
    }
}

void updateEnergyUI() {
    if (!pages[1]) return;
    char buf[64];

    snprintf(buf, sizeof(buf), "今日省 %.1f", energyData.today_saved);
    lv_label_set_text(label_power, buf);

    snprintf(buf, sizeof(buf), "月省 %.1f kWh", energyData.month_saved);
    lv_label_set_text(label_today, buf);

    snprintf(buf, sizeof(buf), "用电 %.1f/%.0f", energyData.today_usage, energyData.month_usage);
    lv_label_set_text(label_month, buf);

    snprintf(buf, sizeof(buf), "节能 %d间  设备在线", energyData.saved_rooms);
    lv_label_set_text(label_saving, buf);
}

void updateSystemUI() {
    if (!pages[2]) return;
    char buf[64];

    snprintf(buf, sizeof(buf), "%s / %s", systemData.version.c_str(), systemData.device_type.c_str());
    lv_label_set_text(label_version, buf);

    snprintf(buf, sizeof(buf), "状态: %s", systemData.online ? "✅ 在线" : "❌ 离线");
    lv_label_set_text(label_online, buf);

    snprintf(buf, sizeof(buf), "触摸: %s", systemData.touch_ok ? "✅ 正常" : "❌ 异常");
    lv_label_set_text(label_alarm, buf);
    lv_obj_set_style_text_color(label_alarm, systemData.touch_ok ? lv_color_hex(0x22c55e) : lv_color_hex(0xef4444), 0);
}

// ====== UI 创建 ======

void createPageSense() {
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, 320, 480);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0b1120), 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    // 标题
    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "🦞 空间感知");
    lv_obj_set_style_text_color(title, lv_color_hex(0x60a5fa), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    // 人数（大字号）
    label_people = lv_label_create(page);
    lv_obj_set_style_text_color(label_people, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_text_font(label_people, &lv_font_montserrat_48, 0);
    lv_obj_align(label_people, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *people_label = lv_label_create(page);
    lv_label_set_text(people_label, "房间人数");
    lv_obj_set_style_text_color(people_label, lv_color_hex(0x64748b), 0);
    lv_obj_align(people_label, LV_ALIGN_TOP_MID, 0, 100);

    // 温湿度 + 光照（一行三个）
    int y1 = 140;
    int cx[] = {60, 160, 260};
    const char *labels[] = {"温度", "湿度", "光照"};

    label_temp = lv_label_create(page);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0x4ade80), 0);
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, cx[0]-30, y1);

    label_humidity = lv_label_create(page);
    lv_obj_set_style_text_color(label_humidity, lv_color_hex(0x60a5fa), 0);
    lv_obj_align(label_humidity, LV_ALIGN_TOP_LEFT, cx[1]-30, y1);

    label_light = lv_label_create(page);
    lv_obj_set_style_text_color(label_light, lv_color_hex(0xfbbf24), 0);
    lv_obj_align(label_light, LV_ALIGN_TOP_LEFT, cx[2]-30, y1);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *l = lv_label_create(page);
        lv_label_set_text(l, labels[i]);
        lv_obj_set_style_text_color(l, lv_color_hex(0x64748b), 0);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, cx[i]-20, y1+28);
    }

    // Zones 指示器（2x2 网格）
    int zy = 210;
    int zcx[] = {86, 236};
    int zcy[] = {zy, zy + 100};
    for (int i = 0; i < 4; i++) {
        int col = i % 2, row = i / 2;
        zone_indicators[i] = lv_obj_create(page);
        lv_obj_set_size(zone_indicators[i], 130, 70);
        lv_obj_set_style_bg_color(zone_indicators[i], lv_color_hex(0x334155), 0);
        lv_obj_set_style_radius(zone_indicators[i], 8, 0);
        lv_obj_align(zone_indicators[i], LV_ALIGN_TOP_LEFT, zcx[col] - 65, zcy[row]);

        lv_obj_t *zl = lv_label_create(zone_indicators[i]);
        lv_label_set_text(zl, ZONE_LABELS[i]);
        lv_obj_set_style_text_color(zl, lv_color_hex(0xe2e8f0), 0);
        lv_obj_center(zl);
    }

    pages[0] = page;
}

void createPageEnergy() {
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, 320, 480);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0b1120), 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    // 标题
    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "⚡ 节能数据");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4ade80), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    // 当前功率（大字号）
    label_power = lv_label_create(page);
    lv_obj_set_style_text_color(label_power, lv_color_hex(0x4ade80), 0);
    lv_obj_set_style_text_font(label_power, &lv_font_montserrat_48, 0);
    lv_obj_align(label_power, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *pw = lv_label_create(page);
    lv_label_set_text(pw, "当前功率");
    lv_obj_set_style_text_color(pw, lv_color_hex(0x64748b), 0);
    lv_obj_align(pw, LV_ALIGN_TOP_MID, 0, 110);

    // 今日 + 本月（两列）
    int ey = 150;
    int ecx[] = {86, 236};

    label_today = lv_label_create(page);
    lv_obj_set_style_text_color(label_today, lv_color_hex(0x60a5fa), 0);
    lv_obj_align(label_today, LV_ALIGN_TOP_LEFT, ecx[0]-50, ey);

    label_month = lv_label_create(page);
    lv_obj_set_style_text_color(label_month, lv_color_hex(0xa78bfa), 0);
    lv_obj_align(label_month, LV_ALIGN_TOP_LEFT, ecx[1]-50, ey);

    lv_obj_t *lt = lv_label_create(page);
    lv_label_set_text(lt, "今日");
    lv_obj_set_style_text_color(lt, lv_color_hex(0x64748b), 0);
    lv_obj_align(lt, LV_ALIGN_TOP_LEFT, ecx[0]-15, ey+30);

    lv_obj_t *lm = lv_label_create(page);
    lv_label_set_text(lm, "本月");
    lv_obj_set_style_text_color(lm, lv_color_hex(0x64748b), 0);
    lv_obj_align(lm, LV_ALIGN_TOP_LEFT, ecx[1]-15, ey+30);

    // 节能房间数
    label_saving = lv_label_create(page);
    lv_obj_set_style_text_color(label_saving, lv_color_hex(0xfbbf24), 0);
    lv_obj_align(label_saving, LV_ALIGN_TOP_MID, 0, 250);

    lv_obj_t *sr = lv_label_create(page);
    lv_label_set_text(sr, "设备在线 / 总数");
    lv_obj_set_style_text_color(sr, lv_color_hex(0x64748b), 0);
    lv_obj_align(sr, LV_ALIGN_TOP_MID, 0, 280);

    pages[1] = page;
}

void createPageSystem() {
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, 320, 480);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0b1120), 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    // 标题
    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "⚙️ 系统状态");
    lv_obj_set_style_text_color(title, lv_color_hex(0xa78bfa), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    int sy = 60;
    int line_h = 50;

    label_version = lv_label_create(page);
    lv_obj_set_style_text_color(label_version, lv_color_hex(0xe2e8f0), 0);
    lv_obj_align(label_version, LV_ALIGN_TOP_LEFT, 20, sy);

    lv_obj_t *sv = lv_label_create(page);
    lv_label_set_text(sv, "固件版本");
    lv_obj_set_style_text_color(sv, lv_color_hex(0x64748b), 0);
    lv_obj_align(sv, LV_ALIGN_TOP_LEFT, 20, sy + 24);

    label_online = lv_label_create(page);
    lv_obj_set_style_text_color(label_online, lv_color_hex(0x4ade80), 0);
    lv_obj_align(label_online, LV_ALIGN_TOP_LEFT, 20, sy + line_h);

    lv_obj_t *so = lv_label_create(page);
    lv_label_set_text(so, "连接状态");
    lv_obj_set_style_text_color(so, lv_color_hex(0x64748b), 0);
    lv_obj_align(so, LV_ALIGN_TOP_LEFT, 20, sy + line_h + 24);

    label_alarm = lv_label_create(page);
    lv_obj_set_style_text_color(label_alarm, lv_color_hex(0x22c55e), 0);
    lv_obj_align(label_alarm, LV_ALIGN_TOP_LEFT, 20, sy + line_h * 2);

    lv_obj_t *sa = lv_label_create(page);
    lv_label_set_text(sa, "告警状态");
    lv_obj_set_style_text_color(sa, lv_color_hex(0x64748b), 0);
    lv_obj_align(sa, LV_ALIGN_TOP_LEFT, 20, sy + line_h * 2 + 24);

    // 屏幕信息
    lv_obj_t *screen_info = lv_label_create(page);
    lv_label_set_text(screen_info, "ILI9488 320x480 · FT6336U触摸");
    lv_obj_set_style_text_color(screen_info, lv_color_hex(0x475569), 0);
    lv_obj_align(screen_info, LV_ALIGN_BOTTOM_MID, 0, -16);

    pages[2] = page;
}

void switchPage(int pageIndex) {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (pages[i]) {
            lv_obj_set_hidden(pages[i], i != pageIndex);
        }
    }
    currentPage = pageIndex;
}

// ====== 初始化 ======
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n🦞 灵犀屏端 v0624A");

    // 初始化 LCD
    lcd.init();
    lcd.setBrightness(128);
    lcd.fillScreen(TFT_BLACK);

    // 初始化 LVGL
    lv_init();
    disp = lv_display_create(320, 480);
    lv_display_set_flush_cb(disp, lvgl_flush);
    static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(320 * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    static lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(320 * 60 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_display_set_buffers(disp, buf1, buf2, 320 * 60 * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 触摸（TODO: 集成FT6336U）
    indev_touch = lv_indev_create();
    lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touch, lvgl_touch_read);

    // 创建三页
    createPageSense();
    createPageEnergy();
    createPageSystem();

    // 默认显示页0
    switchPage(0);

    // WiFi 连接
    Serial.print("[WiFi] 连接 ");
    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" 已连接");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // MQTT 初始化
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

// ====== 主循环 ======
void loop() {
    // MQTT 保活
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();

    // LVGL 刷新
    lv_timer_handler();
    delay(5);

    // 触摸翻页检测（TODO: 改为真实触摸事件）
    // 临时：每10秒自动翻页
    static unsigned long lastSwitch = 0;
    if (millis() - lastSwitch > 10000) {
        lastSwitch = millis();
        switchPage((currentPage + 1) % NUM_PAGES);
    }
}

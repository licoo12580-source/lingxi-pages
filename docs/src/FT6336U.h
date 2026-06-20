// FT6336U.h — 电容触摸屏驱动 (FocalTech FT6336U)
// 适用: ESP32-S3 + ILI9488 320x480
// 引脚: SDA=GPIO35, SCL=GPIO36, RST=GPIO37, IRQ=GPIO40

#ifndef FT6336U_H
#define FT6336U_H

#include <Wire.h>
#include <Arduino.h>

// === 引脚 ===
#define FT6336U_SDA  35
#define FT6336U_SCL  36
#define FT6336U_RST  37
#define FT6336U_INT  40

#define FT6336U_ADDR  0x38

// === 寄存器 ===
#define REG_DEVICE_MODE  0x00
#define REG_GESTURE_ID   0x01
#define REG_TD_STATUS    0x02
#define REG_P1_XH        0x03
#define REG_P1_XL        0x04
#define REG_P1_YH        0x05
#define REG_P1_YL        0x06
#define REG_P2_XH        0x09
#define REG_P2_XL        0x0A
#define REG_P2_YH        0x0B
#define REG_P2_YL        0x0C
#define REG_CHIP_ID      0xA3
#define REG_G_MODE       0xA4
#define REG_VENDOR_ID    0xA8

// === 触摸事件 ===
enum TouchEvt : uint8_t {
  EVT_PRESS  = 0,
  EVT_RELEASE = 1,
  EVT_CONTACT = 2,
  EVT_NONE   = 3
};

// === 触摸点 ===
struct TPoint {
  TouchEvt event;
  uint16_t x, y;
  uint8_t weight, area;
};

// === 扫描结果 ===
struct TScan {
  uint8_t count;
  TPoint pts[2];
  uint8_t gesture;
};

// === 驱动类 ===
class FT6336U {
public:
  FT6336U();

  // 初始化，可指定I2C引脚（不传则用默认35/36）
  bool begin(int sda = -1, int scl = -1);

  // 硬件复位
  void reset();

  // 扫描触摸
  TScan scan();

  // 获取触摸点数（快速）
  uint8_t touchCount();

  // 获取第一个触摸点的坐标（快速）
  bool getPoint(uint16_t &x, uint16_t &y);

  // 检测是否有触摸按下
  bool isTouched();

  // 等待触摸释放 (用于检测"点击"动作)
  bool waitRelease(uint32_t timeout_ms = 500);

private:
  uint8_t readReg(uint8_t reg);
  void writeReg(uint8_t reg, uint8_t val);
  uint16_t readCoord12(uint8_t xh_reg, uint8_t xl_reg);
  TouchEvt readEvt(uint8_t xh_reg);
};

#endif

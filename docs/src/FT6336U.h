// FT6336U.h — 电容触摸屏驱动 (FocalTech FT6336U)
// 适用: ESP32-S3 + CL35BC1070-40A (ILI9488 + FT6336U)
// 引脚: SDA=GPIO35, SCL=GPIO36, RST=GPIO37, IRQ=GPIO40
// 参考: FocalTech FT6X36 Datasheet + 鸿讯电子Demo

#ifndef FT6336U_H
#define FT6336U_H

#include <Wire.h>
#include <Arduino.h>

// === 引脚 ===
#define FT6336U_SDA  35
#define FT6336U_SCL  36
#define FT6336U_RST  37
#define FT6336U_INT  40

// I2C从机地址 (7-bit)
#define FT6336U_ADDR  0x38

// === 寄存器 (Operating Mode) ===
#define REG_DEVICE_MODE    0x00
#define REG_GESTURE_ID     0x01
#define REG_TD_STATUS      0x02      // Touch Data Status [3:0]=点数
#define REG_P1_XH          0x03      // 第1点 X高字节 [7:6]=Event [3:0]=X[11:8]
#define REG_P1_XL          0x04      // 第1点 X低字节 X[7:0]
#define REG_P1_YH          0x05      // 第1点 Y高字节 [7:4]=ID [3:0]=Y[11:8]
#define REG_P1_YL          0x06      // 第1点 Y低字节 Y[7:0]
#define REG_P1_WEIGHT      0x07
#define REG_P1_MISC        0x08
#define REG_P2_XH          0x09
#define REG_P2_XL          0x0A
#define REG_P2_YH          0x0B
#define REG_P2_YL          0x0C
#define REG_P2_WEIGHT      0x0D
#define REG_P2_MISC        0x0E

// === 系统信息寄存器 ===
#define REG_CHIP_ID        0xA3      // ID_G_CIPHER, 厂商默认0x55
#define REG_G_MODE         0xA4      // ID_G_MODE: 0=中断使能 1=中断禁用
#define REG_VENDOR_ID      0xA8      // ID_G_FT5201ID, FT5201返回0x79

// === 触摸事件类型 ===
enum TouchEvt : uint8_t {
  EVT_PRESS   = 0,    // Put Down
  EVT_RELEASE = 1,    // Put Up
  EVT_CONTACT = 2,    // Contact
  EVT_NONE    = 3     // Reserved
};

// === 触摸点 ===
struct TPoint {
  TouchEvt event;
  uint16_t x, y;
  uint8_t weight, area;
};

// === 扫描结果 ===
struct TScan {
  uint8_t count;       // 触摸点数
  TPoint pts[2];       // 最多支持2点
  uint8_t gesture;     // 手势ID
};

// === 驱动类 ===
class FT6336U {
public:
  FT6336U();

  // 初始化（传引脚则调setPins，不传用默认35/36）
  bool begin(int sda = -1, int scl = -1);

  // 硬件复位
  void reset();

  // 全扫描
  TScan scan();

  // 快速接口
  uint8_t touchCount();
  bool getPoint(uint16_t &x, uint16_t &y);
  bool isTouched();
  bool waitRelease(uint32_t timeout_ms = 500);

private:
  uint8_t  readReg8(uint8_t reg);
  bool     readRegs(uint8_t reg, uint8_t *buf, uint8_t len);
  void     writeReg8(uint8_t reg, uint8_t val);

  uint16_t readCoord12(uint8_t xh_reg, uint8_t xl_reg);
  TouchEvt readEvt(uint8_t xh_reg);
};

#endif

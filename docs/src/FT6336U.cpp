// FT6336U.cpp — 电容触摸屏驱动 (厂家协议精确复刻版)
// 参考: FocalTech FT6X36 Datasheet + 鸿讯电子CL35BC1070-40A Demo
#include "FT6336U.h"

FT6336U::FT6336U() {}

bool FT6336U::begin(int sda, int scl) {
  // 初始化I2C总线
  if (sda >= 0 && scl >= 0) {
    Wire.setPins(sda, scl);
  }
  Wire.begin();
  Wire.setClock(100000);  // 标准100kHz
  Wire.setTimeout(50);

  // 厂家RST时序: LOW→20ms → HIGH→300ms
  pinMode(FT6336U_RST, OUTPUT);
  digitalWrite(FT6336U_RST, LOW);
  delay(20);
  digitalWrite(FT6336U_RST, HIGH);
  delay(300);

  pinMode(FT6336U_INT, INPUT_PULLUP);

  // 读取芯片版本号（不做ID校验，仅日志）
  // 注: 厂家demo代码不做ID检查，直接读取数据
  uint8_t id_a8 = readReg8(0xA8);  // FT5201ID (FT6336U可能不同)
  uint8_t id_a3 = readReg8(0xA3);  // CIPHER
  Serial.printf("[Touch] ID_A8=0x%02X ID_A3=0x%02X\n", id_a8, id_a3);

  // 设置中断模式: 触发模式 (INT下降沿表示有触摸)
  writeReg8(REG_G_MODE, 0x01);
  writeReg8(REG_DEVICE_MODE, 0x00);

  // 尝试读取触摸状态验证通信
  uint8_t td = readReg8(REG_TD_STATUS);
  // 即使返回0xFF(无响应)也继续，留给scan()去处理
  bool ok = (td != 0xFF) || (id_a8 != 0xFF);
  if (ok) {
    Serial.printf("[Touch] OK (TD_STATUS=0x%02X)\n", td);
  } else {
    Serial.println("[Touch] WARN - no ACK from chip");
  }
  return ok;
}

// ====== 寄存器读写 (关键！使用完整的STOP+RESTART协议) ======

// 读单字节: START + Addr_W + RegAddr + STOP + START + Addr_R + Data + NACK + STOP
uint8_t FT6336U::readReg8(uint8_t reg) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return 0xFF;  // NACK = 芯片无响应
  
  if (Wire.requestFrom(FT6336U_ADDR, (uint8_t)1) != 1) return 0xFF;
  return Wire.read();
}

// 读多字节: START + Addr_W + RegAddr + STOP + START + Addr_R + N×Data + NACK + STOP
bool FT6336U::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return false;
  
  if (Wire.requestFrom(FT6336U_ADDR, len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

// 写单字节: START + Addr_W + RegAddr + Data + STOP
void FT6336U::writeReg8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

// ====== 触摸数据读取 ======

// 获取触摸点数 (读TD_STATUS寄存器0x02)
uint8_t FT6336U::touchCount() {
  uint8_t td = readReg8(REG_TD_STATUS);
  if (td == 0xFF) return 0;  // 芯片无响应
  return td & 0x0F;
}

// 获取第一个触摸点的坐标
bool FT6336U::getPoint(uint16_t &x, uint16_t &y) {
  uint8_t buf[4] = {0};
  if (!readRegs(REG_P1_XH, buf, 4)) return false;
  
  // 解析: EventFlag[7:6] + X[11:8] at P1_XH(0x03)
  //       X[7:0] at P1_XL(0x04)
  //       TouchID[7:4] + Y[11:8] at P1_YH(0x05)
  //       Y[7:0] at P1_YL(0x06)
  uint8_t evt = (buf[0] >> 6) & 0x03;
  if (evt == 0 || evt == 2) {  // Put Down or Contact
    x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
    y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    return true;
  }
  return false;  // Put Up or Reserved
}

TScan FT6336U::scan() {
  TScan data = {0};
  uint8_t buf[4];
  
  uint8_t td = readReg8(REG_TD_STATUS);
  if (td == 0xFF) return data;  // 无响应
  data.count = td & 0x0F;
  data.gesture = readReg8(REG_GESTURE_ID);
  
  for (uint8_t i = 0; i < data.count && i < 2; i++) {
    uint8_t base = REG_P1_XH + i * 6;
    if (readRegs(base, buf, 4)) {
      data.pts[i].event = (TouchEvt)((buf[0] >> 6) & 0x03);
      data.pts[i].x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
      data.pts[i].y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    }
  }
  return data;
}

// 快速检查是否有触摸按下
bool FT6336U::isTouched() {
  return touchCount() > 0;
}

bool FT6336U::waitRelease(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    if (!isTouched()) return true;
    delay(10);
  }
  return false;
}

// 硬件复位(外部调用)
void FT6336U::reset() {
  pinMode(FT6336U_RST, OUTPUT);
  digitalWrite(FT6336U_RST, LOW);
  delay(20);
  digitalWrite(FT6336U_RST, HIGH);
  delay(300);
}

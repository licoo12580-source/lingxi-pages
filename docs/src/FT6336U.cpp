// FT6336U.cpp — 电容触摸屏驱动实现
#include "FT6336U.h"

FT6336U::FT6336U() {}

bool FT6336U::begin(int sda, int scl) {
  if (sda >= 0 && scl >= 0) {
    Wire.begin(sda, scl);
  } else {
    Wire.begin(FT6336U_SDA, FT6336U_SCL);
  }
  pinMode(FT6336U_INT, INPUT_PULLUP);
  reset();
  delay(200);

  uint8_t vendor = readReg(REG_VENDOR_ID);
  uint8_t chip = readReg(REG_CHIP_ID);

  Serial.printf("[Touch] Vendor=0x%02X Chip=0x%02X\n", vendor, chip);

  if (vendor != 0x11 || chip != 0x64) {
    Serial.println("[Touch] ID mismatch, trying alternative init...");
    // 有些模块需要额外延时
    delay(500);
    vendor = readReg(REG_VENDOR_ID);
    chip = readReg(REG_CHIP_ID);
    Serial.printf("[Touch] Retry: Vendor=0x%02X Chip=0x%02X\n", vendor, chip);
    if (vendor != 0x11 || chip != 0x64) {
      Serial.println("[Touch] FAILED - touch not available");
      return false;
    }
  }

  // 中断模式: 触发模式 (INT拉低表示有触摸)
  writeReg(REG_G_MODE, 0x01);
  writeReg(REG_DEVICE_MODE, 0x00);

  Serial.println("[Touch] OK");
  return true;
}

void FT6336U::reset() {
  pinMode(FT6336U_RST, OUTPUT);
  digitalWrite(FT6336U_RST, LOW);
  delay(10);
  digitalWrite(FT6336U_RST, HIGH);
  delay(300);
}

uint8_t FT6336U::readReg(uint8_t reg) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(FT6336U_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void FT6336U::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(FT6336U_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint16_t FT6336U::readCoord12(uint8_t xh_reg, uint8_t xl_reg) {
  uint8_t xh = readReg(xh_reg);
  uint8_t xl = readReg(xl_reg);
  return ((uint16_t)(xh & 0x0F) << 8) | xl;
}

TouchEvt FT6336U::readEvt(uint8_t xh_reg) {
  return (TouchEvt)((readReg(xh_reg) >> 6) & 0x03);
}

uint8_t FT6336U::touchCount() {
  return readReg(REG_TD_STATUS) & 0x0F;
}

bool FT6336U::getPoint(uint16_t &x, uint16_t &y) {
  if (touchCount() == 0) return false;
  x = readCoord12(REG_P1_XH, REG_P1_XL);
  y = readCoord12(REG_P1_YH, REG_P1_YL);
  return true;
}

bool FT6336U::isTouched() {
  return touchCount() > 0;
}

TScan FT6336U::scan() {
  TScan data;
  data.count = touchCount();
  data.gesture = readReg(REG_GESTURE_ID);

  if (data.count >= 1) {
    data.pts[0].event = readEvt(REG_P1_XH);
    data.pts[0].x = readCoord12(REG_P1_XH, REG_P1_XL);
    data.pts[0].y = readCoord12(REG_P1_YH, REG_P1_YL);
    data.pts[0].weight = readReg(0x07);
    data.pts[0].area = readReg(0x08) >> 4;
  } else {
    data.pts[0].event = EVT_NONE;
  }

  if (data.count >= 2) {
    data.pts[1].event = readEvt(REG_P2_XH);
    data.pts[1].x = readCoord12(REG_P2_XH, REG_P2_XL);
    data.pts[1].y = readCoord12(REG_P2_YH, REG_P2_YL);
    data.pts[1].weight = readReg(0x0D);
    data.pts[1].area = readReg(0x0E) >> 4;
  } else {
    data.pts[1].event = EVT_NONE;
  }

  return data;
}

bool FT6336U::waitRelease(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    if (!isTouched()) return true;
    delay(10);
  }
  return false;
}

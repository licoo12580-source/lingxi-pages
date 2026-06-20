// FT6336U.cpp — 电容触摸屏驱动 (软件I2C bitbang版)
// 参考: FocalTech FT6X36 Datasheet + 鸿讯电子CL35BC1070-40A Demo
// 完全放弃硬件Wire，使用GPIO bitbang I2C + 内部上拉 + 50ms超时保护

#include "FT6336U.h"

FT6336U::FT6336U() {}

// ====== 软件I2C GPIO操作宏 ======
// 浮空输入 → 外部上拉使SCL/SDA为高
// 推挽输出 → 驱动电平

#define SDA_LOW()   do { gpio_set_level(GPIO_NUM_35, 0); gpio_set_direction(GPIO_NUM_35, GPIO_MODE_OUTPUT_OD); } while(0)
#define SDA_REL()   do { gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT_OUTPUT_OD); gpio_set_level(GPIO_NUM_35, 1); } while(0)
#define SCL_LOW()   do { gpio_set_level(GPIO_NUM_36, 0); gpio_set_direction(GPIO_NUM_36, GPIO_MODE_OUTPUT_OD); } while(0)
#define SCL_REL()   do { gpio_set_direction(GPIO_NUM_36, GPIO_MODE_INPUT_OUTPUT_OD); gpio_set_level(GPIO_NUM_36, 1); } while(0)
#define SDA_READ()  gpio_get_level(GPIO_NUM_35)
#define SCL_READ()  gpio_get_level(GPIO_NUM_36)
#define SW_DELAY    delayMicroseconds(5)

// 初始化GPIO: 内部上拉 + 开漏输出
void FT6336U::sw_init() {
  gpio_reset_pin(GPIO_NUM_35);
  gpio_reset_pin(GPIO_NUM_36);
  gpio_set_pull_mode(GPIO_NUM_35, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GPIO_NUM_36, GPIO_PULLUP_ONLY);
  // 开漏输出(高电平=浮空靠上拉拉高)
  gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_direction(GPIO_NUM_36, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_level(GPIO_NUM_35, 1);
  gpio_set_level(GPIO_NUM_36, 1);
  delayMicroseconds(10);
}

// 总线恢复: 在SCL上发9个时钟脉冲，SDA释放
bool FT6336U::sw_bus_reset() {
  SDA_REL();
  for (int i = 0; i < 9; i++) {
    SCL_REL();
    SW_DELAY;
    SCL_LOW();
    SW_DELAY;
  }
  // 发送STOP
  SDA_LOW();
  SW_DELAY;
  SCL_REL();
  SW_DELAY;
  SDA_REL();
  SW_DELAY;
  return true;
}

void FT6336U::sw_start() {
  // SDA HIGH → SCL HIGH → SDA LOW → SCL LOW
  SDA_REL();
  SW_DELAY;
  SCL_REL();
  SW_DELAY;
  SDA_LOW();
  SW_DELAY;
  SCL_LOW();
  SW_DELAY;
}

void FT6336U::sw_stop() {
  // SDA LOW → SCL HIGH → SDA HIGH
  SDA_LOW();
  SW_DELAY;
  SCL_REL();
  SW_DELAY;
  SDA_REL();
  SW_DELAY;
}

bool FT6336U::sw_write_byte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    if (data & 0x80) SDA_REL(); else SDA_LOW();
    data <<= 1;
    SW_DELAY;
    SCL_REL();
    SW_DELAY;
    SCL_LOW();
    SW_DELAY;
  }
  // 释放SDA等待ACK
  SDA_REL();
  SW_DELAY;
  SCL_REL();
  SW_DELAY;
  int ack = SDA_READ();  // 0=ACK, 1=NACK
  SCL_LOW();
  return (ack == 0);
}

uint8_t FT6336U::sw_read_byte(bool ack) {
  uint8_t data = 0;
  SDA_REL();  // 释放SDA让从机驱动
  for (int i = 0; i < 8; i++) {
    data <<= 1;
    SCL_REL();
    SW_DELAY;
    if (SDA_READ()) data |= 1;
    SCL_LOW();
    SW_DELAY;
  }
  // ACK (LOW) 或 NACK (HIGH)
  if (ack) SDA_LOW(); else SDA_REL();
  SW_DELAY;
  SCL_REL();
  SW_DELAY;
  SCL_LOW();
  SW_DELAY;
  SDA_REL();  // 释放SDA
  return data;
}

// ====== 寄存器读写 ======

uint8_t FT6336U::readReg8(uint8_t reg) {
  uint32_t t0 = millis();

  // START + ADDR_W + REG + STOP
  sw_start();
  if (!sw_write_byte(FT6336U_ADDR_W)) { sw_stop(); return 0xFF; }
  if (!sw_write_byte(reg)) { sw_stop(); return 0xFF; }
  sw_stop();
  if (millis() - t0 > 50) return 0xFF;

  // START + ADDR_R + READ + NACK + STOP
  sw_start();
  if (!sw_write_byte(FT6336U_ADDR_R)) { sw_stop(); return 0xFF; }
  uint8_t val = sw_read_byte(false);  // NACK (最后字节)
  sw_stop();

  return (millis() - t0 <= 50) ? val : 0xFF;
}

bool FT6336U::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
  uint32_t t0 = millis();

  sw_start();
  if (!sw_write_byte(FT6336U_ADDR_W)) { sw_stop(); return false; }
  if (!sw_write_byte(reg)) { sw_stop(); return false; }
  sw_stop();
  if (millis() - t0 > 50) return false;

  sw_start();
  if (!sw_write_byte(FT6336U_ADDR_R)) { sw_stop(); return false; }
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = sw_read_byte(i < len - 1);  // 最后字节NACK
  }
  sw_stop();

  return (millis() - t0 <= 50);
}

void FT6336U::writeReg8(uint8_t reg, uint8_t val) {
  uint32_t t0 = millis();

  sw_start();
  sw_write_byte(FT6336U_ADDR_W);
  sw_write_byte(reg);
  sw_write_byte(val);
  sw_stop();

  // 超时不报错（写入超时不是关键失败）
  (void)t0;
}

// ====== 初始化 ======

bool FT6336U::begin(int sda, int scl) {
  (void)sda; (void)scl;  // 引脚由#define固定

  // 初始化GPIO（内部上拉+开漏）
  sw_init();

  // 总线恢复：先发STOP确保总线空闲
  sw_bus_reset();
  delay(10);

  // 厂家RST时序: LOW→20ms → HIGH→300ms
  gpio_reset_pin(GPIO_NUM_37);  // RST
  gpio_set_direction(GPIO_NUM_37, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_37, 0);
  delay(20);
  gpio_set_level(GPIO_NUM_37, 1);
  delay(300);

  // INT上拉
  gpio_reset_pin(GPIO_NUM_40);
  gpio_set_pull_mode(GPIO_NUM_40, GPIO_PULLUP_ONLY);
  gpio_set_direction(GPIO_NUM_40, GPIO_MODE_INPUT);

  // 读取芯片版本号
  uint8_t id_a8 = readReg8(0xA8);  // VENDOR_ID
  uint8_t id_a3 = readReg8(0xA3);  // CHIP_ID (CIPHER)
  Serial.printf("[Touch] ID_A8=0x%02X ID_A3=0x%02X\n", id_a8, id_a3);

  // 设置中断模式
  writeReg8(REG_G_MODE, 0x01);
  writeReg8(REG_DEVICE_MODE, 0x00);

  // 尝试读取触摸状态验证通信
  uint8_t td = readReg8(REG_TD_STATUS);
  bool ok = (td != 0xFF) || (id_a8 != 0xFF);
  if (ok) {
    Serial.printf("[Touch] OK (TD_STATUS=0x%02X)\n", td);
  } else {
    Serial.println("[Touch] WARN - 芯片无响应（可能未接线或引脚上拉缺失）");
  }
  return ok;
}

// ====== 触摸数据读取 ======

uint8_t FT6336U::touchCount() {
  uint8_t td = readReg8(REG_TD_STATUS);
  if (td == 0xFF) return 0;
  return td & 0x0F;
}

bool FT6336U::getPoint(uint16_t &x, uint16_t &y) {
  uint8_t buf[4] = {0};
  if (!readRegs(REG_P1_XH, buf, 4)) return false;

  uint8_t evt = (buf[0] >> 6) & 0x03;
  if (evt == 0 || evt == 2) {  // Put Down or Contact
    x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
    y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    return true;
  }
  return false;
}

TScan FT6336U::scan() {
  TScan data = {0};
  uint8_t buf[4];

  uint8_t td = readReg8(REG_TD_STATUS);
  if (td == 0xFF) return data;
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

void FT6336U::reset() {
  gpio_reset_pin(GPIO_NUM_37);
  gpio_set_direction(GPIO_NUM_37, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_37, 0);
  delay(20);
  gpio_set_level(GPIO_NUM_37, 1);
  delay(300);
}

#include "gc9106.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

// Code is a quick copy/paste hack from:
//  https://github.com/Boyeen/TFT_eSPI/blob/master/TFT_Drivers/GC9106_Init.h
//  https://github.com/Bodmer/TFT_eSPI/discussions/1310
//

namespace esphome {
namespace gc9106 {

static const uint8_t ST_CMD_DELAY = 0x80;  // special signifier for command lists

static const uint8_t GC9106_NOP = 0x00;
static const uint8_t GC9106_SWRESET = 0x01;
static const uint8_t GC9106_RDDID = 0x04;
static const uint8_t GC9106_RDDST = 0x09;

static const uint8_t GC9106_SLPIN = 0x10;
static const uint8_t GC9106_SLPOUT = 0x11;
static const uint8_t GC9106_PTLON = 0x12;
static const uint8_t GC9106_NORON = 0x13;

static const uint8_t GC9106_INVOFF = 0x20;
static const uint8_t GC9106_INVON = 0x21;
static const uint8_t GC9106_DISPOFF = 0x28;
static const uint8_t GC9106_DISPON = 0x29;
static const uint8_t GC9106_CASET = 0x2A;
static const uint8_t GC9106_RASET = 0x2B;
static const uint8_t GC9106_RAMWR = 0x2C;
static const uint8_t GC9106_RAMRD = 0x2E;

static const uint8_t GC9106_PTLAR = 0x30;
static const uint8_t GC9106_TEOFF = 0x34;
static const uint8_t GC9106_TEON = 0x35;
static const uint8_t GC9106_MADCTL = 0x36;
static const uint8_t GC9106_COLMOD = 0x3A;

static const uint8_t GC9106_MADCTL_MY = 0x80;
static const uint8_t GC9106_MADCTL_MX = 0x40;
static const uint8_t GC9106_MADCTL_MV = 0x20;
static const uint8_t GC9106_MADCTL_ML = 0x10;
static const uint8_t GC9106_MADCTL_RGB = 0x00;

static const uint8_t GC9106_RDID1 = 0xDA;
static const uint8_t GC9106_RDID2 = 0xDB;
static const uint8_t GC9106_RDID3 = 0xDC;
static const uint8_t GC9106_RDID4 = 0xDD;

// Some register settings
static const uint8_t GC9106_MADCTL_BGR = 0x08;

static const uint8_t GC9106_MADCTL_MH = 0x04;

static const uint8_t GC9106_FRMCTR1 = 0xB1;
static const uint8_t GC9106_FRMCTR2 = 0xB2;
static const uint8_t GC9106_FRMCTR3 = 0xB3;
static const uint8_t GC9106_INVCTR = 0xB4;
static const uint8_t GC9106_DISSET5 = 0xB6;

static const uint8_t GC9106_PWCTR1 = 0xC0;
static const uint8_t GC9106_PWCTR2 = 0xC1;
static const uint8_t GC9106_PWCTR3 = 0xC2;
static const uint8_t GC9106_PWCTR4 = 0xC3;
static const uint8_t GC9106_PWCTR5 = 0xC4;
static const uint8_t GC9106_VMCTR1 = 0xC5;

static const uint8_t GC9106_PWCTR6 = 0xFC;

static const uint8_t GC9106_GMCTRP1 = 0xE0;
static const uint8_t GC9106_GMCTRN1 = 0xE1;

// clang-format off
static const uint8_t PROGMEM
  BCMD[] = {                        // Init commands for 7735B screens
    18,                             // 18 commands in list:
    GC9106_SWRESET,   ST_CMD_DELAY, //  1: Software reset, no args, w/delay
      50,                           //     50 ms delay
    GC9106_SLPOUT,    ST_CMD_DELAY, //  2: Out of sleep mode, no args, w/delay
      255,                          //     255 = max (500 ms) delay
    GC9106_COLMOD,  1+ST_CMD_DELAY, //  3: Set color mode, 1 arg + delay:
      0x05,                         //     16-bit color
      10,                           //     10 ms delay
    GC9106_FRMCTR1, 3+ST_CMD_DELAY, //  4: Frame rate control, 3 args + delay:
      0x00,                         //     fastest refresh
      0x06,                         //     6 lines front porch
      0x03,                         //     3 lines back porch
      10,                           //     10 ms delay
    GC9106_MADCTL,  1,              //  5: Mem access ctl (directions), 1 arg:
      0x08,                         //     Row/col addr, bottom-top refresh
    GC9106_DISSET5, 2,              //  6: Display settings #5, 2 args:
      0x15,                         //     1 clk cycle nonoverlap, 2 cycle gate
                                    //     rise, 3 cycle osc equalize
      0x02,                         //     Fix on VTL
    GC9106_INVCTR,  1,              //  7: Display inversion control, 1 arg:
      0x0,                          //     Line inversion
    GC9106_PWCTR1,  2+ST_CMD_DELAY, //  8: Power control, 2 args + delay:
      0x02,                         //     GVDD = 4.7V
      0x70,                         //     1.0uA
      10,                           //     10 ms delay
    GC9106_PWCTR2,  1,              //  9: Power control, 1 arg, no delay:
      0x05,                         //     VGH = 14.7V, VGL = -7.35V
    GC9106_PWCTR3,  2,              // 10: Power control, 2 args, no delay:
      0x01,                         //     Opamp current small
      0x02,                         //     Boost frequency
    GC9106_VMCTR1,  2+ST_CMD_DELAY, // 11: Power control, 2 args + delay:
      0x3C,                         //     VCOMH = 4V
      0x38,                         //     VCOML = -1.1V
      10,                           //     10 ms delay
    GC9106_PWCTR6,  2,              // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    GC9106_GMCTRP1,16,              // 13: Gamma Adjustments (pos. polarity), 16 args + delay:
      0x09, 0x16, 0x09, 0x20,       //     (Not entirely necessary, but provides
      0x21, 0x1B, 0x13, 0x19,       //      accurate colors)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    GC9106_GMCTRN1,16+ST_CMD_DELAY, // 14: Gamma Adjustments (neg. polarity), 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E,       //     (Not entirely necessary, but provides
      0x22, 0x1D, 0x18, 0x1E,       //      accurate colors)
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                           //     10 ms delay
    GC9106_CASET,   4,              // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,                   //     XSTART = 2
      0x00, 0x81,                   //     XEND = 129
    GC9106_RASET,   4,              // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,                   //     XSTART = 1
      0x00, 0x81,                   //     XEND = 160
    GC9106_NORON,     ST_CMD_DELAY, // 17: Normal display on, no args, w/delay
      10,                           //     10 ms delay
    GC9106_DISPON,    ST_CMD_DELAY, // 18: Main screen turn on, no args, delay
      255 },                        //     255 = max (500 ms) delay

  RCMD1[] = {                       // 7735R init, part 1 (red or green tab)
    24,                             // 24 commands in list:
    0xfe, 0,
    0xfe, 0,
    0xfe, 0,
    0xef, 0,
    0xb3, 1,
      0x03,
    0x36, 1,
      0xd8,
    0x3a, 1,
      0x05,
    0xb6, 1,
      0x11,
    0xac, 1,
      0x0b,
    0xb4, 1,
      0x21,
    GC9106_FRMCTR1, 1,
      0xc0,
    0xe6, 2,
      0x50, 0x43,
    0xe7, 2,
      0x56, 0x43,
    0xF0, 14, //gamma 1
      0x1f,
      0x41,
      0x1B,
      0x55,
      0x36,
      0x3d,
      0x3e,
      0x00,
      0x16,
      0x08,
      0x09,
      0x15,
      0x14,
      0xf,
    0xF1, 14, //gamma 2
      0x1f,
      0x41,
      0x1B,
      0x55,
      0x36,
      0x3d,
      0x3e,
      0x00,
      0x16,
      0x08,
      0x09,
      0x15,
      0x14,
      0xf,
    0xfe, 0,
    0xff, 0,
    GC9106_TEON, 1,
      0x00,
    0x44, 1,
      0x00,
    GC9106_SLPOUT, 0,
    GC9106_DISPON, 0,
    GC9106_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x18,                   //     XSTART = 0
      0x00, 0x67,                   //     XEND = 79
    GC9106_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x9F,
    0x2c };

// clang-format on
static const char *const TAG = "gc9106";

GC9106::GC9106(int width, int height, int colstart, int rowstart, bool eightbitcolor, bool usebgr,
               bool invert_colors)
    : colstart_(colstart),
      rowstart_(rowstart),
      eightbitcolor_(eightbitcolor),
      usebgr_(usebgr),
      invert_colors_(invert_colors),
      width_(width),
      height_(height) {}

void GC9106::setup() {
  ESP_LOGCONFIG(TAG, "Setting up GC9106...");
  this->spi_setup();

  this->dc_pin_->setup();  // OUTPUT
  this->cs_->setup();      // OUTPUT

  this->dc_pin_->digital_write(true);
  this->cs_->digital_write(true);

  this->init_reset_();
  delay(100);  // NOLINT

  ESP_LOGD(TAG, "  START");
  dump_config();
  ESP_LOGD(TAG, "  END");

  display_init_(RCMD1);
  height_ == 0 ? height_ = GC9106_TFTHEIGHT_160 : height_;
  width_ == 0 ? width_ = GC9106_TFTWIDTH_80 : width_;
  colstart_ == 0 ? colstart_ = 24 : colstart_;
  rowstart_ == 0 ? rowstart_ = 0 : rowstart_;

  uint8_t data = 0;
  data = GC9106_MADCTL_MX | GC9106_MADCTL_MY;
  if (this->usebgr_) {
    data = data | GC9106_MADCTL_BGR;
  } else {
    data = data | GC9106_MADCTL_RGB;
  }
  sendcommand_(GC9106_MADCTL, &data, 1);

  if (this->invert_colors_)
    sendcommand_(GC9106_INVON, nullptr, 0);

  this->init_internal_(this->get_buffer_length());
  memset(this->buffer_, 0x00, this->get_buffer_length());
}

void GC9106::update() {
  this->do_update_();
  this->write_display_data_();
}

int GC9106::get_height_internal() { return height_; }

int GC9106::get_width_internal() { return width_; }

size_t GC9106::get_buffer_length() {
  if (this->eightbitcolor_) {
    return size_t(this->get_width_internal()) * size_t(this->get_height_internal());
  }
  return size_t(this->get_width_internal()) * size_t(this->get_height_internal()) * 2;
}

void HOT GC9106::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0)
    return;

  if (this->eightbitcolor_) {
    const uint32_t color332 = display::ColorUtil::color_to_332(color);
    uint16_t pos = (x + y * this->get_width_internal());
    this->buffer_[pos] = color332;
  } else {
    const uint32_t color565 = display::ColorUtil::color_to_565(color);
    uint16_t pos = (x + y * this->get_width_internal()) * 2;
    this->buffer_[pos++] = (color565 >> 8) & 0xff;
    this->buffer_[pos] = color565 & 0xff;
  }
}

void GC9106::init_reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(1);
    // Trigger Reset
    this->reset_pin_->digital_write(false);
    delay(10);
    // Wake up
    this->reset_pin_->digital_write(true);
  }
}

void GC9106::display_init_(const uint8_t *addr) {
  uint8_t num_commands, cmd, num_args;
  uint16_t ms;

  num_commands = progmem_read_byte(addr++);  // Number of commands to follow
  while (num_commands--) {                   // For each command...
    cmd = progmem_read_byte(addr++);         // Read command
    num_args = progmem_read_byte(addr++);    // Number of args to follow
    ms = num_args & ST_CMD_DELAY;            // If hibit set, delay follows args
    num_args &= ~ST_CMD_DELAY;               // Mask out delay bit
    this->sendcommand_(cmd, addr, num_args);
    addr += num_args;

    if (ms) {
      ms = progmem_read_byte(addr++);  // Read post-command delay time (ms)
      if (ms == 255)
        ms = 500;  // If 255, delay for 500 ms
      delay(ms);
    }
  }
}

void GC9106::dump_config() {
  LOG_DISPLAY("", "GC9106", this);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGD(TAG, "  Buffer Size: %zu", this->get_buffer_length());
  ESP_LOGD(TAG, "  Height: %d", this->height_);
  ESP_LOGD(TAG, "  Width: %d", this->width_);
  ESP_LOGD(TAG, "  ColStart: %d", this->colstart_);
  ESP_LOGD(TAG, "  RowStart: %d", this->rowstart_);
  LOG_UPDATE_INTERVAL(this);
}

void HOT GC9106::writecommand_(uint8_t value) {
  this->enable();
  this->dc_pin_->digital_write(false);
  this->write_byte(value);
  this->dc_pin_->digital_write(true);
  this->disable();
}

void HOT GC9106::writedata_(uint8_t value) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(value);
  this->disable();
}

void HOT GC9106::sendcommand_(uint8_t cmd, const uint8_t *data_bytes, uint8_t num_data_bytes) {
  this->writecommand_(cmd);
  this->senddata_(data_bytes, num_data_bytes);
}

void HOT GC9106::senddata_(const uint8_t *data_bytes, uint8_t num_data_bytes) {
  this->dc_pin_->digital_write(true);  // pull DC high to indicate data
  this->cs_->digital_write(false);
  this->enable();
  for (uint8_t i = 0; i < num_data_bytes; i++) {
    this->write_byte(progmem_read_byte(data_bytes++));  // write byte - SPI library
  }
  this->cs_->digital_write(true);
  this->disable();
}

void HOT GC9106::write_display_data_() {
  uint16_t offsetx = colstart_;
  uint16_t offsety = rowstart_;

  uint16_t x1 = offsetx;
  uint16_t x2 = x1 + get_width_internal() - 1;
  uint16_t y1 = offsety;
  uint16_t y2 = y1 + get_height_internal() - 1;

  this->enable();

  // set column(x) address
  this->dc_pin_->digital_write(false);
  this->write_byte(GC9106_CASET);
  this->dc_pin_->digital_write(true);
  this->spi_master_write_addr_(x1, x2);

  // set Page(y) address
  this->dc_pin_->digital_write(false);
  this->write_byte(GC9106_RASET);
  this->dc_pin_->digital_write(true);
  this->spi_master_write_addr_(y1, y2);

  //  Memory Write
  this->dc_pin_->digital_write(false);
  this->write_byte(GC9106_RAMWR);
  this->dc_pin_->digital_write(true);

  if (this->eightbitcolor_) {
    for (size_t line = 0; line < this->get_buffer_length(); line = line + this->get_width_internal()) {
      for (int index = 0; index < this->get_width_internal(); ++index) {
        auto color332 = display::ColorUtil::to_color(this->buffer_[index + line], display::ColorOrder::COLOR_ORDER_RGB,
                                                     display::ColorBitness::COLOR_BITNESS_332, true);

        auto color = display::ColorUtil::color_to_565(color332);

        this->write_byte((color >> 8) & 0xff);
        this->write_byte(color & 0xff);
      }
    }
  } else {
    this->write_array(this->buffer_, this->get_buffer_length());
  }
  this->disable();
}

void GC9106::spi_master_write_addr_(uint16_t addr1, uint16_t addr2) {
  static uint8_t byte[4];
  byte[0] = (addr1 >> 8) & 0xFF;
  byte[1] = addr1 & 0xFF;
  byte[2] = (addr2 >> 8) & 0xFF;
  byte[3] = addr2 & 0xFF;

  this->dc_pin_->digital_write(true);
  this->write_array(byte, 4);
}

void GC9106::spi_master_write_color_(uint16_t color, uint16_t size) {
  static uint8_t byte[1024];
  int index = 0;
  for (int i = 0; i < size; i++) {
    byte[index++] = (color >> 8) & 0xFF;
    byte[index++] = color & 0xFF;
  }

  this->dc_pin_->digital_write(true);
  return write_array(byte, size * 2);
}

}  // namespace GC9106
}  // namespace esphome

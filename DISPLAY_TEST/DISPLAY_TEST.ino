
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_NV3052C _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
public:
  LGFX(void) {
    { // SPI バス設定
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI0_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000; // 40MHz
      cfg.pin_sclk = 18;
      cfg.pin_mosi = 19;
      cfg.pin_miso = -1; // 読み出し不使用
      cfg.pin_dc   = -1; // NV3052CはDC不要（9bitSPI）
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    { // パネル設定
      auto cfg = _panel_instance.config();
      cfg.pin_cs   = 17;
      cfg.pin_rst  = 16;
      cfg.pin_busy = -1;
      cfg.panel_width  = 720;
      cfg.panel_height = 720; // 製品仕様に合わせて調整
      cfg.offset_rotation = 0;
      _panel_instance.config(cfg);
    }
    { // バックライト
      auto cfg = _light_instance.config();
      cfg.pin_bl = 15;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 0;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
  }
};

static LGFX lcd;

void setup() {
  Serial.begin(115200);

  lcd.init();
  Serial.println("init done");

  // 赤で塗りつぶし
  lcd.fillScreen(TFT_RED);
  Serial.println("fillScreen RED done");
  delay(1000);

  // 緑
  lcd.fillScreen(TFT_GREEN);
  delay(1000);

  // 青
  lcd.fillScreen(TFT_BLUE);
  delay(1000);
}

void loop() {}


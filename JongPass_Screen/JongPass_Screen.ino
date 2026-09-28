#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ピン定義（回路に合わせて設定）
#define PIN_SCK   18
#define PIN_MOSI  19
#define PIN_DC    20
#define PIN_RST   21
#define PIN_CS1   22
#define PIN_CS2   17

#define PIN_LGT1  28

class LGFX_ILI9225 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9225 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;

public:
  LGFX_ILI9225(int pin_cs) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host   = 0;          // SPI0
      cfg.spi_mode   = 0;
      cfg.freq_write = 16000000;   // 安定動作のため16MHzに低減
      cfg.pin_sclk   = PIN_SCK;
      cfg.pin_mosi   = PIN_MOSI;
      cfg.pin_miso   = -1;
      cfg.pin_dc     = PIN_DC;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs       = pin_cs;
      cfg.pin_rst      = -1;       // ★個別RSTは無効化（共通ピンのため手動で同時リセットする）
      cfg.pin_busy     = -1;
      cfg.panel_width  = 176;
      cfg.panel_height = 220;
      cfg.offset_x     = 0;
      cfg.offset_y     = 0;
      cfg.bus_shared   = true;
      cfg.invert       = false;
      cfg.rgb_order    = false;

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

LGFX_ILI9225 tft1(PIN_CS1);
LGFX_ILI9225 tft2(PIN_CS2);
int isBright;

void drawMahjongPoints(LovyanGFX &lcd, LovyanGFX &lcd2) {
  int32_t w = lcd.width();   // 176
  int32_t h = lcd.height();  // 220

  lcd.fillScreen(TFT_BLACK);
  lcd2.fillScreen(TFT_BLACK);

  // ==================== 画面1 ====================
  {
    LGFX_Sprite spr(&lcd);
    spr.setColorDepth(16);
    spr.createSprite(120, 36);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.setTextDatum(middle_center);
    spr.setFont(&fonts::efontJA_16);
    spr.setTextSize(1);
    spr.setPivot(60, 18); // スプライトの中心(120/2, 36/2)を回転軸に設定

    // 下辺: 西 点数 (0度)
    spr.fillScreen(TFT_BLACK);
    spr.drawString("25000", 60, 18);
    spr.pushRotateZoom(w / 2, h - 20, 0, 1.0, 1.0);

    // 上辺: 東 方角 (180度)
    spr.fillScreen(TFT_BLACK);
    spr.drawString("東", 60, 18);
    spr.pushRotateZoom(w / 2, 20, 180, 1.0, 1.0);

    // 左辺: 東1局 (90度)
    spr.fillScreen(TFT_BLACK);
    spr.drawString("東1局", 60, 18);
    spr.pushRotateZoom(20, h / 2, 90, 1.0, 1.0);

    // 右辺: 北 25000 (270度)
    spr.fillScreen(TFT_BLACK);
    spr.drawString("北 25000", 60, 18);
    spr.pushRotateZoom(w - 20, h / 2, 270, 1.0, 1.0);

    spr.deleteSprite();
  }

  delay(20);

  // ==================== 画面2 ====================
  {
    LGFX_Sprite spr2(&lcd2);
    spr2.setColorDepth(16);
    spr2.createSprite(120, 36);
    spr2.setTextColor(TFT_WHITE, TFT_BLACK);
    spr2.setTextDatum(middle_center);
    spr2.setFont(&fonts::efontJA_16);
    spr2.setTextSize(1);
    spr2.setPivot(60, 18);

    // 下辺: 西 方角 (0度)
    spr2.fillScreen(TFT_BLACK);
    spr2.drawString("西", 60, 18);
    spr2.pushRotateZoom(w / 2, h - 20, 0, 1.0, 1.0);

    // 上辺: 東 点数 (180度)
    spr2.fillScreen(TFT_BLACK);
    spr2.drawString("25000", 60, 18);
    spr2.pushRotateZoom(w / 2, 20, 180, 1.0, 1.0);

    // 左辺: 南 25000 (90度)
    spr2.fillScreen(TFT_BLACK);
    spr2.drawString("南 25000", 60, 18);
    spr2.pushRotateZoom(20, h / 2, 90, 1.0, 1.0);

    // 右辺: 0本場 (90度に変更)
    spr2.fillScreen(TFT_BLACK);
    spr2.drawString("0本場", 60, 18);
    spr2.pushRotateZoom(w - 20, h / 2, 90, 1.0, 1.0);

    spr2.deleteSprite();
  }
}

void setup() {
  Serial.begin(115200);
  // 1. 各ピンの初期電圧を確定させる（CSは非アクティブ=HIGH）
  pinMode(PIN_CS1, OUTPUT);
  pinMode(PIN_CS2, OUTPUT);
  pinMode(PIN_LGT1, INPUT);

  digitalWrite(PIN_CS1, HIGH);
  digitalWrite(PIN_CS2, HIGH);

  // 2. 共通RSTピンを手動で叩いて両方のディスプレイを同時にハードウェアリセット
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, HIGH);
  delay(10);
  digitalWrite(PIN_RST, LOW);
  delay(20);
  digitalWrite(PIN_RST, HIGH);
  delay(50);

  // 3. 描画
  tft1.init();
  tft1.setRotation(0);
  delay(10);
  tft2.init();
  tft2.setRotation(0);
  delay(20);
  drawMahjongPoints(tft1, tft2);
}

void loop() {
  isBright = digitalRead(PIN_LGT1);
  Serial.println(isBright);
  delay(100);
}

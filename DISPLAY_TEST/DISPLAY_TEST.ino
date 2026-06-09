#include <LovyanGFX.hpp>

// ============================================================================
// 1. 今回の4.0インチ正方形液晶(NV3052C)専用の設定クラスを定義
// ============================================================================
class LGFX_JongPass : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel_instance; // NV3052CのSPIモードは基本ST7789系のコマンドと互換性があります
    lgfx::Bus_SPI       _bus_instance;

public:
    LGFX_JongPass() {
        // ① SPIバスの設定（Pico 2 W の5本配線に合わせる）
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host   = 0;          // PicoのSPI0を使用
            cfg.spi_mode   = 0;          // SPIモード0
            cfg.freq_write = 40000000;   // 書き込みクロック（まずは安全に40MHz）
            cfg.freq_read  = 16000000;   // 読み込みクロック
            cfg.pin_sclk   = 18;         // 【25番ピン】SCL ──► GPIO18
            cfg.pin_mosi   = 19;         // 【26番ピン】SDA ──► GPIO19
            cfg.pin_miso   = -1;         // 読み込み線は使わないので -1 (無し)
            cfg.pin_dc     = 20;         // 【27番ピン】DC  ──► GPIO20
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // ② 液晶パネル本体の設定（720x720 正方形仕様）
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 17;   // 【24番ピン】CS ──► GPIO17
            cfg.pin_rst          = 21;   // 【28番ピン】RESET ──► GPIO21
            
            // パネルの解像度設定
            cfg.panel_width      = 720;  // 横幅 720 ピクセル
            cfg.panel_height     = 720;  // 縦幅 720 ピクセル
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            
            // 【修正】エラーの原因となっていたメンバー名を最新の仕様に修正
            cfg.invert           = false; // 画面の色が反転（ネガ反転）する場合はここを true にする
            cfg.rgb_order        = false; // 赤と青が入れ替わる場合はここを true にする

            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

// 作成したクラスのインスタンスを生成
LGFX_JongPass lcd;

// ============================================================================
// 2. メインプログラム
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== 麻雀コンパス 液晶テスト開始 ===");

    // 液晶の初期化
    lcd.init();
    
    // 画面の向き（0:普通）
    lcd.setRotation(0); 

    // 画面を一度真っ白にクリア
    lcd.clear(TFT_WHITE);
}

void loop() {
    Serial.println("画面を赤色にします");
    lcd.fillScreen(TFT_RED); // 全面赤色
    delay(2000);

    Serial.println("画面を緑色にします");
    lcd.fillScreen(TFT_GREEN); // 全面緑色
    delay(2000);

    Serial.println("画面を青色にします");
    lcd.fillScreen(TFT_BLUE); // 全面青色
    delay(2000);

    // 文字の描画テスト
    lcd.fillScreen(TFT_BLACK); // 背景黒
    lcd.setTextColor(TFT_YELLOW); // 文字色黄
    lcd.setTextSize(4); // 文字サイズ
    lcd.setCursor(100, 300); // 画面中央付近に移動
    lcd.print("JongPass Test!");
    delay(5000);
}
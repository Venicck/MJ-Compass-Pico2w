#include <BLE.h>
#include <map>
#include <functional>
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

BLEServiceUART uart;
bool flagSend = false;
bool is_connected = false;
const int boardLED = LED_BUILTIN;

// 麻雀コンパスのグローバル変数
int scores[] = {25000, 25000, 25000, 25000};
bool reach[] = {false, false, false, false};
int oya = 0;
int honba = 0;
int kyotaku = 0;
bool gamehalf = false; // false:東場 true:南場
bool sanma = false; // 三麻

String directionOf(int player) {
    String dirs[] = {"東", "南", "西", "北"};
    if (sanma) {
        return dirs[(oya + player) % 3];
    } else {
        return dirs[(oya + player) % 4];
    }
}

// 画面処理

// 1つのテキストをスプライト経由で指定位置・角度に描画するヘルパー関数
void drawItem(LovyanGFX &lcd, LGFX_Sprite &spr, const String &text, int32_t x, int32_t y, float angle, bool isRed, bool isHighlight) {
  uint16_t bg_color = TFT_BLACK;
  uint16_t text_color = isRed ? TFT_RED : TFT_WHITE;
  if (isHighlight) {
    uint16_t tmp = text_color;
    text_color = bg_color;
    bg_color = tmp;
  }
  
  spr.fillScreen(bg_color);
  spr.setTextColor(text_color, bg_color);
  spr.drawString(text, 60, 18);
  spr.pushRotateZoom(x, y, angle, 1.0, 1.0);
}

void drawMahjongPoints() {
  int32_t w = tft1.width();   // 176
  int32_t h = tft1.height();  // 220

  // ==================== 画面1 ====================
  {
    LGFX_Sprite spr(&tft1);
    spr.setColorDepth(16);
    spr.createSprite(120, 36);
    spr.setTextDatum(middle_center);
    spr.setFont(&fonts::efontJA_16);
    spr.setTextSize(1);
    spr.setPivot(60, 18); // スプライトの中心(120/2, 36/2)を回転軸に設定

    // 下辺: 西 点数 (0度)
    drawItem(tft1, spr, String(scores[2]), w / 2, h - 20, 0, false, false);

    // 上辺: 東 方角 (180度)
    drawItem(tft1, spr, directionOf(0), w / 2, 20, 180, oya == 0, false);

    // 左辺: 東1局 (90度)
    drawItem(tft1, spr, "東" + String(oya+1) + "局", 20, h / 2, 90, false, false);

    // 右辺: 北 25000 (270度)
    drawItem(tft1, spr, directionOf(3) + " " + String(scores[3]), w - 20, h / 2, 270, false, reach[3]);

    spr.deleteSprite();
  }

  delay(20);

  // ==================== 画面2 ====================
  {
    LGFX_Sprite spr2(&tft2);
    spr2.setColorDepth(16);
    spr2.createSprite(120, 36);
    spr2.setTextColor(TFT_WHITE, TFT_BLACK);
    spr2.setTextDatum(middle_center);
    spr2.setFont(&fonts::efontJA_16);
    spr2.setTextSize(1);
    spr2.setPivot(60, 18);

    // 下辺: 西 方角 (0度)
    drawItem(tft2, spr2, directionOf(2), w / 2, h - 20, 0, false, reach[2]);

    // 上辺: 東 点数 (180度)
    drawItem(tft2, spr2, String(scores[0]), w / 2, 20, 180, false, reach[0]);

    // 左辺: 南 方角と点数 (90度)
    drawItem(tft2, spr2, directionOf(1)+" "+String(scores[1]), 20, h / 2, 90, false, reach[1]);

    // 右辺: 0本場 (90度に変更)
    drawItem(tft2, spr2, String(honba) + "本場", w - 20, h / 2, 90, false, false);

    spr2.deleteSprite();
  }
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        Serial.println("[BLE] Device Connected.");
        is_connected = true;
        flagSend = true;
    }
    void onDisconnect(BLEServer* pServer) override {
        Serial.println("[BLE] Device Disconnected.");
        is_connected = false;
        flagSend = false;
        BLE.startAdvertising();
    }
};

// ===================================================================
// 1. 各コマンドに対応する処理関数（クリーンアップ版）
// ===================================================================

void Reset(String payload) {
    oya = 0;
    honba = 0;
    kyotaku = 0; // 供託もリセットに追加
    gamehalf = false;
    if (sanma) {
        for (int i = 0; i < 4; i++) {
            scores[i] = 30000;
            reach[i] = false;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            scores[i] = 25000;
            reach[i] = false;
        }
    }
    Serial.println("[Func] リセットしました");
    SendToBrw(); // ブラウザ同期
}

// 流局（または局進行ボタン）が届いたときの関数
void Ryukyoku(String payload) {
    Serial.print("[Func] 局を進めました。");
    
    // 親がリーチ（またはテンパイ）していなければ親移動
    if (!reach[oya]) {
        oya++;
        honba = 0;
    } else {
        honba++;
    }
    
    if ((oya > 3 && !sanma) || (oya > 2 && sanma)) {
        oya = 0;
        gamehalf = !gamehalf;
    }
    
    // 全員のリーチフラグを下ろす
    for (int i = 0; i < 4; i++) {
        reach[i] = false;
    }

    if (!gamehalf) Serial.print("東");
    else Serial.print("南");
    Serial.println(String(oya + 1) + "局 " + String(honba) + "本場");
    
    SendToBrw(); // ブラウザ同期
    drawMahjongPoints();
}

void Reach(String payload) {
    int player = payload[0] - '0';
    if (reach[player]) {
        Serial.println("[Func] プレイヤー" + String(player) + "は既にリーチ");
        return;
    }
    Serial.println("[Func] プレイヤー" + String(player) + "がリーチ");
    reach[player] = true;
    kyotaku++; // 供託リーチ棒を1本増やす
    scores[player] -= 1000; // 

    SendToBrw(); // ブラウザ同期
    drawMahjongPoints();
}

void Tumo(String payload) { // フォーマット例："0<-2000_4000"
    Serial.println("[Func] ツモ!");
    int who = payload[0] - '0';
    int sep = payload.indexOf("_");
    
    int ko_transit = payload.substring(3, sep).toInt();
    int oya_transit = payload.substring(sep + 1).toInt();
    
    if (who == oya) {
        Serial.println("--- 親ツモ ---");
        if (sanma) {
            for (int i = 0; i < 3; i++) {
                if (i != oya) {
                    scores[i] -= ko_transit;
                    scores[who] += ko_transit;
                }
            }
        } else {
            for (int i = 0; i < 4; i++) {
                if (i != oya) {
                    scores[i] -= ko_transit;
                    scores[who] += ko_transit;
                }
            }
        }
        
        honba++; // 親連荘
    } else {
        // ==========================================
        // 2. 子のツモあがり
        // ==========================================
        Serial.println("--- 子ツモ ---");
        
        // ① 親からの支払い
        scores[oya] -= oya_transit;
        scores[who] += oya_transit;
        
        if (sanma) {
            for (int i = 0; i < 3; i++) {
                if (i != oya && i != who) {
                    scores[i] -= ko_transit;
                    scores[who] += ko_transit;
                }
            }
        } else {
            for (int i = 0; i < 4; i++) {
                if (i != oya && i != who) {
                    scores[i] -= ko_transit;
                    scores[who] += ko_transit;
                }
            }
        }
    }
    
    // 全員のリーチ状態をリセット
    for (int i = 0; i < 4; i++) {
        reach[i] = false;
    }
    
    // 供託リーチ棒の回収
    if (kyotaku) {
        Serial.println("供託" + String(kyotaku * 1000) + "点 -> プレイヤー" + String(who));
        scores[who] += kyotaku * 1000;
        kyotaku = 0;
    }
    
    // 親の移動を関数の一番最後に持ってくることで、表示のズレを防ぐ
    if (who != oya) {
        oya++;
        honba = 0; // 子あがりなので本場はリセット
        if (oya > 2 && sanma) {
            oya = 0;
            gamehalf = !gamehalf;
        } else if (oya > 3) {
            oya = 0;
            gamehalf = !gamehalf;
        }
    }
    
    printScores();
    drawMahjongPoints();
    SendToBrw(); // 最後に一回だけスマホへ同期
}

void Ron(String payload) { // "和了<-打った人_点数"
    Serial.println("[Func] ロン!");
    int whogets = payload[0] - '0';
    int whopays = payload[3] - '0';
    int transit = payload.substring(5).toInt();
    
    Serial.println("プレイヤー" + String(whopays) + " -> " + String(transit) + " -> プレイヤー" + String(whogets));
    scores[whogets] += transit;
    scores[whopays] -= transit;
    
    // 供託回収
    scores[whogets] += kyotaku * 1000;
    kyotaku = 0;
    
    // リーチ状態リセット
    for (int i = 0; i < 4; i++) {
        reach[i] = false;
    }

    // あがったのが親か子かで親移動・連荘を処理
    if (whogets == oya) {
        honba++;
    } else {
        oya++;
        honba = 0;
        if (oya > 3) {
            oya = 0;
            gamehalf = !gamehalf;
        }
    }

    printScores();
    drawMahjongPoints();
    SendToBrw(); // ブラウザ同期
}

void TogglePeople() {
    sanma = !sanma;
    Reset("");
    SendToBrw();
}

void printScores() {
    Serial.print("【現在の点数】 ");
    for(int i = 0; i < 4; i++) {
        Serial.print("P" + String(i) + ":" + String(scores[i]) + "点  ");
    }
    Serial.println();
}

void SendToBrw() { // BTデバイスに送信
    //1行目:点数 2行目:リーチ 3行目:三麻? 南場? 親? 本場? 供託?
    String msg[] = {"", "", ""};
    for (int i=0;i<3;i++) {
        msg[0] += String(scores[i]);
        msg[0] += "_";
        if (reach[i]) msg[1] += "T_";
        else msg[1] += "F_";
    }
    msg[0] += String(scores[3]);
    if (reach[3]) msg[1] += "T";
    else msg[1] += "F";

    if (sanma) msg[2] += "T ";
    else msg[2] += "F ";
    if (gamehalf) msg[2] += "T ";
    else msg[2] += "F ";
    msg[2] += String(oya) + " " + String(honba) + " " + String(kyotaku);
    for (int i=0;i<3;i++) {
        uart.print(msg[i] + "\n");
    }
}
// -------------------------------------------------------------------
// 2. コマンド名と関数を紐付ける関数リスト
// -------------------------------------------------------------------
std::map<String, std::function<void(String)>> commandMap;
void setupCommandMap() {
    commandMap["NEXT"] = Ryukyoku;
    commandMap["TUMO:"] = Tumo;
    commandMap["RON:"] = Ron;
    commandMap["RESET"] = Reset;
    commandMap["REACH:"] = Reach;
}
// -------------------------------------------------------------------
// 3. メインシステム
// -------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(boardLED, OUTPUT);

    Serial.println("JongPass Started.");

    // コマンド関数リストの初期化
    setupCommandMap();

    BLE.begin("JongPass");

    BLE.server()->setCallbacks(new MyServerCallbacks());
    BLE.server()->addService(&uart);
    BLE.startAdvertising();
    uart.setAutoflush(50);
    
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
  tft1.fillScreen(TFT_BLACK);
  delay(10);
  tft2.init();
  tft2.setRotation(0);
  tft2.fillScreen(TFT_BLACK);
  delay(20);
  
  drawMahjongPoints();
}

void loop() {
    // 明るさセンサー
    isBright = digitalRead(PIN_LGT1);
    Serial.println(isBright);
    
    if (!is_connected) {
        if (millis()%500<250) {
            digitalWrite(boardLED, HIGH);
        } else {
            digitalWrite(boardLED, LOW);
        }
    }
    if (flagSend) {
        flagSend = false;        
        digitalWrite(boardLED, LOW);
        Serial.println("現在のデータを送信中...");
        delay(2000);
        SendToBrw();
    }
    if (uart.available()) {
        String receivedText = uart.readString();
        receivedText.trim();

        Serial.print("【受信データ】: ");
        Serial.println(receivedText);

        // 一致するコマンドがあるか、関数リスト（Map）から探索する
        bool commandFound = false;

        for (auto const& [key, func] : commandMap) {
        // 受信文字が、登録されたキー（"P1:" や "NEXT"）で始まっているか確認
        if (receivedText.startsWith(key)) {
            // キー以降の残りのデータ（引数、ペイロード）を切り出す
            // 例："P1:32000" から "32000" を抽出。 "NEXT" の場合は空文字になる
            String payload = receivedText.substring(key.length());
            
            // 紐付けられた関数を実行！
            func(payload);
            
            commandFound = true;
            break; // マッチしたらループを抜ける
        }
        }

        if (!commandFound) {
            Serial.print("エラー: 未登録のコマンドです -> ");
            Serial.println(receivedText);
        }
    }
}
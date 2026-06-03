#include <BLE.h>
#include <map>
#include <functional>

BLEServiceUART uart;
bool flag_Connect = false;
bool is_connected = false;
const int boardLED = LED_BUILTIN;

// 麻雀コンパスのグローバル変数
int scores[] = {25000, 25000, 25000, 25000};
bool reach[] = {false, false, false, false};
int oya = 0;
int honba = 0;
int kyotaku = 0;
bool gamehalf = false; // false:東場 true:南場

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        Serial.println("[BLE] Device Connected.");
        flag_Connect = true;
        is_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("[BLE] Device Disconnected.");
        is_connected = false;
        // 切断されたら、自動的に再度アドバタイズ（電波発信）を開始して再接続を待つ
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
    for (int i = 0; i < 4; i++) {
        scores[i] = 25000;
        reach[i] = false;
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
    
    if (oya > 3) {
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
}

void Reach(String payload) {
    int player = payload[0] - '0';
    if (player >= 0 && player <= 3) {
        if (reach[player]) {
            Serial.println("[Func] プレイヤー" + String(player) + "は既にリーチ");
            return;
        }
        Serial.println("[Func] プレイヤー" + String(player) + "がリーチ");
        reach[player] = true;
        kyotaku++; // 供託リーチ棒を1本増やす
        scores[player] -= 1000; // 
    }
    SendToBrw(); // ブラウザ同期
}

void Tumo(String payload) { // フォーマット例："0<-2000_4000"
    Serial.println("[Func] ツモ!");
    int who = payload[0] - '0';
    int sep = payload.indexOf("_");
    
    int ko_transit = payload.substring(3, sep).toInt();
    int oya_transit = payload.substring(sep + 1).toInt();
    
    if (who == oya) {
        // 親のツモあがり
        Serial.println("--- 親ツモ ---");
        for (int i = 0; i < 4; i++) {
            if (i != oya) {
                scores[i] -= ko_transit;
                scores[who] += ko_transit;
            }
        }
        honba++; // 親連荘なので本場を増やす
    } else {
        // 子のツモあがり
        Serial.println("--- 子ツモ ---");
        scores[oya] -= oya_transit;
        scores[who] += oya_transit;
        
        for (int i = 0; i < 4; i++) {
            if (i != oya && i != who) {
                scores[i] -= ko_transit;
                scores[who] += ko_transit;
            }
        }
        
        // 子が上がったので、次の局へ移るために親を回す処理
        oya++; 
        honba = 0; // 子あがりなので本場はリセット
        if (oya > 3) {
            oya = 0;
            gamehalf = !gamehalf;
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
    
    printScores();
    SendToBrw(); // ブラウザ同期
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
    
    // リーチ状態リセット
    for (int i = 0; i < 4; i++) {
        reach[i] = false;
    }

    printScores();
    SendToBrw(); // ブラウザ同期
}

void printScores() {
    Serial.print("【現在の点数】 ");
    for(int i = 0; i < 4; i++) {
        Serial.print("P" + String(i) + ":" + String(scores[i]) + "点  ");
    }
    Serial.println();
    Serial.print("【現在の点数】 ");
    for(int i = 0; i < 4; i++) {
        Serial.print("P" + String(i) + ":" + String(scores[i]) + "点  ");
    }
    Serial.println();
}

void SendToBrw() { // BTデバイスに送信
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
    if (gamehalf) msg[2] += "T ";
    else msg[2] += "F ";
    msg[2] += String(oya) + " " + String(honba) + " " + String(kyotaku);
    for (int i=0;i<3;i++) {
        uart.println(msg[i]);
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

    Serial.println("=== 麻雀コンパス コマンドマップモード 起動 ===");

    // コマンド関数リストの初期化
    setupCommandMap();

    BLE.begin("JongPass");

    BLE.server()->setCallbacks(new MyServerCallbacks());
    BLE.server()->addService(&uart);
    BLE.startAdvertising();
    uart.setAutoflush(50);
}

void loop() {
    if (!is_connected) {
        if (millis()%500<250) {
            digitalWrite(boardLED, HIGH);
        } else {
            digitalWrite(boardLED, LOW);
        }
    }
    if (flag_Connect) {
        delay(500);
        Serial.println("現在のデータを送信中...");
        SendToBrw();
        flag_Connect = false;
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
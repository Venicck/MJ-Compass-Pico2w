#include <SerialBT.h>

int score[] = {25000, 25000, 25000, 25000};

void setup() {
  Serial.begin(115200);
  SerialBT.setName("Mahjong");
  SerialBT.begin(9600);
}

void process_bt() {
  String req = "";
  while (SerialBT.available()) {
    char c = SerialBT.read();
    req += c;
  }
  Serial.println("Recieved: " + req);
  score[req[0]-'0'] = req.substring(2).toInt();
}

void loop() {
  if (SerialBT.available()) {
    process_bt();
  }
  Serial.println("-------");
  Serial.println("東:" + String(score[0]));
  Serial.println("南:" + String(score[1]));
  Serial.println("西:" + String(score[2]));
  Serial.println("北:" + String(score[3]));
  Serial.println("-------");
  delay(1000);
}

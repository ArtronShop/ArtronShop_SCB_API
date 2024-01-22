#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArtronShop_SCB_API.h>

#define API_KEY    "<API Key>"
#define API_SECRET "<API Secret>"

WiFiMulti wifiMulti;
ArtronShop_SCB_API SCB_API(API_KEY, API_SECRET);

String randomString() {
  uint8_t buff[8];
  esp_fill_random(buff, sizeof(buff));
  String str = "";
  for (int i=0;i<sizeof(buff);i++) {
    str += String(buff[i], HEX);
  }
  str.toUpperCase();
  return str;
}

void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("<WiFi SSID>", "<WiFi Password>");

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((wifiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
  }
  Serial.println(" connected");

  if (!SCB_API.setClock()) { // Sync time for verify work
    Serial.println("Sync time fail");
  }

  String ref1 = randomString();
  String ref2 = randomString();
  Serial.println("Ref1:" + ref1);
  Serial.println("Ref2:" + ref2);

  String qrRawData;
  if (SCB_API.QRCodeTag30Create("<Biller ID>", 20, ref1, ref2, "GOOREF3", &qrRawData)) {
    Serial.print("QRCode Tag 30 : ");
    Serial.println(qrRawData);
  } else {
    Serial.println("QRCode Tag 30 create fail");
  }
}

void loop() {
  delay(5000);
  bool confirm = false;
  if (SCB_API.checkPaymentConfirm(&confirm)) {
    if (confirm) {
      Serial.println("Payment confirm !!!");
      while(1) delay(100);
    } else {
      Serial.println("Payment not confirm");
    }
  } else {
    Serial.println("Check Payment Confirm fail");
  }
}

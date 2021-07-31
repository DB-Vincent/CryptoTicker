#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "wifi"
#define STAPSK  "1234"
#endif

#define STASSID "ViAnGe-IoT"
#define STAPSK  "1234509876234"

#define UPDATE_INTERVAL 15 // in seconds
#define CAROUSEL_INTERVAL 5 // in seconds

const uint8_t fingerprint[20] = {0x89, 0x25, 0x60, 0x5d, 0x50, 0x44, 0xfc, 0xc0, 0x85, 0x2b, 0x98, 0xd7, 0xd3, 0x66, 0x52, 0x28, 0x68, 0x4d, 0xe6, 0xe2};

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

struct coinStat {
  long price;
  double dayChange;
  unsigned long updatedAt;
};

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  return true;
}

coinStat getCointStats(String coin) {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setFingerprint(fingerprint);
  HTTPClient https;

  if (https.begin(*client, "https://api.coingecko.com/api/v3/simple/price?ids=" + coin + "&vs_currencies=eur&include_24hr_change=true&include_last_updated_at=true")) {
    int httpCode = https.GET();
  
    if (httpCode > 0) {
      String payload = https.getString();
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      https.end();

      long price = doc[coin]["eur"];
      double dayChange = doc[coin]["eur_24h_change"];
      unsigned long updatedAt = doc[coin]["last_updated_at"];

      coinStat curCoinStat = {price, dayChange, updatedAt};

      return curCoinStat;
    }
  }
}

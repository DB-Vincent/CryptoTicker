#include <TimeLib.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>

#include "icons.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#ifndef STASSID
#define STASSID "wifi"
#define STAPSK  "12345"
#endif

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

#define BTN 14 // Button used for menu
long buttonTimer = 0;
const long longPressTime = 500;     // Long press duration
const long debounceTime = 25;       // Debounce for smoothening out the button response
long lastDebounceTime = 0;

boolean buttonActive = false;
boolean longPressActive = false;

#define UPDATE_INTERVAL 15 // in seconds
#define CAROUSEL_INTERVAL 20 // in seconds

#define AMOUNT_OF_COINS 2

struct coinStruct {
  char name[16];
  long price;
  double dayChange;
  unsigned long updatedAt;
  double chartData[25];
  unsigned long chartUpdatedAt;
};

coinStruct coinList[AMOUNT_OF_COINS] = {
  {"bitcoin", 0, 0, 0, 0, 0},  
  {"ethereum", 0, 0, 0, 0, 0}
};

int curCoin = 0;
long lastStatUpdate = 0;
long lastChartUpdate = 0;
long carouselUpdate = 0;

const uint8_t fingerprint[20] = {0x33, 0xc5, 0x7b, 0x69, 0xe6, 0x3b, 0x76, 0x5c, 0x39, 0x3d, 0xf1, 0x19, 0x3b, 0x17, 0x68, 0xb8, 0x1b, 0x0a, 0x1f, 0xd9};

struct coinStat {
  long price;
  double dayChange;
  unsigned long updatedAt;
};

bool displayGraph = false;

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  return true;
}

double getMax(double inputArray[]) {
  double maxValue = 0;

  for (int i = 0; i < 25; i++) {
    if (inputArray[i] > maxValue) maxValue = inputArray[i];
  }

  return maxValue;
}

double getMin(double inputArray[]) {
  double minValue = 0;

  for (int i = 0; i < 25; i++) {
    if (inputArray[i] < minValue || minValue == 0) minValue = inputArray[i];
  }

  return minValue;
}

coinStat getCoinStats(String coin) {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setFingerprint(fingerprint);
  HTTPClient https;

  if (https.begin(*client, "https://api.coingecko.com/api/v3/simple/price?ids=" + coin + "&vs_currencies=eur&include_24hr_change=true&include_last_updated_at=true")) {
    int httpCode = https.GET();
  
    if (httpCode > 0) {
      String payload = https.getString();
      StaticJsonDocument<100> doc;
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

void updateStats() {
  if ((millis() - lastStatUpdate > (UPDATE_INTERVAL * 1000)) || lastStatUpdate == 0) {
    for (int i = 0; i < AMOUNT_OF_COINS; i++) {
      coinStat curCoinStat = getCoinStats(coinList[i].name);
      coinList[i].price = curCoinStat.price;
      coinList[i].dayChange = curCoinStat.dayChange;
      coinList[i].updatedAt = curCoinStat.updatedAt;
    }

    lastStatUpdate = millis();
  }
}

void displayStats() {
  display.clearDisplay();
  displayIcon(coinList[curCoin].name);
  display.setCursor(35, 10);
  display.println("Price: " + String(coinList[curCoin].price) + "EUR");
  display.setCursor(35, 20);
  display.println("24h: " + String(coinList[curCoin].dayChange) + "%");
  display.display();
}

void getCoinChartData() {
  if ((millis() - coinList[curCoin].chartUpdatedAt > (3600 * 1000)) || coinList[curCoin].chartUpdatedAt == 0) {
    String coin = coinList[curCoin].name;
    
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(fingerprint);
    HTTPClient https;
  
    if (https.begin(*client, "https://api.coingecko.com/api/v3/coins/" + coin + "/market_chart?vs_currency=eur&days=1&interval=hourly")) {
      int httpCode = https.GET();
    
      if (httpCode > 0) {
        String payload = https.getString();
        int cutFrom = payload.indexOf(",\"market_caps\"");
  
        String fixedPayload = payload.substring(0,cutFrom);
        fixedPayload += "}";
        
        DynamicJsonDocument doc(1536);
        DeserializationError error = deserializeJson(doc, fixedPayload);
  
        https.end();
  
        JsonArray prices = doc["prices"];
  
        for (int i = 0; i < 25; i++) {
          coinList[curCoin].chartData[i] = prices[i][1];
        }
      }
    }

    coinList[curCoin].chartUpdatedAt = millis();
  }
}

void displayChartData() {
  double minValue = getMin(coinList[curCoin].chartData);
  double maxValue = getMax(coinList[curCoin].chartData);

  int x0 = 0;
  int x1 = 0;
  int y0 = 0;
  int y1 = 0;

  display.clearDisplay();  
  for (int i = 0; i < 25; i++) {
    double temp = 32 - (((coinList[curCoin].chartData[i] - minValue) / (maxValue - minValue)) * 32);

    if (i == 0) {
      x0 = 0;
      x1 = 0;
      y0 = (int)temp;
      y1 = (int)temp;
    } else {
      x0 = x1;
      y0 = y1;
      x1 += 5;
      y1 = (int)temp;
    }

    display.drawCircle(x1, y1, 1, SSD1306_WHITE);
    display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);

    x0 = x1;
    y0 = (int)temp;
  }
  display.display();
}

void displayIcon(String coin) {
  if (coin == "bitcoin") {
    display.drawBitmap(0, 8, bitcoin, 20, 20, 1);
  } else if (coin == "ethereum") {
    display.drawBitmap(4, 8, ethereum, 13, 20, 1);
  }
}

void nextCoin() {
  int maxCoins = AMOUNT_OF_COINS - 1;

  if (curCoin == maxCoins) {
    curCoin = 0;
  } else {
    curCoin = curCoin + 1;
  }

  carouselUpdate = millis();
}

void setup() {
  Serial.begin(9600);

  pinMode(BTN, INPUT_PULLUP);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(500);
  display.clearDisplay();
  
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Connecting to WiFi: " + String(STASSID) + "...");
  display.display();
  
  if (connectWifi()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connected!");
    display.setCursor(0, 10);
    display.println(WiFi.localIP());
    display.display();
  } else {
    for(;;); // Don't proceed, loop forever
  }

  delay(500);
}

void loop() {  
  if ((millis() - lastDebounceTime) > debounceTime) {
    if (digitalRead(BTN) == LOW) { // Button pressed -> LOW
      if (buttonActive == false) {
        buttonActive = true;
        buttonTimer = millis();
      }
      
      if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) { // Long press
        longPressActive = true;

        if (displayGraph == false) displayGraph = true;
        else if (displayGraph == true) displayGraph = false;
      }

      lastDebounceTime = millis();
    } else {
      if (buttonActive == true) {
        if (longPressActive == true) {
          longPressActive = false;
        } else { // Short press
          nextCoin();
        }
        
        buttonActive = false;
      }

      lastDebounceTime = millis();
    }
  }

  if ((millis() - carouselUpdate) > (CAROUSEL_INTERVAL * 1000)) {
    nextCoin();
    carouselUpdate = millis();
  } else if (carouselUpdate == 0) {
    carouselUpdate = millis();
  }

  if (displayGraph == true) {
    getCoinChartData();
    displayChartData(); 
  } else {
    updateStats();
    displayStats(); 
  }
}

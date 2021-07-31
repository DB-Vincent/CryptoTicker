
#include <TimeLib.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "utils.h"
#include "icons.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#ifndef STASSID
#define STASSID "wifi"
#define STAPSK  "1234"
#endif

#define BTN 14 // Button used for menu
long buttonTimer = 0;
const long longPressTime = 500;     // Long press duration
const long debounceTime = 25;       // Debounce for smoothening out the button response
long lastDebounceTime = 0;

boolean buttonActive = false;
boolean longPressActive = false;

#define AMOUNT_OF_COINS 2

struct coinStruct {
  char name[16];
  long price;
  double dayChange;
  unsigned long updatedAt;
};

coinStruct coinList[AMOUNT_OF_COINS] = {
  {"bitcoin", 0, 0, 0},  
  {"ethereum", 0, 0, 0}
};

int curCoin = 0;
long lastUpdate = 0;
long carouselUpdate = 0;

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
  
  updateStats();
  displayStats();
}

void updateStats() {
  if ((millis() - lastUpdate > (UPDATE_INTERVAL * 1000)) || lastUpdate == 0) {
    for (int i = 0; i < AMOUNT_OF_COINS; i++) {
      coinStat curCoinStat = getCointStats(coinList[i].name);
      coinList[i].price = curCoinStat.price;
      coinList[i].dayChange = curCoinStat.dayChange;
      coinList[i].updatedAt = curCoinStat.updatedAt;
    }

    lastUpdate = millis();
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

void nextCoin() {
  int maxCoins = AMOUNT_OF_COINS - 1;

  if (curCoin == maxCoins) {
    curCoin = 0;
  } else {
    curCoin = curCoin + 1;
  }

  carouselUpdate = millis();
}

void displayIcon(String coin) {
  if (coin == "bitcoin") {
    display.drawBitmap(0, 8, bitcoin, 20, 20, 1);
  } else if (coin == "ethereum") {
    display.drawBitmap(4, 8, ethereum, 13, 20, 1);
  }
}

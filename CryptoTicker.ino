#include "icons.h"

#define BTN 14 // Button used for menu
#define CAROUSEL_INTERVAL 20 // in seconds

long buttonTimer = 0;
const long longPressTime = 250;     // Long press duration
const long debounceTime = 25;       // Debounce for smoothening out the button response
long lastDebounceTime = 0;
boolean buttonActive = false;
boolean longPressActive = false;
boolean displayGraph = false;
long carouselUpdate = 0;

struct coinStat {
  double price;
  double dayChange;
  unsigned long updatedAt;
};

struct coinStruct {
  char name[16];
  double price;
  double dayChange;
  unsigned long updatedAt;
  double chartData[25];
};


void setup() {
  Serial.begin(9600);

  pinMode(BTN, INPUT_PULLUP);
  
  initializeDisplay();
  connectWifi();
  setupOTA();

  delay(500);
}

void loop() {  
  handleOTA();
  
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
    updateCharts();
    displayChartData(); 
  } else {
    updateStats();
    displayStats(); 
  }
}

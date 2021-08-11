/*
 * WiFi
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

ESP8266WebServer server(80);


/*
 * Display 
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/*
 * ArduinoJson
 */
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>


/*
 * EEPROM
 */
#include <EEPROM.h>


/*
 * Variables
 */
#define UPDATE_INTERVAL 15 // in seconds
#define AMOUNT_OF_COINS 3

coinStruct coinList[AMOUNT_OF_COINS] = {
  {"bitcoin", 0, 0, 0, 0},  
  {"ethereum", 0, 0, 0, 0},
  {"cardano", 0, 0, 0, 0},
};

int curCoin = 0;
long lastStatUpdate = 0;
long lastChartUpdate = 0;

const uint8_t fingerprint[20] = {0x33, 0xc5, 0x7b, 0x69, 0xe6, 0x3b, 0x76, 0x5c, 0x39, 0x3d, 0xf1, 0x19, 0x3b, 0x17, 0x68, 0xb8, 0x1b, 0x0a, 0x1f, 0xd9};

// Variables for wifi setup
int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;

/*
 * Initialize display
 * Display logo and set font settings
 */
void initializeDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.display();
  delay(500);
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

/*
 * Test WiFi connection
 * If no connection can be made, return false.
 * Used for initial setup
 */
bool testWifi() {
  int c = 0;
  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true; // Wifi configured + connected
    }
    
    delay(500);
    c++;
  }
  
  return false; // No WiFi configured
}


/*
 * Launch webserver
 * Used for configuring WiFi
 */
void launchWeb() {
  if (WiFi.status() == WL_CONNECTED) { Serial.println("WiFi connected"); }
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
}


/*
 * Setup Access Point
 * Used for initial setup
 */
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.softAP("CryptoTicker");
  launchWeb();
}


/*
 * Webserver creation
 * Handles all webserver interactions
 */
void createWebServer() {
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>Hello from CryptoTicker at ";
      content += ipStr;
      content += "<form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32></br><label>Pass: </label><input name='pass' length=64><br><input type='submit'></form>";
      content += "</body></html>";
      server.send(200, "text/html", content);
    });
    
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");

      if (qsid.length() > 0 && qpass.length() > 0) {
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
 
        for (int i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
        }
        
        for (int i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
        }
        EEPROM.commit();
 
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }

      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  } 
}


/*
 * Connect to WiFi
 * If no WiFi is configured, show initial configuration
 * If WiFi is configured, connect to WiFi.
 */
void connectWifi() {
  EEPROM.begin(512);

  WiFi.disconnect();

  String esid;
  for (int i = 0; i < 32; ++i) {
    esid += char(EEPROM.read(i));
  }

  String epass = "";
  for (int i = 32; i < 96; ++i) {
    epass += char(EEPROM.read(i));
  }

  WiFi.begin(esid.c_str(), epass.c_str());
  display.clearDisplay();  
  if (testWifi()) {
    String ssid = esid.c_str();
    WiFi.softAPdisconnect(true);
    display.println("Succesfully connected to " + ssid);
    display.display();
    return;
  } else {
    IPAddress ip = WiFi.softAPIP();
    display.println("Initial config");
    display.println("WiFi: CryptoTicker");
    display.println("Visit: http://192.168.4.1");
    display.display();
    launchWeb();
    setupAP();// Setup HotSpot
  }
  

  while ((WiFi.status() != WL_CONNECTED)) {
    server.handleClient();
    delay(100);
  }
}


/*
 * Set up OTA (Over The Air) updates
 */
void setupOTA() {
  ArduinoOTA.setHostname("cryptoticker");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}


/*
 * Handle OTA updates
 */
void handleOTA() {
  ArduinoOTA.handle();
}


/*
 * Helper function to get the highest value of an array
 */
double getMax(double inputArray[]) {
  double maxValue = 0;

  for (int i = 0; i < 25; i++) {
    if (inputArray[i] > maxValue) maxValue = inputArray[i];
  }

  return maxValue;
}


/*
 * Helper function to get the lowest value of an array
 */
double getMin(double inputArray[]) {
  double minValue = 0;

  for (int i = 0; i < 25; i++) {
    if (inputArray[i] < minValue || minValue == 0) minValue = inputArray[i];
  }

  return minValue;
}


/*
 * Small helper function for printing a string in the center of the screen
 */
void printCenterString(const String &buf, int x, int y) {
  int16_t  x1, y1;
  uint16_t w, h;

  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);
  display.setCursor((x - w) / 2, y);
  display.print(buf);
}


/*
 * Retrieve pricing of specific coin from Coingecko API
 */
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

      double price = doc[coin]["eur"];
      double dayChange = doc[coin]["eur_24h_change"];
      unsigned long updatedAt = doc[coin]["last_updated_at"];

      coinStat curCoinStat = {price, dayChange, updatedAt};
      
      return curCoinStat;
    }
  }
}


/*
 * Used for updating pricing stats every x seconds (UPDATE_INTERVAL)
 */
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


/*
 * Show pricing and 24h change of current coin.
 */
void displayStats() {
  display.clearDisplay();
  displayIcon(coinList[curCoin].name);
  display.setFont(&FreeSans9pt7b);
  printCenterString((String)coinList[curCoin].price + "EUR", 128, 52);
  display.setFont();
  printCenterString((String)coinList[curCoin].dayChange + "%", 128, 55);
  display.display();
}


/*
 * Get chart data for the current coin
 */
void getCoinChartData(String coin) {
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

      int item = 0;
      for (int i = 0; i < AMOUNT_OF_COINS; i++) {
        if ((String)coinList[i].name == coin) item = i;
      }

      for (int i = 0; i < 25; i++) {
        coinList[item].chartData[i] = prices[i][1];
      }
    }
  }
}


/*
 * Used for updating the chart every hour
 */
void updateCharts() {
  if ((millis() - lastChartUpdate > (3600 * 1000)) || lastChartUpdate == 0) {
    for (int i = 0; i < AMOUNT_OF_COINS; i++) {
      getCoinChartData(coinList[i].name);
    }

    lastChartUpdate = millis();
  }
}

/*
 * Show chart for current coin
 */
void displayChartData() {
  double minValue = getMin(coinList[curCoin].chartData);
  double maxValue = getMax(coinList[curCoin].chartData);

  int x0 = 0;
  int x1 = 0;
  int y0 = 0;
  int y1 = 0;

  display.clearDisplay(); 
  displayIcon(coinList[curCoin].name); 
  for (int i = 0; i < 25; i++) {
    double temp = (30 - (((coinList[curCoin].chartData[i] - minValue) / (maxValue - minValue)) * 30)) + 33;

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


/*
 * Displays the icon of a specific coin (using icons.h)
 */
void displayIcon(String coin) {
  if (coin == "bitcoin") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8,  bitcoin, 20, 20, 1);
  } else if (coin == "ethereum") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, ethereum, 13, 20, 1);
  } else if (coin == "cardano") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, cardano, 22, 20, 1);
  }
}


/*
 * Goes to next coin, used for both the short button press and the carousel.
 */
void nextCoin() {
  int maxCoins = AMOUNT_OF_COINS - 1;

  if (curCoin == maxCoins) {
    curCoin = 0;
  } else {
    curCoin = curCoin + 1;
  }

  carouselUpdate = millis();
}

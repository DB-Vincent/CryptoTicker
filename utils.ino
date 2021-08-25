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

coinStruct coinList[10] = {
  {"bitcoin", 0, 0, 0, 0},  
  {"ethereum", 0, 0, 0, 0},
  {"tether", 0, 0, 0, 0},
  {"cardano", 0, 0, 0, 0},
  {"ripple", 0, 0, 0, 0},
  {"dogecoin", 0, 0, 0, 0},  
  {"polkadot", 0, 0, 0, 0},
  {"usd-coin", 0, 0, 0, 0},
  {"bitcoin-cash", 0, 0, 0, 0},
  {"matic-network", 0, 0, 0, 0},
};

coinStruct configuredCoinList[5];

int amountOfCoins = 0;
String configuredCoins[5];

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

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}


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
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>CryptoTicker initial configuration";
      content += "<form method='get' action='setting'><h1>WiFi</h1><br><label>SSID: </label><input name='ssid' length=32></br><label>Pass: </label><input name='pass' length=64><br><h1>Cryptocurrencies</h1>";
      for (int i = 0; i < 10; i++) {
        content += "<br><input type='checkbox' id='" + coinList[i].name + "' name='" + coinList[i].name + "' value='" + coinList[i].name + "'><label for='" + coinList[i].name + "'>" + coinList[i].name + "</label>";
      }
      content += "<br><input type='submit'></form></body></html>";
      server.send(200, "text/html", content);
    });
    
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");

      if (qsid.length() > 0 && qpass.length() > 0 && (server.hasArg("bitcoin") || server.hasArg("ethereum") || server.hasArg("tether") || server.hasArg("cardano") || server.hasArg("ripple") || server.hasArg("dogecoin") || server.hasArg("polkadot") || server.hasArg("usd-coin") || server.hasArg("bitocin-cash") || server.hasArg("matic-network"))) {
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
 
        for (int i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
        }
        
        for (int i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
        }

        if (server.hasArg("bitcoin")) {
          configuredCoins[amountOfCoins] = "bitcoin";
          amountOfCoins++;
        }

        if (server.hasArg("ethereum")) {
          configuredCoins[amountOfCoins] = "ethereum";
          amountOfCoins++;
        }

        if (server.hasArg("tether")) {
          configuredCoins[amountOfCoins] = "tether";
          amountOfCoins++;
        }

        if (server.hasArg("cardano")) {
          configuredCoins[amountOfCoins] = "cardano";
          amountOfCoins++;
        }

        if (server.hasArg("ripple")) {
          configuredCoins[amountOfCoins] = "ripple";
          amountOfCoins++;
        }

        if (server.hasArg("dogecoin")) {
          configuredCoins[amountOfCoins] = "dogecoin";
          amountOfCoins++;
        }

        if (server.hasArg("polkadot")) {
          configuredCoins[amountOfCoins] = "polkadot";
          amountOfCoins++;
        }

        if (server.hasArg("usd-coin")) {
          configuredCoins[amountOfCoins] = "usd-coin";
          amountOfCoins++;
        }

        if (server.hasArg("bitcoin-cash")) {
          configuredCoins[amountOfCoins] = "bitcoin-cash";
          amountOfCoins++;
        }

        if (server.hasArg("matic-network")) {
          configuredCoins[amountOfCoins] = "matic-network";
          amountOfCoins++;
        }

        if (sizeof(configuredCoins) > 3) {
          content = "{\"Error\":\"Only maximum 3 coins currently supported.\"}";
          statusCode = 500;
          Serial.println("Sending 404");
        }

        EEPROM_writeAnything(100, amountOfCoins);
        EEPROM_writeAnything(116, configuredCoins);
        
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
void initialConfig() {
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
  
    EEPROM_readAnything(100, amountOfCoins);
    EEPROM_readAnything(116, configuredCoins);

    Serial.println(configuredCoins[2]);

    for (int i = 0; i < amountOfCoins; i++) {
      for (int j = 0; j < 10; j++) {
        if (coinList[j].name == configuredCoins[i]) {
          configuredCoinList[i] = coinList[j];
        }
      }
    }

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
    for (int i = 0; i < amountOfCoins; i++) {
      coinStat curCoinStat = getCoinStats(configuredCoinList[i].name);
      configuredCoinList[i].price = curCoinStat.price;
      configuredCoinList[i].dayChange = curCoinStat.dayChange;
      configuredCoinList[i].updatedAt = curCoinStat.updatedAt;
    }

    lastStatUpdate = millis();
  }
}


/*
 * Show pricing and 24h change of current coin.
 */
void displayStats() {
  display.clearDisplay();
  displayIcon(configuredCoinList[curCoin].name);
  display.setFont(&FreeSans9pt7b);
  printCenterString((String)configuredCoinList[curCoin].price + "EUR", 128, 52);
  display.setFont();
  printCenterString((String)configuredCoinList[curCoin].dayChange + "%", 128, 55);
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
      for (int i = 0; i < amountOfCoins; i++) {
        if ((String)configuredCoinList[i].name == coin) item = i;
      }

      for (int i = 0; i < 25; i++) {
        configuredCoinList[item].chartData[i] = prices[i][1];
      }
    }
  }
}


/*
 * Used for updating the chart every hour
 */
void updateCharts() {
  if ((millis() - lastChartUpdate > (3600 * 1000)) || lastChartUpdate == 0) {
    for (int i = 0; i < amountOfCoins; i++) {
      getCoinChartData(configuredCoinList[i].name);
    }

    lastChartUpdate = millis();
  }
}

/*
 * Show chart for current coin
 */
void displayChartData() {
  double minValue = getMin(configuredCoinList[curCoin].chartData);
  double maxValue = getMax(configuredCoinList[curCoin].chartData);

  int x0 = 0;
  int x1 = 0;
  int y0 = 0;
  int y1 = 0;

  display.clearDisplay(); 
  displayIcon(configuredCoinList[curCoin].name); 
  for (int i = 0; i < 25; i++) {
    double temp = (30 - (((configuredCoinList[curCoin].chartData[i] - minValue) / (maxValue - minValue)) * 30)) + 33;

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
  } else if (coin == "tether") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, tether, 23, 20, 1);
  } else if (coin == "cardano") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, cardano, 22, 20, 1);
  } else if (coin == "ripple") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, ripple, 24, 20, 1);
  } else if (coin == "dogecoin") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, doge, 20, 20, 1);
  } else if (coin == "polkadot") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, polkadot, 15, 20, 1);
  } else if (coin == "usd-coin") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, usdcoin, 20, 20, 1);
  } else if (coin == "bitcoin-cash") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, bitcoinCash, 32, 20, 1);
  } else if (coin == "matic-network") {
    display.drawBitmap((display.width()  - 20 ) / 2, 8, matic, 23, 20, 1);
  }
}


/*
 * Goes to next coin, used for both the short button press and the carousel.
 */
void nextCoin() {
  int maxCoins = amountOfCoins - 1;

  if (curCoin == maxCoins) {
    curCoin = 0;
  } else {
    curCoin = curCoin + 1;
  }

  carouselUpdate = millis();
}

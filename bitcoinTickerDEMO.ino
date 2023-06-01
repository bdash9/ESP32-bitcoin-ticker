//#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <EasyNTPClient.h> 
#include <WiFiUdp.h>
//#include "secrets.h" // WiFi Configuration (WiFi name and Password)
#include <ArduinoJson.h>
//#define SCREEN_WIDTH 128 // OLED tft width, in pixels
//#define SCREEN_HEIGHT 64 // OLED tft height, in pixels
//#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//Adafruit_SSD1306 tft(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

const char ssid[] = "AP_NAME";
const char password[] = "password";

const int httpsPort = 443;
// Powered by CoinDesk - https://www.coindesk.com/price/bitcoin
const String url = "http://api.coindesk.com/v1/bpi/currentprice/BTC.json";
const String historyURL = "http://api.coindesk.com/v1/bpi/historical/close.json";
const String cryptoCode = "BTC";

WiFiClient client;
HTTPClient http;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

void setup() {
  Serial.begin(115200);

  delay(300);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor (ST77XX_RED);
  tft.print ("DASH"); 
  tft.setTextColor (ST77XX_YELLOW);
  tft.print ("9");    
  tft.setTextColor (ST77XX_BLUE);
  tft.print ("COMPUTING");
  tft.setTextColor(ST77XX_WHITE);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);
  tft.print("\nConnecting to WiFi: ");
  tft.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    tft.print(".");    
    delay(500);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  tft.println("\nWiFi connected!");
  tft.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  tft.println(ip);
  delay(1500);
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  Serial.print("Connecting to ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  StaticJsonDocument<2000> doc;
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error) {
    Serial.print(F("deserializeJson Failed"));
    Serial.println(error.f_str());
    delay(2500);
    return;
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String BTCUSDPrice = doc["bpi"]["USD"]["rate_float"].as<String>();
  String lastUpdated = doc["time"]["updated"].as<String>();
  http.end();

  Serial.print("Getting history...");
  StaticJsonDocument<2000> historyDoc;
  http.begin(historyURL);
  int historyHttpCode = http.GET();
  DeserializationError historyError = deserializeJson(historyDoc, http.getString());

  if (historyError) {
    Serial.print(F("deserializeJson(History) failed"));
    Serial.println(historyError.f_str());
    delay(2500);
    return;
  }

  Serial.print("History HTTP Status Code: ");
  Serial.println(historyHttpCode);
  JsonObject bpi = historyDoc["bpi"].as<JsonObject>();
  double yesterdayPrice;
  for (JsonPair kv : bpi) {
    yesterdayPrice = kv.value().as<double>();
  }

  Serial.print("BTCUSD Price: ");
  Serial.println(BTCUSDPrice.toDouble());

  Serial.print("Yesterday's Price: ");
  Serial.println(yesterdayPrice);

  bool isUp = BTCUSDPrice.toDouble() > yesterdayPrice;
  double percentChange;
  if (isUp) {
    percentChange = ((BTCUSDPrice.toDouble() - yesterdayPrice) / yesterdayPrice) * 100;
  } else {
    percentChange = ((yesterdayPrice - BTCUSDPrice.toDouble()) / yesterdayPrice) * 100;
  }

  Serial.print("Percent Change: ");
  Serial.println(percentChange);

  //tft Header
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.setTextSize(3);
  tft.setTextColor (ST77XX_YELLOW);
  //printCenter("BTC/USD", 0, 0);
  tft.println("BTC - Bitcoin");
  tft.println();

  //tft BTC Price
  tft.setTextSize(3);
  tft.setTextColor (ST77XX_RED);
  tft.print("U");
  tft.setTextColor (ST77XX_WHITE);
  tft.print("S");
  tft.setTextColor (ST77XX_BLUE);
  tft.print("D");
  tft.setTextColor (ST77XX_WHITE);
  tft.println(":");
  //printCenter("$" + BTCUSDPrice, 0, 25);
  tft.setTextColor (ST77XX_GREEN);
  tft.println ("$" + BTCUSDPrice);
  tft.println();

  //tft 24hr. Percent Change
  // Doesn't work...http://api.coindesk.com/v1/bpi/currentprice/BTC.json doesn't exist
  /*
  tft.setTextSize(3);
  tft.setTextColor (ST77XX_WHITE);
  String dayChangeString = "24hr. Change: ";
  if (isUp) {
    percentChange = ((BTCUSDPrice.toDouble() - yesterdayPrice) / yesterdayPrice) * 100;
  } else {
    percentChange = ((yesterdayPrice - BTCUSDPrice.toDouble()) / yesterdayPrice) * 100;
    dayChangeString = dayChangeString + "-";
  }
  tft.setTextSize(2);
  dayChangeString = dayChangeString + percentChange + "%";
  //printCenter(dayChangeString, 0, 55);
  tft.println(dayChangeString);
  tft.println();
*/
  http.end();
  delay(15000);
}

void printCenter(const String buf, int x, int y)
{
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
  tft.setCursor((x - w / 2) + (128 / 2), y);
  tft.print(buf);
}

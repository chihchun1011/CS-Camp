#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>  // required even though we do not use I2C 
#include <XPT2046_Touchscreen.h>

// Additional UI functions
#include "GfxUi.h"

// Fonts created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"

// Download helper
#include "WebResource.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// Helps with connecting to internet
#include <WiFiManager.h>

// check settings.h for adapting to your needs
#include "settings.h"
#include <JsonListener.h>
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "TimeClient.h"

#include "DHTesp.h"
#define DHTPIN D0     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 
DHTesp dht;

#define TOUCH_CS_PIN D3
#define TOUCH_IRQ_PIN D2

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 320
#define TS_MINY 250
#define TS_MAXX 3800
#define TS_MAXY 3900

XPT2046_Touchscreen spitouch(TOUCH_CS_PIN,TOUCH_IRQ_PIN);

// HOSTNAME for OTA update
#define HOSTNAME "ESP8266-OTA-"

/*****************************
 * Important: see settings.h to configure your settings!!!
 * ***************************/

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
GfxUi ui = GfxUi(&tft);

WebResource webResource;
TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
OpenWeatherMapCurrent OWMCclient;
OpenWeatherMapCurrentData OWMCdata;
OpenWeatherMapForecast OWMFclient;
#define MAX_FORECASTS 15
OpenWeatherMapForecastData OWMFdata[MAX_FORECASTS];
uint8_t allowedHours[] = {0,12};
 
//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
String getMeteoconIcon(String iconText);
void drawAstronomy();
void drawSeparator(uint16_t y);
void sleepNow(uint8_t wakepin);

long lastDownloadUpdate = millis();

//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
  
void setup() {
  dht.setup(DHTPIN, DHTesp::DHTTYPE); // Connect DHT sensor 
  Serial.begin(115200);
  pinMode(BL_LED,OUTPUT); //BackLight
  if (! spitouch.begin()) {
    Serial.println("TouchSensor not found?");
  }

  // if using deep sleep, we can use the TOUCH's IRQ pin to wake us up
  if (DEEP_SLEEP) {
    pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP); 
  }
  
  digitalWrite(BL_LED,HIGH); // backlight on

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(CENTER);
  ui.drawString(160, 120, "Connecting to WiFi");
  
  // Uncomment for testing wifi manager, ony once
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();

  //Manual Wifi
  //WiFi.begin("Kuo's iPhone", "b06901023");

  // OTA Setup
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  SPIFFS.begin();
  
  //Uncomment if you want to update all internet resources
  //SPIFFS.format();

  // download images from the net. If images already exist don't download
  downloadResources();

  // load the weather information
  updateData();
}

long lastDrew = 0;
void loop() {
  if (USE_TOUCHSCREEN_WAKE) {     // determine in settings.h!
    
    // for AWAKE_TIME seconds we'll hang out and wait for OTA updates
    for (uint16_t i=0; i<AWAKE_TIME; i++  ) {
      // Handle OTA update requests
      ArduinoOTA.handle();
      delay(10000);
      yield();
    }

    // turn off the display and wait for a touch!
    // flush the touch buffer
//    while (spitouch.bufferSize() != 0) {
//      spitouch.getPoint();
//      Serial.print('.');
//      yield();
//    }
    
    Serial.println("Zzzz");
    digitalWrite(BL_LED,LOW);// backlight off  

    if (DEEP_SLEEP) {
      sleepNow(TOUCH_IRQ_PIN);
    } else {
      while (! spitouch.touched()) {
        // twiddle thumbs
        delay(10);
      }
    }
    Serial.println("Touch detected!");
  
    // wipe screen & backlight on
    tft.fillScreen(ILI9341_BLACK);
    digitalWrite(BL_LED,HIGH); // backlight on
    updateData();
  } 
  else // "standard setup"
  {
    // Handle OTA update requests
    ArduinoOTA.handle();

    // Check if we should update the clock
    if (millis() - lastDrew > 30000 /*&& wunderground.getSeconds() == "00"*/) {
      drawTime();
      lastDrew = millis();
    }

    // Check if we should update weather information
    if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
      updateData();
      lastDownloadUpdate = millis();
    }
  }
}

// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
  ui.setTextAlignment(CENTER);
  tft.setFont(&ArialRoundedMTBold_14);
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(160, 28, "Wifi Manager");
  ui.drawString(160, 44, "Please connect to AP");
  tft.setTextColor(ILI9341_WHITE);
  ui.drawString(160, 60, myWiFiManager->getConfigPortalSSID());
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(160, 76, "To setup Wifi Configuration");
}

// callback called during download of files. Updates progress bar
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal) {
  Serial.println(String(bytesDownloaded) + " / " + String(bytesTotal));

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    ui.drawString(160, 112, filename);
  }
  if (percentage % 5 == 0) {
    ui.setTextAlignment(CENTER);
    ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
    //ui.drawString(160, 125, String(percentage) + "%");
    ui.drawProgressBar(10, 125, 320 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
  }

}

// Download the bitmaps
void downloadResources() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  char id[5];
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp", wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp", "/mini/" + wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 23; i++) {
    tft.fillRect(0, 100, 320, 18, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }
}

// Update the internet based information and update screen
void updateData() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  drawProgress(20, "Updating time...");
  timeClient.updateTime();

  drawProgress(60, "Updating conditions...");
  OWMCclient.setLanguage(OPENWEATHERMAP_LANGUAGE);
  OWMCclient.setMetric(IS_METRIC);
 
  drawProgress(80, "Updating conditions...");
  OWMCclient.updateCurrent(&OWMCdata, OPENWEATHERMAP_APP_ID, OPENWEATHERMAP_CITY+","+OPENWEATHERMAP_COUNTRY);

  OWMFclient.setMetric(IS_METRIC);
  OWMFclient.setLanguage(OPENWEATHERMAP_LANGUAGE);
  OWMFclient.setAllowedHours(allowedHours, 2);
  uint8_t foundForecasts = OWMFclient.updateForecasts(OWMFdata, OPENWEATHERMAP_APP_ID, OPENWEATHERMAP_CITY+","+OPENWEATHERMAP_COUNTRY, MAX_FORECASTS);

  //lastUpdate = timeClient.getFormattedTime();
  //readyForWeatherUpdate = false;
  drawProgress(100, "Done...");
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
  drawTime();
  drawCurrentWeather();
  drawForecast();
  drawLocalTH();
  drawAstronomy();
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.fillRect(0, 100, 320, 40, ILI9341_BLACK);
  ui.drawString(160, 112, text);
  ui.drawProgressBar(10, 125, 320 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
}

// draws the clock
void drawTime() {
  time_t Ftime = OWMFdata[0].observationTime;
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  String CFtime = ctime(&Ftime);
  ui.drawString(160, 20, CFtime.substring(0,10)+CFtime.substring(CFtime.length()-6));
  
  tft.setFont(&ArialRoundedMTBold_36);
  String time = timeClient.getHours() + ":" + timeClient.getMinutes();
  ui.drawString(160, 56, time);
  drawSeparator(65);
}

// draws current weather information
void drawCurrentWeather() {
  ui.setTextAlignment(LEFT);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  String city = OWMCdata.cityName;
  ui.drawString(10, 50, city);
  
  // Weather Icon
  String weatherIcon = OWM_to_WU_Icon_Mapping(OWMCdata.icon);
  ui.drawBmp(weatherIcon + ".bmp", 0, 55);
  // Weather Text
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  ui.drawString(220, 90, OWMCdata.main);
  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  if (IS_METRIC) {
    degreeSign = "C";
  }
  String dash = "-";
  String temp = String((int)(OWMCdata.tempMin+0.5)) + dash + String((int)(OWMCdata.tempMax+0.5)) + degreeSign;
  ui.drawString(220, 125, temp);
  drawSeparator(135);

}

// draws the three forecast columns
void drawForecast() {
  drawForecastDetail(5, 165, 2);
  drawForecastDetail(65, 165, 4);
  drawForecastDetail(125, 165, 6);
  drawForecastDetail(185, 165, 8);
  drawSeparator(165 + 65 + 10);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(CENTER);
  time_t Ftime = OWMFdata[dayIndex].observationTime;
  String CFtime = ctime(&Ftime);
  String day = CFtime.substring(4,10);
  //day.toUpperCase();
  ui.drawString(x + 25, y, day);

  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(x + 25, y + 14, String((int)(OWMFdata[dayIndex].tempMin+0.5)) + "|" + String((int)(OWMFdata[dayIndex].tempMax+0.5)));

  String weatherIcon = OWM_to_WU_Icon_Mapping(OWMFdata[dayIndex].icon);
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x, y + 15);
}

void drawLocalTH()
{
  float h = dht.getHumidity();
  float t = dht.getTemperature();

  String degC = "C";
  String percent = "%";
  String temp = String((int)t) + degC;
  String hum = String((int)h) + percent;
  String local ="Local";
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.drawString(280, 150, local);
  tft.setFont(&ArialRoundedMTBold_36);
  ui.drawString(280, 185, temp);
  ui.drawString(280, 225, hum);
  // drawSeparator(300);
}

float JDN(int Y, int M, int D)
{ float jdn = (1461.0 * (Y + 4800.0 + (M - 14.0)/12.0))/4.0 +(367.0 * (M - 2.0 - 12.0 * ((M - 14.0)/12.0)))/12.0 - (3.0 * ((Y + 4900.0 + (M - 14.0)/12.0)/100.0))/4.0 + D - 32075.0;
  Serial.println(jdn);
  return jdn;
/* subtract integer part to leave fractional part of original jd */
}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  struct tm * timeinfo;
  String Moon ="Moon";
  ui.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.drawString(280, 60, Moon);
  time_t Ftime = OWMFdata[0].observationTime;
  timeinfo = localtime(&Ftime);  
  float jdn_months = (JDN(timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday)-JDN(1900, 1, 1)-2)/29.53;
  float moonphase_now = jdn_months - floor(jdn_months);
  int moonAgeImage = (int) (23.9 * moonphase_now);
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 250, 68);
}

String OWM_to_WU_Icon_Mapping(String icon){
  if (icon.substring(0,2) == "01") return "sunny";
  if (icon.substring(0,2) == "02") return "partlycloudy";
  if (icon.substring(0,2) == "03") return "cloudy";
  if (icon.substring(0,2) == "04") return "mostlycloudy";
  if (icon.substring(0,2) == "09") return "chancerain";
  if (icon.substring(0,2) == "10") return "rain";
  if (icon.substring(0,2) == "11") return "tstorms";  
  if (icon.substring(0,2) == "13") return "snow";
  if (icon.substring(0,2) == "50") return "hazy";  
  return "unknown";
}

// if you want separators, uncomment the tft-line
void drawSeparator(uint16_t y) {
 //  tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
}




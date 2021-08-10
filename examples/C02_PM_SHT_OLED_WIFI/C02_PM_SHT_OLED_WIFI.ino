/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The codes needs the following libraries installed:
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" tested with Version 4.1.0

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

MIT License
*/

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

#include <WiFiClientSecureBearSSL.h>

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// set sensors that you do not use to false
boolean hasPM=true;
boolean hasCO2=true;
boolean hasSHT=true;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI=true;

// change if you want to send the data to another server
String APIROOT = "https://wac.fzysqr.com/";

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWIFI) connectToWifi();
  delay(2000);
}

void loop(){

  // create payload
  String tags = "id=" + String(ESP.getChipId(),HEX) + ",env=inside";
  String payload = "wifi," + tags + " rssi=" + String(WiFi.RSSI()) + "\n";

  if (hasPM) {
    // Wake up the sensor and wait 30 seconds before getting data
    ag.wakeUp();
    delay(30000);

    PMS_DATA result = ag.getPMS_Data();
    // Check to see if the PMS read succeeded, otherwise report a failure and skip reporting
    if (!result.success)
    {
      showTextRectangle("PMS Read", "Failure", true);
      payload=payload + "meta," + tags + " pms_read_failure=true\n";
      delay(10000);
    }
    else
    {
      // Get PM data
      int PM1 = result.PM_AE_UG_1_0;
      payload=payload + "air_quality," + tags + " pm01=" + String(PM1) + "\n";

      int PM2 = result.PM_AE_UG_2_5;
      payload=payload + "air_quality," + tags + " pm02=" + String(PM2) + "\n";

      int PM10 = result.PM_AE_UG_10_0;
      payload=payload + "air_quality," + tags + " pm10=" + String(PM10) + "\n";

      // Put sensor to sleep again until next loop
      ag.sleep();
      showTextRectangle("SLEEP", "30s", true);
      delay(2000);

      showTextRectangle("PM1.0",String(PM1),true);
      delay(5000);
      showTextRectangle("PM2.5",String(PM2),true);
      delay(6000);
      showTextRectangle("PM10",String(PM10),true);
      delay(6000);
    }
  }

  if (hasCO2) {
    int CO2 = ag.getCO2_Raw();
    payload=payload + "air_quality," + tags + " rco2=" + String(CO2) + "\n";
    showTextRectangle("CO2",String(CO2),true);
    delay(6000);
  }

  if (hasSHT) {
    TMP_RH result = ag.periodicFetchData();
    payload=payload + "weather," + tags + " atmp=" + String(result.t) + "\n";
    payload=payload + "weather," + tags + " rhum=" + String(result.rh) + "\n";
    showTextRectangle(String(result.t),String(result.rh)+"%",true);
    delay(5000);
  }

  // send payload
  if (connectWIFI){
  Serial.println(payload);
  String POSTURL = APIROOT + "api/v2/write?org=wac&bucket=sensordata&precision=s";
  Serial.println(POSTURL);

  // Hack to not check finger prints on https connection
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient http;
  http.begin(*client, POSTURL);
  http.addHeader("Authorization", "Token 1XKQsyW-UKB5EU94ioSXiJtY-6918O0YewbobFqXzn3To9iUSioidF_EbD3OQqW1MgfsPMYOgrzwKRUW63I3IQ==");
  int httpCode = http.POST(payload);
  String response = http.getString();
  Serial.println(httpCode);
  Serial.println(response);
  http.end();
  }
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }

}

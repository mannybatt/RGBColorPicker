



/**
 * 
 *  This piece of software is designed to recieve the exact RGB values of
 *  a real world set of LED's in order to use those values in other code.
 *  Uses MQTT to send those values.   -Manny Batt
 *  
 */


// ***************************************
// ********** Global Variables ***********
// ***************************************


//Globals for Wifi Setup and OTA
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//WiFi Credentials
#ifndef STASSID
#define STASSID "your_ssid"
#endif
#ifndef STAPSK
#define STAPSK  "your_password"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

//MQTT
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#ifndef AIO_SERVER
#define AIO_SERVER      "your_MQTT_server_address"
#endif
#ifndef AIO_SERVERPORT
#define AIO_SERVERPORT  0000 //Your MQTT port
#endif
#ifndef AIO_USERNAME
#define AIO_USERNAME    "your_MQTT_username"
#endif
#ifndef AIO_KEY
#define AIO_KEY         "your_MQTT_key"
#endif
#define MQTT_KEEP_ALIVE 150
unsigned long previousTime;

//Initialize and Subscribe to MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe cpp_Red = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ColorPickerProgram_Red");
Adafruit_MQTT_Subscribe cpp_Green = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ColorPickerProgram_Green");
Adafruit_MQTT_Subscribe cpp_Blue = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ColorPickerProgram_Blue");
Adafruit_MQTT_Subscribe cpp_Brightness = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ColorPickerProgram_Brightness");

//Globals for Light Effects
#include "FastLED.h"
#define Pixels 78
CRGB leds[Pixels];
#define PIN D4
uint16_t ledBrightness = 0;
uint16_t r = 0;
uint16_t g = 0;
uint16_t b = 0;




// ***************************************
// *************** Setup *****************
// ***************************************


void setup() {

  //Initialize Serial
  Serial.begin(115200);
  Serial.println("Booting");
  wifiSetup();

  //Subscirbe to MQTT Feeds
  mqtt.subscribe(&cpp_Red);
  mqtt.subscribe(&cpp_Green);
  mqtt.subscribe(&cpp_Blue);
  mqtt.subscribe(&cpp_Brightness);
  Serial.println("Debug - sub");

  //Initialize RGB
  FastLED.addLeds<WS2811, PIN, GRB>(leds, Pixels).setCorrection( TypicalLEDStrip );
}




// ***************************************
// ************* Da Loop *****************
// ***************************************


void loop() {

  //Network Housekeeping
  ArduinoOTA.handle();
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10))) {
    Serial.println("Subscription Recieved");
    uint16_t red = atoi((char *)cpp_Red.lastread);
    uint16_t green = atoi((char *)cpp_Green.lastread);
    uint16_t blue = atoi((char *)cpp_Blue.lastread);
    uint16_t brightness = atoi((char *)cpp_Blue.lastread);

    ledBrightness = brightness;
    r = red;
    g = green;
    b = blue;
  }

  FastLED.setBrightness(ledBrightness);
  fill_solid(leds, Pixels, CRGB(r, g, b));
  FastLED.show();

}




// ***************************************
// ********** Backbone Methods ***********
// ***************************************


void wifiSetup() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("ColorPickerProgram");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    //Serial.println("Connected");
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      //while (1);
      Serial.println("Wait 10 min to reconnect");
      delay(600000);
    }
  }
  Serial.println("MQTT Connected!");
}

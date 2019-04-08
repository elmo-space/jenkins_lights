#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

FASTLED_USING_NAMESPACE

// WIFI Settings
const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);

#define DATA_PIN    0
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    40
CRGB leds[NUM_LEDS];

String led_states[NUM_LEDS];
String power = "on";

#define MIN_BRIGHTNESS 8
#define MAX_BRIGHTNESS 220

int led = 0;

int FPS = 30;

CRGB color = CRGB::Green;

void setup() {
  delay(3000); // 3 second delay for recovery

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  server.on("/", handleRoot);

  server.on("/set", [](){
    handleBody();
  });

  server.onNotFound(handleNotFound);

  server.begin();
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  fill_rainbow( leds, NUM_LEDS, 0, 7);
  FastLED.show();
  FastLED.delay(3000);
}

void loop()
{
  server.handleClient();

  float breath = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;
  breath = map(breath, 0, 255, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  if ( power == "off" ) {
    FastLED.setBrightness(0);
  } else {
    FastLED.setBrightness(MAX_BRIGHTNESS);
    for ( int i; i < NUM_LEDS; i++ ) {
      if ( led_states[i] == "failed") {
        leds[i] = CRGB::Red;
      } else if ( led_states[i] == "unstable" ) {
        leds[i] = CRGB::Yellow;
      } else if ( led_states[i] == "stable" ) {
        leds[i] = CRGB::Green;
      } else if ( led_states[i] == "building" ) {
        leds[i].maximizeBrightness(breath);
      } else {
        leds[i] = CRGB::Black;
      }
    }
  }
  FastLED.show();
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void handleRoot() {
  server.send(200, "text/plain", "ok");
}

void handleBody() { //Handler for the body path
  if (server.hasArg("plain")== false){ //Check if body received
    server.send(200, "text/plain", "Body not received");
    return;
  }
  
  String message = "Body received:\n";
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  // Parameters
  String pwr = root["power"];   // global power?
  String mode = root["mode"];   // build_state?
  String act_led = root["led"]; // led to set
  int led = act_led.toInt();

  power = pwr;
  led_states[led] = mode;
  
  server.send(200, "text/plain", "ok");
}

void handleNotFound(){
  server.send(404);
}


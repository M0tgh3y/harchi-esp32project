#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <WiFiManager.h>

// Configuration - change per ESP32
const int deviceNumber = 1;  // 1 or 2

// Touch pin
#define TOUCH_PIN 4   // GPIO 4 as touch button

// WS2812 LED config
#define LED_PIN     13
#define NUM_LEDS    16
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

WebServer server(80);

// Touch detection
bool touched = false;

void setup() {
  Serial.begin(115200);

  // LED setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(100);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Touch pin setup
  pinMode(TOUCH_PIN, INPUT_PULLUP);

  // WiFi setup
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("ESP32_LED_Controller")) {
    Serial.println("Failed to connect, restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/setcolor", handleSetColor);
  server.on("/touchstatus", handleTouchStatus);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  int state = digitalRead(TOUCH_PIN);
  touched = (state == LOW);   // pressed = LOW because of pull-up
}

void handleRoot() {
  String message = "ESP32 WS2812 Controller #";
  message += deviceNumber;
  message += "\n\n/setcolor?hex=RRGGBB\n/touchstatus";
  server.send(200, "text/plain", message);
}

void handleSetColor() {
  if (server.hasArg("hex")) {
    String hex = server.arg("hex");
    if (hex.length() == 6) {
      long number = strtol(hex.c_str(), NULL, 16);
      int r = number >> 16;
      int g = (number >> 8) & 0xFF;
      int b = number & 0xFF;

      fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
      FastLED.show();

      server.send(200, "text/plain", "Color set to #" + hex);
      return;
    }
  }
  server.send(400, "text/plain", "Bad request. Use /setcolor?hex=RRGGBB");
}

void handleTouchStatus() {
  if (touched) {
    server.send(200, "text/plain", "touched");
  } else {
    server.send(200, "text/plain", "not_touched");
  }
}

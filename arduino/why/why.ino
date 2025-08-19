#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <WiFiManager.h>

// Configuration - customize these for each ESP32

const int deviceNumber = 2; // Change to 2 for the second ESP32

WebServer server(80);
// WS2812 LED configuration
#define LED_PIN     13       // Data pin connected to the LED strip
#define NUM_LEDS    16       // Number of LEDs in your strip
#define LED_TYPE    WS2812   // LED chip type
#define COLOR_ORDER GRB      // Color order (may vary by strip)
CRGB leds[NUM_LEDS];         // LED array

void setup() {
  Serial.begin(115200);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(100);  // Initial brightness (0-255)

  //***************
  WiFiManager wifiManager;
  
  // Uncomment for testing to reset settings
  // wifiManager.resetSettings();
  
  // Set timeout until configuration portal gets turned off
  wifiManager.setTimeout(180); // 3 minutes
  
  // Start configuration portal with a custom name
  if(!wifiManager.autoConnect("ESP32_LED_Controller")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart(); // Reset and try again
    delay(5000);
  }
  //************
  
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Set all LEDs to off initially
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Set up server routes
  server.on("/", handleRoot);
  server.on("/setcolor", handleSetColor);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String message = "ESP32 WS2812 Controller #";
  message += deviceNumber;
  message += "\n\nUse /setcolor?hex=RRGGBB to set color for all LEDs";
  server.send(200, "text/plain", message);
}

void handleSetColor() {
  if (server.hasArg("hex")) {
    String hex = server.arg("hex");
    if (hex.length() == 6) {
      long number = strtol(hex.c_str(), NULL, 16);
      
      // Extract RGB components
      int r = number >> 16;
      int g = (number >> 8) & 0xFF;
      int b = number & 0xFF;
      
      // Set all LEDs to the same color
      fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
      FastLED.show();
      
      server.send(200, "text/plain", "Color set to #" + hex + " on all LEDs");
      return;
    }
  }
  server.send(400, "text/plain", "Bad request - use /setcolor?hex=RRGGBB");
}
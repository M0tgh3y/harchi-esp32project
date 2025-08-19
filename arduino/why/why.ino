#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <WiFiManager.h>

// Configuration - customize these for each ESP32
const int deviceNumber = 2; // Change to 2 for the second ESP32

// Touch pin configuration - GPIO 4
#define TOUCH_PIN T0        // Using GPIO 4 as touch pin
#define TOUCH_THRESHOLD 20 // Lower value means more sensitive (typical range: 20-80)

WebServer server(80);

// WS2812 LED configuration
#define LED_PIN     13       // Data pin connected to the LED strip
#define NUM_LEDS    16       // Number of LEDs in your strip
#define LED_TYPE    WS2812   // LED chip type
#define COLOR_ORDER GRB      // Color order (may vary by strip)
CRGB leds[NUM_LEDS];         // LED array

// Touch detection variables
bool touched = false;
bool lastTouched = false;
unsigned long touchStartTime = 0;
unsigned long lastTouchReportTime = 0;

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
  server.on("/touchstatus", handleTouchStatus); // New endpoint for touch status
  
  server.begin();
  Serial.println("HTTP server started");
  
  // Print touch pin info
  Serial.print("Touch sensor initialized on GPIO ");
  Serial.println(TOUCH_PIN);
  Serial.println("Touch the pin to test sensitivity");
}

void loop() {
  server.handleClient();
  
  // Read touch sensor - GPIO 4 doesn't have a dedicated touch channel
  // So we'll use digitalRead with a pullup resistor instead
  // We'll enable the internal pullup resistor and check for LOW state
  
  // For GPIO 4, we need to set it as input with pullup
  pinMode(TOUCH_PIN, INPUT_PULLUP);
  int touchState = digitalRead(TOUCH_PIN);
  
  // Check if touched (LOW means touched because of pullup)
  if (touchState == LOW) {
    if (!lastTouched) {
      touchStartTime = millis();
      lastTouched = true;
      Serial.println("Touch started");
    }
    
    // Require touch to be held for 50ms to avoid false positives
    if (millis() - touchStartTime > 50 && !touched) {
      touched = true;
      Serial.println("Touch confirmed!");
    }
  } else {
    if (lastTouched) {
      Serial.println("Touch ended");
    }
    lastTouched = false;
    touched = false;
  }
  
  // Print touch status periodically for debugging
  if (millis() - lastTouchReportTime > 2000) {
    Serial.print("Touch pin state: ");
    Serial.println(touchState);
    Serial.print("Touched status: ");
    Serial.println(touched);
    lastTouchReportTime = millis();
  }
}

void handleRoot() {
  String message = "ESP32 WS2812 Controller #";
  message += deviceNumber;
  message += "\n\nUse /setcolor?hex=RRGGBB to set color for all LEDs";
  message += "\nUse /touchstatus to check if touch pin was activated";
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

// New function to handle touch status requests
void handleTouchStatus() {
  if (touched) {
    server.send(200, "text/plain", "touched");
    touched = false;
    Serial.println("Touch status reported and reset");
  } else {
    server.send(200, "text/plain", "not_touched");
  }
}
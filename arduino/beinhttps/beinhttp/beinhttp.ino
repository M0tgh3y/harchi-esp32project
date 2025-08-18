#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_https_server.h>
#include <FastLED.h>

// Configuration - customize these for each ESP32
const char* ssid = "Lalala";
const char* password = "";
const int deviceNumber = 1; // Change to 2 for the second ESP32

// WS2812 LED configuration
#define LED_PIN     13       // Data pin connected to the LED strip
#define NUM_LEDS    16       // Number of LEDs in your strip
#define LED_TYPE    WS2812   // LED chip type
#define COLOR_ORDER GRB      // Color order (may vary by strip)
CRGB leds[NUM_LEDS];         // LED array

// HTTPS Server
httpd_handle_t server = NULL;

// Certificate data - you need to provide these
extern const uint8_t servercert_pem_start[] asm("_binary_servercert_pem_start");
extern const uint8_t servercert_pem_end[]   asm("_binary_servercert_pem_end");
extern const uint8_t prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
extern const uint8_t prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");

// Root handler
esp_err_t handle_root(httpd_req_t *req) {
    char message[100];
    snprintf(message, sizeof(message), "ESP32 WS2812 Controller #%d\n\nUse /setcolor?hex=RRGGBB to set color for all LEDs", deviceNumber);
    httpd_resp_send(req, message, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Set color handler
esp_err_t handle_set_color(httpd_req_t *req) {
    char buf[100];
    char hex[7] = {0};
    
    // Get query parameter
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "hex", hex, sizeof(hex)) == ESP_OK) {
            if (strlen(hex) == 6) {
                long number = strtol(hex, NULL, 16);
                
                // Extract RGB components
                int r = number >> 16;
                int g = (number >> 8) & 0xFF;
                int b = number & 0xFF;
                
                // Set all LEDs to the same color
                fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
                FastLED.show();
                
                char response[50];
                snprintf(response, sizeof(response), "Color set to #%s on all LEDs", hex);
                httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            }
        }
    }
    
    httpd_resp_send(req, "Bad request - use /setcolor?hex=RRGGBB", HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, "400 Bad Request");
    return ESP_OK;
}

void start_https_server() {
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    
    // Configure SSL certificate and private key
    conf.httpd.stack_size = 10240;
    conf.cacert_pem = servercert_pem_start;
    conf.cacert_len = servercert_pem_end - servercert_pem_start;
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;
    
    // Start the HTTPS server
    if (httpd_ssl_start(&server, &conf) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = handle_root,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        
        httpd_uri_t setcolor_uri = {
            .uri       = "/setcolor",
            .method    = HTTP_GET,
            .handler   = handle_set_color,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &setcolor_uri);
        
        Serial.println("HTTPS server started");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize LED strip
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(100);  // Initial brightness (0-255)
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        // Blink LED during connection
        leds[0] = CRGB::Blue;
        FastLED.show();
        delay(100);
        leds[0] = CRGB::Black;
        FastLED.show();
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start HTTPS server
    start_https_server();
}

void loop() {
    // Nothing to do here - server runs in background
}
#include "wifi.h"
#include "wifi-config-manager.h"
#include "ota-handler.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>

// External references from main.cpp
extern Adafruit_NeoPixel myLedStrip;
extern int pixelIndex(int col, int row);

#define BUTTON_PIN 0  // Same as in main.cpp

// WiFi reconnection variables
static unsigned long lastWiFiCheck = 0;
static bool wasConnected = false;
static unsigned long disconnectTime = 0;
static bool isReconnecting = false;
const unsigned long WIFI_CHECK_INTERVAL = 5000; // Check every 5 seconds
const unsigned long RECONNECT_DELAY = 10000; // Wait 10 seconds before reconnecting

void connectWifi()
{
    Serial.println("Starting WiFi connection process...");
    
    // Initialize the WiFi configuration manager
    if (!wifiConfigManager.begin()) {
        Serial.println("Failed to initialize WiFi config manager");
    }
    
    // Try to connect to saved WiFi network
    if (wifiConfigManager.hasValidConfig()) {
        Serial.println("Found saved WiFi configuration, attempting connection...");
        if (wifiConfigManager.connectToWiFi()) {
            Serial.println("Successfully connected to saved WiFi network");
            initOTA(); // Initialize OTA after successful WiFi connection
            return; // Connected successfully
        } else {
            Serial.println("Failed to connect to saved WiFi network, starting configuration mode");
        }
    } else {
        Serial.println("No valid WiFi configuration found, starting configuration mode");
    }
    
    // Start configuration mode (Access Point with captive portal)
    wifiConfigManager.startConfigMode();
    Serial.println("WiFi configuration mode started. Connect to 'NeoPixel-Setup' network to configure WiFi.");
    
    // Show AP mode animation: purple corners pulsing
    const int cols = 32;
    const int rows = 7;
    unsigned long lastPulse = 0;
    int pulsePhase = 0;
    
    // Wait in configuration mode until WiFi is configured
    while (wifiConfigManager.isConfigMode()) {
        wifiConfigManager.handleClient();
        handleOTA();
        
        // Pulsing purple corners animation to indicate config mode
        if (millis() - lastPulse >= 50) {
            lastPulse = millis();
            pulsePhase = (pulsePhase + 1) % 100;
            
            uint8_t brightness = (uint8_t)(32 + 32 * sin(pulsePhase * 0.0628)); // 0.0628 ≈ 2π/100
            
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(brightness, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(cols-1, 0), myLedStrip.Color(brightness, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(0, rows-1), myLedStrip.Color(brightness, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(cols-1, rows-1), myLedStrip.Color(brightness, 0, brightness));
            myLedStrip.show();
        }
        
        delay(10);
    }
}

// Check if WiFi is connected
bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Handle WiFi reconnection if connection is lost
void handleWiFiReconnection() {
    unsigned long currentTime = millis();
    
    // Matrix dimensions
    const int cols = 32;
    const int rows = 7;
    
    // Only check WiFi status periodically to avoid excessive checking
    if (currentTime - lastWiFiCheck < WIFI_CHECK_INTERVAL) {
        return;
    }
    lastWiFiCheck = currentTime;
    
    // Handle configuration mode if active
    if (wifiConfigManager.isConfigMode()) {
        wifiConfigManager.handleClient();
        return;
    }
    
    bool currentlyConnected = isWiFiConnected();
    
    // Detect disconnection
    if (wasConnected && !currentlyConnected && !isReconnecting) {
        Serial.println("WiFi connection lost! Starting reconnection process...");
        disconnectTime = currentTime;
        isReconnecting = true;
        
        // Brief visual indication of disconnection (red flash on corners)
        myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(127, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(cols-1, 0), myLedStrip.Color(127, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(0, rows-1), myLedStrip.Color(127, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(cols-1, rows-1), myLedStrip.Color(127, 0, 0));
        myLedStrip.show();
        delay(100);
        // Clear the corners after indication
        myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(cols-1, 0), myLedStrip.Color(0, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(0, rows-1), myLedStrip.Color(0, 0, 0));
        myLedStrip.setPixelColor(pixelIndex(cols-1, rows-1), myLedStrip.Color(0, 0, 0));
        myLedStrip.show();
    }
    
    // Handle reconnection after delay
    if (isReconnecting && !currentlyConnected) {
        if (currentTime - disconnectTime >= RECONNECT_DELAY) {
            Serial.println("Attempting WiFi reconnection...");
            
            // Try to reconnect using the WiFi config manager
            if (wifiConfigManager.connectToWiFi()) {
                Serial.println("WiFi reconnected successfully!");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                isReconnecting = false;
                initOTA(); // Reinitialize OTA after reconnection
                
                // Brief success indication (green flash on corners)
                myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 127, 0));
                myLedStrip.setPixelColor(pixelIndex(cols-1, 0), myLedStrip.Color(0, 127, 0));
                myLedStrip.setPixelColor(pixelIndex(0, rows-1), myLedStrip.Color(0, 127, 0));
                myLedStrip.setPixelColor(pixelIndex(cols-1, rows-1), myLedStrip.Color(0, 127, 0));
                myLedStrip.show();
                delay(200);
                myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
                myLedStrip.setPixelColor(pixelIndex(cols-1, 0), myLedStrip.Color(0, 0, 0));
                myLedStrip.setPixelColor(pixelIndex(0, rows-1), myLedStrip.Color(0, 0, 0));
                myLedStrip.setPixelColor(pixelIndex(cols-1, rows-1), myLedStrip.Color(0, 0, 0));
                myLedStrip.show();
            } else {
                Serial.println("Failed to reconnect. Starting configuration mode...");
                isReconnecting = false;
                wifiConfigManager.startConfigMode();
            }
        }
    }
    
    // Update connection state
    wasConnected = currentlyConnected;
}

// Handle WiFi configuration reset button
void handleWiFiConfigButton() {
    wifiConfigManager.checkResetButton(BUTTON_PIN);
}

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

// Maximum lengths for WiFi credentials
#define WIFI_SSID_MAX_LEN 64
#define WIFI_PASSWORD_MAX_LEN 64
#define CONFIG_FILENAME "/wifi_config.json"
#define AP_SSID "NeoPixel-Setup"
#define AP_PASSWORD "setup123"
#define CAPTIVE_PORTAL_IP IPAddress(192, 168, 4, 1)

// WiFi configuration structure
struct WiFiConfig {
    char ssid[WIFI_SSID_MAX_LEN];
    char password[WIFI_PASSWORD_MAX_LEN];
    bool isValid;
};

class WiFiConfigManager {
private:
    WiFiConfig config;
    ESP8266WebServer* configServer;
    DNSServer dnsServer;
    bool isAPMode;
    unsigned long buttonPressStart;
    bool buttonPressed;
    static const unsigned long BUTTON_HOLD_TIME = 3000; // 3 seconds to trigger reset
    
    void startConfigServer();
    void stopConfigServer();
    String getConfigPage();
    String getScanResultsJson();
    void handleRoot();
    void handleScan();
    void handleSave();
    void handleStatus();
    bool saveConfig();
    bool loadConfig();
    void clearConfig();
    
public:
    WiFiConfigManager();
    ~WiFiConfigManager();
    
    // Main interface methods
    bool begin();
    void handleClient();
    void checkResetButton(int buttonPin);
    bool connectToWiFi();
    void startConfigMode();
    bool isConfigMode() { return isAPMode; }
    bool hasValidConfig() { return config.isValid; }
    const char* getSSID() { return config.ssid; }
    const char* getPassword() { return config.password; }
    void resetConfig();
};

extern WiFiConfigManager wifiConfigManager;
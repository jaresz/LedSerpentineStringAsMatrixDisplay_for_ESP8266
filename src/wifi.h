// wifi.h - declaration and constants for WiFi handling
#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel myLedStrip;

void connectWifi();
void handleWiFiReconnection();
bool isWiFiConnected();
void handleWiFiConfigButton(); // Function to handle WiFi reset button

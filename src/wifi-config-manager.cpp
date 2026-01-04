#include "wifi-config-manager.h"
#include "ota-handler.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Global instance
WiFiConfigManager wifiConfigManager;

// External reference to LED strip for visual feedback
extern Adafruit_NeoPixel myLedStrip;
extern int pixelIndex(int col, int row);

WiFiConfigManager::WiFiConfigManager() {
    configServer = nullptr;
    isAPMode = false;
    buttonPressStart = 0;
    buttonPressed = false;
    config.isValid = false;
    memset(config.ssid, 0, WIFI_SSID_MAX_LEN);
    memset(config.password, 0, WIFI_PASSWORD_MAX_LEN);
}

WiFiConfigManager::~WiFiConfigManager() {
    if (configServer) {
        delete configServer;
    }
}

bool WiFiConfigManager::begin() {
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        // Try to format and mount again
        Serial.println("Formatting LittleFS...");
        if (!LittleFS.format()) {
            Serial.println("Failed to format LittleFS");
            return false;
        }
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount LittleFS after format");
            return false;
        }
    }
    
    // Try to load existing configuration
    return loadConfig();
}

bool WiFiConfigManager::loadConfig() {
    if (!LittleFS.exists(CONFIG_FILENAME)) {
        Serial.println("WiFi config file doesn't exist");
        config.isValid = false;
        return false;
    }
    
    File file = LittleFS.open(CONFIG_FILENAME, "r");
    if (!file) {
        Serial.println("Failed to open config file for reading");
        config.isValid = false;
        return false;
    }
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("Failed to parse config file");
        config.isValid = false;
        return false;
    }
    
    strncpy(config.ssid, doc["ssid"] | "", WIFI_SSID_MAX_LEN - 1);
    strncpy(config.password, doc["password"] | "", WIFI_PASSWORD_MAX_LEN - 1);
    config.isValid = doc["valid"] | false;
    
    if (strlen(config.ssid) > 0 && config.isValid) {
        Serial.print("Loaded WiFi config: ");
        Serial.println(config.ssid);
        return true;
    } else {
        config.isValid = false;
        return false;
    }
}

bool WiFiConfigManager::saveConfig() {
    File file = LittleFS.open(CONFIG_FILENAME, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    DynamicJsonDocument doc(512);
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["valid"] = config.isValid;
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write config file");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("WiFi config saved successfully");
    return true;
}

void WiFiConfigManager::clearConfig() {
    config.isValid = false;
    memset(config.ssid, 0, WIFI_SSID_MAX_LEN);
    memset(config.password, 0, WIFI_PASSWORD_MAX_LEN);
    
    if (LittleFS.exists(CONFIG_FILENAME)) {
        LittleFS.remove(CONFIG_FILENAME);
    }
    
    Serial.println("WiFi config cleared");
}

void WiFiConfigManager::resetConfig() {
    clearConfig();
    Serial.println("WiFi configuration reset. Restarting...");
    delay(1000);
    ESP.restart();
}

bool WiFiConfigManager::connectToWiFi() {
    if (!config.isValid || strlen(config.ssid) == 0) {
        Serial.println("No valid WiFi configuration");
        return false;
    }
    
    Serial.print("Connecting to WiFi: ");
    Serial.println(config.ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    // Visual feedback during connection
    unsigned long startTime = millis();
    const unsigned long timeout = 30000; // 30 seconds
    unsigned long lastToggle = 0;
    const unsigned long toggleInterval = 200;
    bool ledState = false;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        // Pulse corner LEDs in blue while connecting
        if (millis() - lastToggle >= toggleInterval) {
            lastToggle = millis();
            ledState = !ledState;
            
            uint8_t brightness = ledState ? 64 : 0;
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 0, brightness));
            myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 0, brightness));
            myLedStrip.show();
        }
        delay(100);
    }
    
    // Clear corner LEDs
    myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 0, 0));
    myLedStrip.show();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi connected! IP address: ");
        Serial.println(WiFi.localIP());
        
        // Initialize OTA after successful connection
        initOTA();
        
        // Success animation: Green flash
        for (int i = 0; i < 3; i++) {
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 127, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 127, 0));
            myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 127, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 127, 0));
            myLedStrip.show();
            delay(200);
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 0, 0));
            myLedStrip.show();
            delay(200);
        }
        return true;
    } else {
        Serial.println();
        Serial.println("Failed to connect to WiFi");
        
        // Error animation: Red flash
        for (int i = 0; i < 5; i++) {
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(127, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(127, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(127, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(127, 0, 0));
            myLedStrip.show();
            delay(150);
            myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 0, 0));
            myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 0, 0));
            myLedStrip.show();
            delay(150);
        }
        
        // Mark config as invalid and clear it
        config.isValid = false;
        saveConfig();
        return false;
    }
}

void WiFiConfigManager::startConfigMode() {
    Serial.println("Starting WiFi configuration mode");
    
    isAPMode = true;
    
    // Stop any existing connections
    WiFi.disconnect();
    delay(100);
    
    // Start Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(CAPTIVE_PORTAL_IP, CAPTIVE_PORTAL_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print("Access Point started. SSID: ");
    Serial.println(AP_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Start captive portal DNS server
    dnsServer.start(53, "*", CAPTIVE_PORTAL_IP);
    
    // Start configuration web server
    startConfigServer();
    
    // Visual feedback: Purple corners to indicate config mode
    myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(64, 0, 64));
    myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(64, 0, 64));
    myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(64, 0, 64));
    myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(64, 0, 64));
    myLedStrip.show();
}

void WiFiConfigManager::startConfigServer() {
    if (configServer) {
        delete configServer;
    }
    
    configServer = new ESP8266WebServer(80);
    
    configServer->on("/", [this]() { handleRoot(); });
    configServer->on("/scan", [this]() { handleScan(); });
    configServer->on("/save", HTTP_POST, [this]() { handleSave(); });
    configServer->on("/status", [this]() { handleStatus(); });
    configServer->onNotFound([this]() { handleRoot(); }); // Captive portal
    
    configServer->begin();
    Serial.println("Config web server started");
}

void WiFiConfigManager::stopConfigServer() {
    if (configServer) {
        configServer->stop();
        delete configServer;
        configServer = nullptr;
    }
    
    dnsServer.stop();
    isAPMode = false;
    
    // Clear config mode visual feedback
    myLedStrip.setPixelColor(pixelIndex(0, 0), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(31, 0), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(0, 6), myLedStrip.Color(0, 0, 0));
    myLedStrip.setPixelColor(pixelIndex(31, 6), myLedStrip.Color(0, 0, 0));
    myLedStrip.show();
}

void WiFiConfigManager::handleClient() {
    if (isAPMode && configServer) {
        dnsServer.processNextRequest();
        configServer->handleClient();
    }
}

void WiFiConfigManager::checkResetButton(int buttonPin) {
    int buttonState = digitalRead(buttonPin);
    
    if (buttonState == LOW && !buttonPressed) {
        // Button just pressed
        buttonPressed = true;
        buttonPressStart = millis();
    } else if (buttonState == HIGH && buttonPressed) {
        // Button released
        buttonPressed = false;
        unsigned long pressDuration = millis() - buttonPressStart;
        
        if (pressDuration >= BUTTON_HOLD_TIME) {
            Serial.println("Reset button held - triggering WiFi reset");
            resetConfig();
        }
    }
}

void WiFiConfigManager::handleRoot() {
    configServer->send(200, "text/html", getConfigPage());
}

void WiFiConfigManager::handleScan() {
    configServer->send(200, "application/json", getScanResultsJson());
}

void WiFiConfigManager::handleSave() {
    if (configServer->hasArg("ssid") && configServer->hasArg("password")) {
        String ssid = configServer->arg("ssid");
        String password = configServer->arg("password");
        
        if (ssid.length() > 0 && ssid.length() < WIFI_SSID_MAX_LEN && 
            password.length() < WIFI_PASSWORD_MAX_LEN) {
            
            strncpy(config.ssid, ssid.c_str(), WIFI_SSID_MAX_LEN - 1);
            strncpy(config.password, password.c_str(), WIFI_PASSWORD_MAX_LEN - 1);
            config.isValid = true;
            
            if (saveConfig()) {
                configServer->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved. Restarting...\"}");
                delay(1000);
                // Stop config mode before restart
                stopConfigServer();
                ESP.restart();
            } else {
                configServer->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save configuration\"}");
            }
        } else {
            configServer->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid SSID or password length\"}");
        }
    } else {
        configServer->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID or password\"}");
    }
}

void WiFiConfigManager::handleStatus() {
    DynamicJsonDocument doc(256);
    doc["ap_mode"] = isAPMode;
    doc["connected"] = WiFi.status() == WL_CONNECTED;
    if (WiFi.status() == WL_CONNECTED) {
        doc["ip"] = WiFi.localIP().toString();
        doc["ssid"] = WiFi.SSID();
    }
    
    String response;
    serializeJson(doc, response);
    configServer->send(200, "application/json", response);
}

String WiFiConfigManager::getScanResultsJson() {
    DynamicJsonDocument doc(4096);
    JsonArray networks = doc.createNestedArray("networks");
    
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryption"] = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WiFiConfigManager::getConfigPage() {
    return R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NeoPixel WiFi Setup</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            color: white;
            min-height: 100vh;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.1);
            padding: 30px;
            border-radius: 20px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.2em;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
        }
        select, input {
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 8px;
            background: rgba(255, 255, 255, 0.9);
            color: #333;
            font-size: 16px;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 8px;
            background: #4CAF50;
            color: white;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            margin-top: 10px;
            transition: background 0.3s;
        }
        button:hover {
            background: #45a049;
        }
        button:disabled {
            background: #888;
            cursor: not-allowed;
        }
        .scan-btn {
            background: #2196F3;
        }
        .scan-btn:hover {
            background: #1976D2;
        }
        .status {
            margin-top: 20px;
            padding: 10px;
            border-radius: 8px;
            text-align: center;
        }
        .status.success {
            background: rgba(76, 175, 80, 0.3);
        }
        .status.error {
            background: rgba(244, 67, 54, 0.3);
        }
        .loading {
            text-align: center;
            margin: 20px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 WiFi Setup</h1>
        <form id="wifiForm">
            <div class="form-group">
                <label for="networkSelect">Select WiFi Network:</label>
                <select id="networkSelect" name="ssid" required>
                    <option value="">Scanning for networks...</option>
                </select>
                <button type="button" id="scanBtn" class="scan-btn">🔄 Rescan Networks</button>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" placeholder="Enter password (leave empty for open networks)">
            </div>
            
            <button type="submit" id="saveBtn">💾 Save and Connect</button>
        </form>
        
        <div id="status" class="status" style="display: none;"></div>
    </div>

    <script>
        let networks = [];

        function showStatus(message, type = 'success') {
            const statusEl = document.getElementById('status');
            statusEl.textContent = message;
            statusEl.className = 'status ' + type;
            statusEl.style.display = 'block';
        }

        function scanNetworks() {
            const select = document.getElementById('networkSelect');
            const scanBtn = document.getElementById('scanBtn');
            
            select.innerHTML = '<option value="">Scanning...</option>';
            scanBtn.disabled = true;
            scanBtn.textContent = '🔄 Scanning...';

            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    networks = data.networks || [];
                    select.innerHTML = '<option value="">Select a network...</option>';
                    
                    networks.sort((a, b) => b.rssi - a.rssi); // Sort by signal strength
                    
                    networks.forEach(network => {
                        if (network.ssid && network.ssid.trim() !== '') {
                            const option = document.createElement('option');
                            option.value = network.ssid;
                            option.textContent = network.ssid + ' (' + 
                                (network.rssi > -50 ? 'Excellent' : 
                                 network.rssi > -60 ? 'Good' : 
                                 network.rssi > -70 ? 'Fair' : 'Weak') + 
                                ')' + (network.encryption ? ' 🔒' : ' 🔓');
                            select.appendChild(option);
                        }
                    });
                    
                    scanBtn.disabled = false;
                    scanBtn.textContent = '🔄 Rescan Networks';
                })
                .catch(error => {
                    console.error('Scan error:', error);
                    select.innerHTML = '<option value="">Scan failed - try again</option>';
                    scanBtn.disabled = false;
                    scanBtn.textContent = '🔄 Rescan Networks';
                    showStatus('Failed to scan networks', 'error');
                });
        }

        document.getElementById('scanBtn').addEventListener('click', scanNetworks);

        document.getElementById('wifiForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const ssid = document.getElementById('networkSelect').value;
            const password = document.getElementById('password').value;
            const saveBtn = document.getElementById('saveBtn');
            
            if (!ssid) {
                showStatus('Please select a network', 'error');
                return;
            }
            
            saveBtn.disabled = true;
            saveBtn.textContent = '💾 Saving...';
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);
            
            fetch('/save', {
                method: 'POST',
                body: formData
            })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        showStatus('Configuration saved! Device is restarting...', 'success');
                        setTimeout(() => {
                            showStatus('Device should now connect to your WiFi network. You can close this page.', 'success');
                        }, 3000);
                    } else {
                        showStatus('Error: ' + data.message, 'error');
                        saveBtn.disabled = false;
                        saveBtn.textContent = '💾 Save and Connect';
                    }
                })
                .catch(error => {
                    console.error('Save error:', error);
                    showStatus('Failed to save configuration', 'error');
                    saveBtn.disabled = false;
                    saveBtn.textContent = '💾 Save and Connect';
                });
        });

        // Initial network scan
        scanNetworks();
        
        // Auto-refresh every 30 seconds
        setInterval(scanNetworks, 30000);
    </script>
</body>
</html>
)HTML";
}
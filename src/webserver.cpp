#include "webserver.h"
#include "wifi-config-manager.h"
#include <ESP8266WiFi.h>

ESP8266WebServer webServer(80);
volatile AnimationMode currentAnimation = ANIMATION_AUTO;
volatile bool animationChanged = false;


const char HTML_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NeoPixel Animation Controller</title>
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
            max-width: 600px;
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
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        .temperature-display {
            text-align: center;
            margin: 15px 0 30px 0;
            font-size: 1.1em;
            background: rgba(76, 175, 80, 0.2);
            padding: 15px;
            border-radius: 10px;
            display: flex;
            justify-content: space-around;
            flex-wrap: wrap;
        }
        .current-animation {
            text-align: center;
            margin-bottom: 30px;
            padding: 15px;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 10px;
            font-size: 1.2em;
        }
        .button-grid {
            display: grid;
            gap: 15px;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
        }
        .animation-btn {
            padding: 20px;
            font-size: 1.1em;
            border: none;
            border-radius: 15px;
            background: linear-gradient(45deg, #4CAF50, #45a049);
            color: white;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }
        .animation-btn:hover {
            transform: translateY(-3px);
            box-shadow: 0 8px 25px rgba(0, 0, 0, 0.3);
        }
        .animation-btn.qix {
            background: linear-gradient(45deg, #9C27B0, #673AB7);
        }
        .animation-btn.starfield {
            background: linear-gradient(45deg, #1A237E, #000051);
        }
        .animation-btn.starwarp {
            background: linear-gradient(45deg, #0D47A1, #1976D2, #42A5F5);
        }
        .animation-btn.nebula {
            background: linear-gradient(45deg, #4A148C, #7B1FA2, #9C27B0, #E91E63);
        }
        .animation-btn.red {
            background: linear-gradient(45deg, #D32F2F, #B71C1C);
        }
        .animation-btn.auto {
            background: linear-gradient(45deg, #2196F3, #1976D2);
        }
        .animation-btn.matrix {
            background: linear-gradient(45deg, #4CAF50, #388E3C);
        }
        .animation-btn.color-picker {
            background: linear-gradient(45deg, #607D8B, #455A64);
        }
        .animation-btn.draw {
            background: linear-gradient(45deg, #795548, #5D4037);
        }
        .animation-btn.temperature {
            background: linear-gradient(45deg, #FF5722, #D84315);
        }
        .status {
            text-align: center;
            margin-top: 20px;
            padding: 10px;
            border-radius: 8px;
            background: rgba(255, 255, 255, 0.15);
            display: none;
        }
        .status.success {
            background: rgba(76, 175, 80, 0.3);
        }
        .status.error {
            background: rgba(244, 67, 54, 0.3);
        }
        .device-info {
            text-align: center;
            margin-top: 30px;
            opacity: 0.8;
            font-size: 0.9em;
        }
        .speed-btn.selected {
            background: linear-gradient(45deg, #FF6B6B, #4ECDC4);
            transform: scale(1.05);
        }
        
        /* Color picker styles */
        .color-section {
            margin-top: 20px;
            padding: 20px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            display: none;
        }
        
        .color-picker-wrapper {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 15px;
            margin-bottom: 15px;
        }
        
        .color-input {
            width: 80px;
            height: 40px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
        }
        
        .color-presets {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
        }
        
        .color-preset {
            width: 40px;
            height: 40px;
            border: 2px solid white;
            border-radius: 8px;
            cursor: pointer;
            transition: transform 0.2s ease;
        }
        
        .color-preset:hover {
            transform: scale(1.1);
        }
        
        /* Drawing grid styles */
        .drawing-section {
            margin-top: 20px;
            padding: 20px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            display: none;
        }
        
        .drawing-grid {
            display: grid;
            grid-template-columns: repeat(32, 15px);
            grid-template-rows: repeat(7, 15px);
            gap: 1px;
            justify-content: center;
            margin: 15px 0;
            background: rgba(0, 0, 0, 0.3);
            padding: 5px;
            border-radius: 8px;
        }
        
        .grid-pixel {
            width: 15px;
            height: 15px;
            background: rgba(255, 255, 255, 0.1);
            cursor: pointer;
            transition: all 0.1s ease;
        }
        
        .grid-pixel.active {
            background: #00ff00;
        }
        
        .grid-pixel:hover {
            background: rgba(255, 255, 255, 0.3);
        }
        
        .drawing-controls {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
            margin-top: 15px;
        }
        
        .draw-btn {
            padding: 10px 20px;
            border: none;
            border-radius: 8px;
            background: rgba(255, 255, 255, 0.2);
            color: white;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .draw-btn:hover {
            background: rgba(255, 255, 255, 0.3);
            transform: translateY(-2px);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>LED Strip Controller</h1>
        
        <div class="temperature-display">
            <strong>🌡️ Temperature:</strong> <span id="temperature">--.-°C</span>
            <strong>💧 Humidity:</strong> <span id="humidity">--%</span>
        </div>
        
        <div class="current-animation">
            <strong>Current Animation:</strong> <span id="current">Temperature</span>
        </div>
        
        <div class="button-grid">
            <button class="animation-btn temperature" onclick="setAnimation(0, 'Temperature')">
                Temperature
                <br><small>Display temperature from AHT10</small>
            </button>
            <button class="animation-btn qix" onclick="setAnimation(1, 'Qix Lines')">
                Qix Lines
                <br><small>Moving geometric lines</small>
            </button>
            <button class="animation-btn starfield" onclick="setAnimation(2, 'Starfield')">
                Starfield
                <br><small>Classic moving stars</small>
            </button>
            <button class="animation-btn starwarp" onclick="setAnimation(3, 'Star Warp')">
                Star Warp
                <br><small>First-person space travel</small>
            </button>
            <button class="animation-btn nebula" onclick="setAnimation(4, 'Nebula Swirl')">
                Nebula Swirl
                <br><small>Rotating cosmic clouds</small>
            </button>
            <button class="animation-btn red" onclick="setAnimation(5, 'All Red (10%)')">
                All Red (10%)
                <br><small>Solid red at low brightness</small>
            </button>
            <button class="animation-btn auto" onclick="setAnimation(6, 'Auto Cycle')">
                Auto Cycle
                <br><small>Cycle through all animations automatically</small>
            </button>
            <button class="animation-btn matrix" onclick="setAnimation(7, 'Matrix Rain')">
                Matrix Rain
                <br><small>Green falling code effect</small>
            </button>
            <button class="animation-btn fire" onclick="setAnimation(8, 'Fire Effect')">
                Fire Effect
                <br><small>Warm burning fire simulation</small>
            </button>
            <button class="animation-btn color-picker" onclick="setAnimation(9, 'Color Picker')">
                Color Picker
                <br><small>Solid color display</small>
            </button>
            <button class="animation-btn draw" onclick="setAnimation(10, 'Draw Mode')">
                Draw Mode
                <br><small>Pixel-by-pixel drawing</small>
            </button>
        </div>
        
        <!-- Animation Speed Controls -->
        <div class="speed-controls" id="speed-controls" style="margin-top: 20px; text-align: center; display: block;">
            <h3 style="margin-bottom: 15px;">Animation Speed</h3>
            <div style="margin-bottom: 15px;">
                <input type="range" id="speed-slider" min="0.2" max="3.0" step="0.1" value="1.0" onchange="changeSpeed(this.value)" style="width: 200px; margin: 0 10px;">
                <span id="speed-display" style="font-weight: bold; min-width: 50px; display: inline-block; text-align: center;">1.0x</span>
            </div>
            <div class="speed-presets" style="display: flex; flex-wrap: wrap; gap: 10px; justify-content: center;">
                <button class="speed-btn" onclick="setSpeed(0.5)" style="padding: 8px 15px; font-size: 14px;">Slow (0.5x)</button>
                <button class="speed-btn selected" onclick="setSpeed(1.0)" style="padding: 8px 15px; font-size: 14px;">Normal (1.0x)</button>
                <button class="speed-btn" onclick="setSpeed(2.0)" style="padding: 8px 15px; font-size: 14px;">Fast (2.0x)</button>
            </div>
        </div>
        
        <!-- WiFi Settings Section -->
        <div class="wifi-settings" style="margin-top: 20px; text-align: center;">
            <h3 style="margin-bottom: 15px; color: #ff6b6b;">⚙️ WiFi Settings</h3>
            <p style="margin-bottom: 15px; font-size: 0.9em; color: rgba(255,255,255,0.8);">
                Reset WiFi configuration to change network settings
            </p>
            <button id="reset-wifi-btn" onclick="resetWiFi()" 
                style="padding: 12px 20px; background: #ff4757; color: white; border: none; 
                       border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 16px;
                       transition: background 0.3s;">
                🔄 Reset WiFi Configuration
            </button>
            <div id="wifi-status" style="margin-top: 10px; font-size: 0.9em; display: none;"></div>
        </div>
        
        <!-- Color Picker Section -->
        <div class="color-section" id="color-section">
            <h3 style="text-align: center; margin-bottom: 15px;">Color Selection</h3>
            <div class="color-picker-wrapper">
                <input type="color" id="color-picker" class="color-input" value="#00ff00" onchange="setColor(this.value)">
                <span style="font-weight: bold;">Current Color</span>
            </div>
            <div class="color-presets">
                <div class="color-preset" style="background: #ff0000;" onclick="setColorPreset('#ff0000')"></div>
                <div class="color-preset" style="background: #00ff00;" onclick="setColorPreset('#00ff00')"></div>
                <div class="color-preset" style="background: #0000ff;" onclick="setColorPreset('#0000ff')"></div>
                <div class="color-preset" style="background: #ffff00;" onclick="setColorPreset('#ffff00')"></div>
                <div class="color-preset" style="background: #ff00ff;" onclick="setColorPreset('#ff00ff')"></div>
                <div class="color-preset" style="background: #00ffff;" onclick="setColorPreset('#00ffff')"></div>
                <div class="color-preset" style="background: #ffffff;" onclick="setColorPreset('#ffffff')"></div>
                <div class="color-preset" style="background: #ff8000;" onclick="setColorPreset('#ff8000')"></div>
            </div>
        </div>
        
        <!-- Drawing Section -->
        <div class="drawing-section" id="drawing-section">
            <h3 style="text-align: center; margin-bottom: 15px;">Drawing Grid (32x7)</h3>
            <div class="drawing-grid" id="drawing-grid">
                <!-- Grid will be populated by JavaScript -->
            </div>
            <div class="drawing-controls">
                <button class="draw-btn" onclick="clearGrid()">Clear All</button>
                <button class="draw-btn" onclick="fillGrid()">Fill All</button>
                <button class="draw-btn" onclick="drawBorder()">Draw Border</button>
            </div>
        </div>
        
        <div id="status" class="status"></div>
        
        <div class="device-info">
            Device IP: <span id="device-ip">%DEVICE_IP%</span><br>
            WiFi: <span id="wifi-ssid">%WIFI_SSID%</span>
        </div>
    </div>

    <script>
        function setAnimation(mode, name) {
            const status = document.getElementById('status');
            
            fetch('/setAnimation', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'animation=' + mode
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('current').textContent = name;
                status.textContent = 'Animation changed to: ' + name;
                status.className = 'status success';
                status.style.display = 'block';
                updateNxControlsVisibility(mode);
                setTimeout(() => {
                    status.style.display = 'none';
                }, 3000);
            })
            .catch(error => {
                status.textContent = 'Error: ' + error.message;
                status.className = 'status error';
                status.style.display = 'block';
                setTimeout(() => {
                    status.style.display = 'none';
                }, 5000);
            });
        }
        
        function changeSpeed(speed) {
            const speedValue = parseFloat(speed);
            document.getElementById('speed-display').textContent = speedValue.toFixed(1) + 'x';
            
            fetch('/setSpeed', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'speed=' + speedValue
            })
            .then(response => response.text())
            .then(data => {
                const status = document.getElementById('status');
                status.textContent = 'Animation speed updated to: ' + speedValue.toFixed(1) + 'x';
                status.className = 'status success';
                status.style.display = 'block';
                setTimeout(() => {
                    status.style.display = 'none';
                }, 2000);
                
                // Update selected preset button
                document.querySelectorAll('.speed-btn').forEach(btn => btn.classList.remove('selected'));
                if (speedValue === 0.5) document.querySelector('.speed-btn[onclick="setSpeed(0.5)"]').classList.add('selected');
                else if (speedValue === 1.0) document.querySelector('.speed-btn[onclick="setSpeed(1.0)"]').classList.add('selected');
                else if (speedValue === 2.0) document.querySelector('.speed-btn[onclick="setSpeed(2.0)"]').classList.add('selected');
            })
            .catch(error => {
                const status = document.getElementById('status');
                status.textContent = 'Error updating speed: ' + error.message;
                status.className = 'status error';
                status.style.display = 'block';
                setTimeout(() => {
                    status.style.display = 'none';
                }, 5000);
            });
        }
        
        function setSpeed(speed) {
            document.getElementById('speed-slider').value = speed;
            changeSpeed(speed);
        }
        
        function resetWiFi() {
            if (!confirm('Are you sure you want to reset WiFi configuration? The device will restart and start in configuration mode.')) {
                return;
            }
            
            const btn = document.getElementById('reset-wifi-btn');
            const status = document.getElementById('wifi-status');
            
            btn.disabled = true;
            btn.textContent = '🔄 Resetting...';
            btn.style.background = '#888';
            
            status.style.display = 'block';
            status.style.color = '#ffa500';
            status.textContent = 'Resetting WiFi configuration...';
            
            fetch('/resetWiFi', { method: 'POST' })
                .then(response => response.text())
                .then(message => {
                    status.style.color = '#4CAF50';
                    status.textContent = 'WiFi reset successful! Device is restarting...';
                    
                    setTimeout(() => {
                        status.textContent = 'Device should now start in configuration mode. Look for "NeoPixel-Setup" network.';
                    }, 3000);
                })
                .catch(error => {
                    console.error('WiFi reset failed:', error);
                    status.style.color = '#ff4757';
                    status.textContent = 'WiFi reset failed. Please try again.';
                    btn.disabled = false;
                    btn.textContent = '🔄 Reset WiFi Configuration';
                    btn.style.background = '#ff4757';
                });
        }
        
        function updateNxControlsVisibility(mode) {
            const colorSection = document.getElementById('color-section');
            const drawingSection = document.getElementById('drawing-section');
            
            // Hide all special sections first
            colorSection.style.display = 'none';
            drawingSection.style.display = 'none';
            
            if (mode === 9) { // ANIMATION_COLOR_PICKER mode
                colorSection.style.display = 'block';
            } else if (mode === 10) { // ANIMATION_DRAW_MODE
                colorSection.style.display = 'block';
                drawingSection.style.display = 'block';
                initializeDrawingGrid();
            }
        }
        
        // Function to update temperature display
        function updateTemperature() {
            fetch('/getTemperature')
                .then(response => response.json())
                .then(data => {
                    if (data.temperature !== -999.0) {
                        document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';
                    } else {
                        document.getElementById('temperature').textContent = 'Error';
                    }
                    if (data.humidity !== -999.0) {
                        document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                    } else {
                        document.getElementById('humidity').textContent = 'Error';
                    }
                })
                .catch(error => {
                    console.log('Temperature fetch failed:', error);
                    document.getElementById('temperature').textContent = 'N/A';
                    document.getElementById('humidity').textContent = 'N/A';
                });
        }
        
        setInterval(() => {
            fetch('/getAnimation')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('current').textContent = data.name;
                    updateNxControlsVisibility(data.mode);
                })
                .catch(error => console.log('Status refresh failed:', error));
                
            // Also update temperature
            updateTemperature();
        }, 5000);
        
        // Initial temperature update
        updateTemperature();
        
        // Color picker functions
        function setColor(color) {
            const status = document.getElementById('status');
            
            fetch('/setColor', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'color=' + encodeURIComponent(color)
            })
            .then(response => response.text())
            .then(data => {
                status.textContent = 'Color updated to: ' + color;
                status.className = 'status success';
                status.style.display = 'block';
                setTimeout(() => {
                    status.style.display = 'none';
                }, 2000);
            })
            .catch(error => {
                status.textContent = 'Error updating color: ' + error.message;
                status.className = 'status error';
                status.style.display = 'block';
                setTimeout(() => {
                    status.style.display = 'none';
                }, 5000);
            });
        }
        
        function setColorPreset(color) {
            document.getElementById('color-picker').value = color;
            setColor(color);
        }
        
        // Drawing grid functions
        let drawingGridData = [];
        let drawingGridColors = [];  // Store colors for each pixel
        let isDrawing = false;
        let drawMode = true; // true = draw, false = erase
        
        function initializeDrawingGrid() {
            const grid = document.getElementById('drawing-grid');
            if (grid.children.length > 0) return; // Already initialized
            
            grid.innerHTML = '';
            drawingGridData = [];
            drawingGridColors = [];
            
            for (let row = 0; row < 7; row++) {
                drawingGridData[row] = [];
                drawingGridColors[row] = [];
                for (let col = 0; col < 32; col++) {
                    drawingGridData[row][col] = false;
                    drawingGridColors[row][col] = '#00ff00';  // Default color
                    
                    const pixel = document.createElement('div');
                    pixel.className = 'grid-pixel';
                    pixel.dataset.row = row;
                    pixel.dataset.col = col;
                    
                    pixel.addEventListener('mousedown', (e) => {
                        e.preventDefault();
                        isDrawing = true;
                        const currentState = drawingGridData[row][col];
                        drawMode = !currentState;
                        togglePixel(col, row);
                    });
                    
                    pixel.addEventListener('mouseover', (e) => {
                        if (isDrawing) {
                            const currentState = drawingGridData[row][col];
                            if (currentState !== drawMode) {
                                togglePixel(col, row);
                            }
                        }
                    });
                    
                    grid.appendChild(pixel);
                }
            }
            
            document.addEventListener('mouseup', () => {
                isDrawing = false;
            });
        }
        
        function togglePixel(col, row) {
            drawingGridData[row][col] = !drawingGridData[row][col];
            const pixel = document.querySelector(`[data-row="${row}"][data-col="${col}"]`);
            if (drawingGridData[row][col]) {
                pixel.classList.add('active');
                // Store the current selected color for this pixel
                const currentColor = document.getElementById('color-picker').value;
                drawingGridColors[row][col] = currentColor;
                pixel.style.background = currentColor;
            } else {
                pixel.classList.remove('active');
                pixel.style.background = 'rgba(255, 255, 255, 0.1)';
            }
            
            // Send update to server
            updateDrawingPixel(col, row, drawingGridData[row][col]);
        }
        
        function updateDrawingPixel(col, row, state) {
            fetch('/setPixel', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `col=${col}&row=${row}&state=${state}`
            })
            .catch(error => console.log('Pixel update failed:', error));
        }
        
        function clearGrid() {
            for (let row = 0; row < 7; row++) {
                for (let col = 0; col < 32; col++) {
                    drawingGridData[row][col] = false;
                    const pixel = document.querySelector(`[data-row="${row}"][data-col="${col}"]`);
                    pixel.classList.remove('active');
                    pixel.style.background = 'rgba(255, 255, 255, 0.1)';
                }
            }
            
            fetch('/clearGrid', { method: 'POST' })
            .catch(error => console.log('Clear grid failed:', error));
        }
        
        function fillGrid() {
            const currentColor = document.getElementById('color-picker').value;
            for (let row = 0; row < 7; row++) {
                for (let col = 0; col < 32; col++) {
                    drawingGridData[row][col] = true;
                    drawingGridColors[row][col] = currentColor;
                    const pixel = document.querySelector(`[data-row="${row}"][data-col="${col}"]`);
                    pixel.classList.add('active');
                    pixel.style.background = currentColor;
                }
            }
            
            fetch('/fillGrid', { method: 'POST' })
            .catch(error => console.log('Fill grid failed:', error));
        }
        
        function drawBorder() {
            clearGrid();
            const currentColor = document.getElementById('color-picker').value;
            for (let row = 0; row < 7; row++) {
                for (let col = 0; col < 32; col++) {
                    if (row === 0 || row === 6 || col === 0 || col === 31) {
                        drawingGridData[row][col] = true;
                        drawingGridColors[row][col] = currentColor;
                        const pixel = document.querySelector(`[data-row="${row}"][data-col="${col}"]`);
                        pixel.classList.add('active');
                        pixel.style.background = currentColor;
                    }
                }
            }
            
            fetch('/drawBorder', { method: 'POST' })
            .catch(error => console.log('Draw border failed:', error));
        }
    </script>
</body>
</html>
)HTML";

void initWebServer() {
    webServer.on("/", handleRoot);
    webServer.on("/setAnimation", HTTP_POST, handleSetAnimation);
    
    webServer.on("/setSpeed", HTTP_POST, []() {
        if (webServer.hasArg("speed")) {
            float newSpeed = webServer.arg("speed").toFloat();
            
            if (newSpeed >= 0.2 && newSpeed <= 3.0) {
                animationSpeed = newSpeed;
                
                String response = "Animation speed set to: " + String(newSpeed, 1) + "x";
                webServer.send(200, "text/plain", response);
                
                Serial.print("Animation speed changed to: ");
                Serial.println(newSpeed);
            } else {
                webServer.send(400, "text/plain", "Invalid speed value (must be 0.2-3.0)");
            }
        } else {
            webServer.send(400, "text/plain", "Missing speed parameter");
        }
    });
    
    webServer.on("/getAnimation", HTTP_GET, []() {
        String response = "{\"mode\":" + String((int)currentAnimation) + 
                         ",\"name\":\"" + getAnimationName(currentAnimation) + 
                         "\"}";
        webServer.send(200, "application/json", response);
    });
    
    // Temperature endpoint
    webServer.on("/getTemperature", HTTP_GET, []() {
        String response = "{\"temperature\":" + String(currentTemperature, 1) + 
                         ",\"humidity\":" + String(currentHumidity, 1) + 
                         "}";
        webServer.send(200, "application/json", response);
    });
    
    // Color picker endpoint
    webServer.on("/setColor", HTTP_POST, []() {
        if (webServer.hasArg("color")) {
            String colorStr = webServer.arg("color");
            
            // Parse hex color string
            if (colorStr.startsWith("#")) {
                colorStr = colorStr.substring(1);
            }
            
            if (colorStr.length() == 6) {
                long colorValue = strtol(colorStr.c_str(), NULL, 16);
                // Keep RGB format - NeoPixel library will handle BGR conversion
                uint8_t r = (colorValue >> 16) & 0xFF;
                uint8_t g = (colorValue >> 8) & 0xFF;
                uint8_t b = colorValue & 0xFF;
                selectedColor = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
                
                String response = "Color set to: #" + colorStr;
                webServer.send(200, "text/plain", response);
                
                Serial.print("Color changed to: #");
                Serial.println(colorStr);
            } else {
                webServer.send(400, "text/plain", "Invalid color format");
            }
        } else {
            webServer.send(400, "text/plain", "Missing color parameter");
        }
    });
    
    // Drawing pixel endpoint
    webServer.on("/setPixel", HTTP_POST, []() {
        if (webServer.hasArg("col") && webServer.hasArg("row") && webServer.hasArg("state")) {
            int col = webServer.arg("col").toInt();
            int row = webServer.arg("row").toInt();
            bool state = webServer.arg("state") == "true";
            
            setDrawingPixel(col, row, state);
            webServer.send(200, "text/plain", "Pixel updated");
        } else {
            webServer.send(400, "text/plain", "Missing parameters");
        }
    });
    
    // Clear grid endpoint
    webServer.on("/clearGrid", HTTP_POST, []() {
        clearDrawingGrid();
        webServer.send(200, "text/plain", "Grid cleared");
    });
    
    // Fill grid endpoint
    webServer.on("/fillGrid", HTTP_POST, []() {
        for (int col = 0; col < 32; col++) {
            for (int row = 0; row < 7; row++) {
                drawingGrid[col][row] = selectedColor;  // Use current selected color
            }
        }
        needsGridUpdate = true;
        webServer.send(200, "text/plain", "Grid filled");
    });
    
    // Draw border endpoint
    webServer.on("/drawBorder", HTTP_POST, []() {
        clearDrawingGrid();
        for (int col = 0; col < 32; col++) {
            for (int row = 0; row < 7; row++) {
                if (row == 0 || row == 6 || col == 0 || col == 31) {
                    drawingGrid[col][row] = selectedColor;  // Use current selected color
                }
            }
        }
        needsGridUpdate = true;
        webServer.send(200, "text/plain", "Border drawn");
    });
    
    // WiFi reset endpoint
    webServer.on("/resetWiFi", HTTP_POST, []() {
        webServer.send(200, "text/plain", "WiFi configuration reset. Device will restart...");
        delay(1000);
        wifiConfigManager.resetConfig();
    });
    
    webServer.onNotFound(handleNotFound);
    
    webServer.begin();
    Serial.println("Web server started");
    Serial.print("Open http://");
    Serial.print(WiFi.localIP());
    Serial.println(" to control animations");
}

void handleWebServer() {
    webServer.handleClient();
}

void handleRoot() {
    Serial.println("handleRoot called");
    
    String html;
    html.reserve(16384); // Pre-allocate more memory for the large HTML
    
    // Read HTML from PROGMEM using proper method
    html = FPSTR(HTML_PAGE);
    
    Serial.print("HTML length: ");
    Serial.println(html.length());
    
    if (html.length() == 0) {
        Serial.println("ERROR: HTML_PAGE is empty!");
        html = "<html><body><h1>Error: HTML content not loaded</h1></body></html>";
    }
    
    html.replace("%DEVICE_IP%", WiFi.localIP().toString());
    html.replace("%WIFI_SSID%", WiFi.SSID());
    
    Serial.println("Sending HTML response");
    webServer.send(200, "text/html", html);
}

void handleSetAnimation() {
    if (webServer.hasArg("animation")) {
        int newMode = webServer.arg("animation").toInt();
        
        if (newMode >= 0 && newMode <= 10) {  // Updated to 0-10 range to include all animations
            currentAnimation = (AnimationMode)newMode;
            animationChanged = true;
            
            String response = "Animation set to: " + String(getAnimationName(currentAnimation));
            webServer.send(200, "text/plain", response);
            
            Serial.print("Animation changed via web interface to: ");
            Serial.println(getAnimationName(currentAnimation));
        } else {
            webServer.send(400, "text/plain", "Invalid animation mode");
        }
    } else {
        webServer.send(400, "text/plain", "Missing animation parameter");
    }
}

void handleNotFound() {
    webServer.send(404, "text/plain", "Page not found");
}

const char* getAnimationName(AnimationMode mode) {
    switch (mode) {
        case ANIMATION_TEMPERATURE: return "Temperature";
        case ANIMATION_QIX_LINES: return "Qix Lines";
        case ANIMATION_STARFIELD: return "Starfield";
        case ANIMATION_STAR_WARP: return "Star Warp";
        case ANIMATION_NEBULA: return "Nebula Swirl";
        case ANIMATION_RED: return "All Red (10%)";
        case ANIMATION_AUTO: return "Auto Cycle";
        case ANIMATION_MATRIX: return "Matrix Rain";
        case ANIMATION_FIRE: return "Fire Effect";
        case ANIMATION_COLOR_PICKER: return "Color Picker";
        case ANIMATION_DRAW_MODE: return "Draw Mode";
        default: return "Unknown";
    }
}
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define BUTTON_PIN 0
#define SIREN_PIN 14
#define LIGHT_OFF 0
#define LIGHT_TURNING_ON 1
#define LIGHT_ON 2
#define LIGHT_TURNING_OFF 3
#define AHT_SCL_PIN D1  // GPIO5
#define AHT_SDA_PIN D2  // GPIO4
#include "wifi-config-manager.h"
#include "ota-handler.h"
#include "wifi.h"
#include "webserver.h"

int buttonState = HIGH;
int lastButtonState = HIGH;
int ledState = LOW;
int ledStripPin = 13;
int ledStripNumpixels = 224;
int lightState = LIGHT_OFF; // 0 - off, 1 - turning on, 2 - on, 3 - turning off

int selectedPixelNumber = 0;
int pirs = 0;
int lights = 0;
int alarms = 0;
int speakerPin = SIREN_PIN;

unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;	// the debounce time; increase if the output flickers
unsigned long lastPir1Time = 0;
unsigned long lastMakeLight = 0;

Adafruit_NeoPixel myLedStrip(ledStripNumpixels, ledStripPin, NEO_BGR + NEO_KHZ800);

// Temperature sensor
Adafruit_AHTX0 aht;
bool ahtSensorAvailable = false;

// Animation speed multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
float animationSpeed = 1.0;

// Color picker and drawing variables
uint32_t selectedColor = 0x00FF00;  // Default to green
uint32_t drawingGrid[32][7];  // 32x7 matrix for pixel colors (0 = off, >0 = color)
bool needsGridUpdate = true;  // Flag to update grid display

// Global temperature variable for web interface
float currentTemperature = -999.0;
float currentHumidity = -999.0;

// Watchdog timer variables
Ticker secondTick;
bool watchdogFlag = false;
const unsigned long WATCHDOG_TIMEOUT_MS = 8000; // 8 seconds
unsigned long lastWatchdogFeed = 0;

// Function declarations
void feedWatchdog();
void animateQixLines(unsigned long durationMs);
void drawLine(float x1, float y1, float x2, float y2, uint32_t color);
void animateStarfield(unsigned long durationMs);
void animateStarWarp(unsigned long durationMs);
void animateNebula(unsigned long durationMs);
void animateAllRed(unsigned long durationMs);
void animateColorPicker(unsigned long durationMs);
void animateDrawMode(unsigned long durationMs);
void animateTemperature(unsigned long durationMs);
void displayTemperatureDigits(float temperature);
void displayTemperatureError();
void clearDrawingGrid();
void setDrawingPixel(int col, int row, bool state);

uint8_t activePixel = 0;
bool squareEffectDone = false;
bool racingEffectDone = false;
bool matrixEffectDone = false;
bool everyTenthDone = false;

// map column,row -> pixel index for a 32x7 matrix laid out in a serpentine (zig-zag) strip
int pixelIndex(int col, int row)
{
	const int cols = 32;
	// bounds safety
	if (col < 0) col = 0;
	if (col >= cols) col = cols - 1;
	if (row < 0) row = 0;
	if (row >= 7) row = 6;

	int base = row * cols;
	if ((row & 1) == 0)
	{
		// even rows go left->right
		return base + col;
	}
	else
	{
		// odd rows go right->left (serpentine)
		return base + (cols - 1 - col);
	}
}

// Helper function to draw a line using Bresenham's algorithm
void drawLine(float x1, float y1, float x2, float y2, uint32_t color)
{
	int ix1 = (int)x1;
	int iy1 = (int)y1;
	int ix2 = (int)x2;
	int iy2 = (int)y2;
	
	int dx = abs(ix2 - ix1);
	int dy = abs(iy2 - iy1);
	int sx = (ix1 < ix2) ? 1 : -1;
	int sy = (iy1 < iy2) ? 1 : -1;
	int err = dx - dy;
	
	int x = ix1;
	int y = iy1;
	
	while (true) {
		// Set pixel if within bounds
		if (x >= 0 && x < 32 && y >= 0 && y < 7) {
			int idx = pixelIndex(x, y);
			myLedStrip.setPixelColor(idx, color);
		}
		
		if (x == ix2 && y == iy2) break;
		
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x += sx;
		}
		if (e2 < dx) {
			err += dx;
			y += sy;
		}
	}
}

// Qix-style moving lines animation
void animateQixLines(unsigned long durationMs)
{
	static float line1X1 = 16.0, line1Y1 = 3.5;
	static float line1X2 = 24.0, line1Y2 = 2.0;
	static float line1DX1 = 0.3, line1DY1 = 0.2;
	static float line1DX2 = -0.4, line1DY2 = 0.3;
	
	static float line2X1 = 8.0, line2Y1 = 1.0;
	static float line2X2 = 12.0, line2Y2 = 5.5;
	static float line2DX1 = 0.5, line2DY1 = 0.1;
	static float line2DX2 = -0.2, line2DY2 = -0.4;
	
	static unsigned long lastUpdate = 0;
	
	// Update animation every 50ms (adjusted by speed)
	unsigned long updateInterval = (unsigned long)(50 / animationSpeed);
	if (millis() - lastUpdate >= updateInterval) {
		lastUpdate = millis();
		
		const int cols = 32;
		const int rows = 7;
		
		// Clear display
		myLedStrip.clear();
		
		// Update line 1 positions
		line1X1 += line1DX1;
		line1Y1 += line1DY1;
		line1X2 += line1DX2;
		line1Y2 += line1DY2;
		
		// Update line 2 positions
		line2X1 += line2DX1;
		line2Y1 += line2DY1;
		line2X2 += line2DX2;
		line2Y2 += line2DY2;
		
		// Bounce line 1 endpoints off boundaries
		if (line1X1 <= 0 || line1X1 >= cols-1) line1DX1 = -line1DX1;
		if (line1Y1 <= 0 || line1Y1 >= rows-1) line1DY1 = -line1DY1;
		if (line1X2 <= 0 || line1X2 >= cols-1) line1DX2 = -line1DX2;
		if (line1Y2 <= 0 || line1Y2 >= rows-1) line1DY2 = -line1DY2;
		
		// Bounce line 2 endpoints off boundaries
		if (line2X1 <= 0 || line2X1 >= cols-1) line2DX1 = -line2DX1;
		if (line2Y1 <= 0 || line2Y1 >= rows-1) line2DY1 = -line2DY1;
		if (line2X2 <= 0 || line2X2 >= cols-1) line2DX2 = -line2DX2;
		if (line2Y2 <= 0 || line2Y2 >= rows-1) line2DY2 = -line2DY2;
		
		// Draw line 1 (cyan)
		drawLine(line1X1, line1Y1, line1X2, line1Y2, myLedStrip.Color(0, 100, 100));
		
		// Draw line 2 (magenta)
		drawLine(line2X1, line2Y1, line2X2, line2Y2, myLedStrip.Color(100, 0, 100));
		
		// Disable interrupts briefly for clean NeoPixel update
		noInterrupts();
		myLedStrip.show();
		interrupts();
	}
	
	// Handle periodic tasks less frequently for better web performance
	if (millis() - lastUpdate >= 500) {
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// Starfield animation - stars moving from left to right at different speeds
void animateStarfield(unsigned long durationMs)
{
	static struct Star {
		float x, y;
		float speed;
		uint8_t brightness;
		bool active;
	} stars[16];
	
	static bool initialized = false;
	static unsigned long lastUpdate = 0;
	
	// Initialize stars
	if (!initialized) {
		for (int i = 0; i < 16; i++) {
			stars[i].active = false;
		}
		initialized = true;
		lastUpdate = millis();
	}
	
	// Update animation every 80ms (adjusted by speed)
	unsigned long updateInterval = (unsigned long)(80 / animationSpeed);
	if (millis() - lastUpdate >= updateInterval) {
		lastUpdate = millis();
		
		const int cols = 32;
		const int rows = 7;
		
		// Clear display
		myLedStrip.clear();
		
		// Create new stars randomly
		if (random(0, 100) < 30) {
			for (int i = 0; i < 16; i++) {
				if (!stars[i].active) {
					stars[i].x = -1.0;
					stars[i].y = random(0, rows);
					stars[i].speed = (0.2 + (random(0, 100) / 100.0) * 0.8) * animationSpeed; // Speed between 0.2 and 1.0, adjusted by speed
					stars[i].brightness = 50 + random(0, 150); // Brightness between 50 and 200
					stars[i].active = true;
					break;
				}
			}
		}
		
		// Update and draw stars
		for (int i = 0; i < 16; i++) {
			if (stars[i].active) {
				// Move star
				stars[i].x += stars[i].speed;
				
				// Check if star is still visible
				if (stars[i].x >= cols) {
					stars[i].active = false;
					continue;
				}
				
				// Draw star if within bounds
				if (stars[i].x >= 0 && stars[i].x < cols && stars[i].y >= 0 && stars[i].y < rows) {
					int ix = (int)stars[i].x;
					int iy = (int)stars[i].y;
					int idx = pixelIndex(ix, iy);
					
					// Create white star with varying brightness
					uint8_t b = stars[i].brightness;
					myLedStrip.setPixelColor(idx, myLedStrip.Color(b, b, b));
				}
			}
		}
		
		// Disable interrupts briefly for clean NeoPixel update
		noInterrupts();
		myLedStrip.show();
		interrupts();
	}
	
	// Handle periodic tasks less frequently for better web performance
	if (millis() - lastUpdate >= 500) {
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// Star Warp animation - first person perspective traveling through stars
void animateStarWarp(unsigned long durationMs)
{
	static struct WarpStar {
		float x, y; // Current position
		float startX, startY; // Starting position near center
		float dx, dy; // Direction vector
		float speed;
		uint8_t brightness;
		bool active;
		float life; // How far along the warp path (0.0 to 1.0)
	} warpStars[24];
	
	static bool initialized = false;
	static unsigned long lastUpdate = 0;
	
	// Initialize stars
	if (!initialized) {
		for (int i = 0; i < 24; i++) {
			warpStars[i].active = false;
		}
		initialized = true;
		lastUpdate = millis();
	}
	
	// Update animation every 60ms (adjusted by speed)
	unsigned long updateInterval = (unsigned long)(60 / animationSpeed);
	if (millis() - lastUpdate >= updateInterval) {
		lastUpdate = millis();
		
		const int cols = 32;
		const int rows = 7;
		const float centerX = cols / 2.0;
		const float centerY = rows / 2.0;
		
		// Clear display
		myLedStrip.clear();
		
		// Create new warp stars randomly
		if (random(0, 100) < 40) {
			for (int i = 0; i < 24; i++) {
				if (!warpStars[i].active) {
					// Start near center with slight random offset
					warpStars[i].startX = centerX + random(-2, 3) * 0.5;
					warpStars[i].startY = centerY + random(-1, 2) * 0.5;
					warpStars[i].x = warpStars[i].startX;
					warpStars[i].y = warpStars[i].startY;
					
					// Calculate direction vector from center outward
					float dx = random(-100, 101) / 50.0; // -2.0 to 2.0
					float dy = random(-100, 101) / 50.0; // -2.0 to 2.0
					
					// Favor horizontal movement due to rectangular display
					if (abs(dx) < 0.3) dx = (dx < 0) ? -0.8 : 0.8;
					
					warpStars[i].dx = dx;
					warpStars[i].dy = dy;
					warpStars[i].speed = (0.05 + (random(0, 50) / 100.0) * 0.1) * animationSpeed; // 0.05 to 0.15, adjusted by speed
					warpStars[i].brightness = 60 + random(0, 120); // 60 to 180
					warpStars[i].life = 0.0;
					warpStars[i].active = true;
					break;
				}
			}
		}
		
		// Update and draw warp stars
		for (int i = 0; i < 24; i++) {
			if (warpStars[i].active) {
				// Update life progress
				warpStars[i].life += warpStars[i].speed;
				
				// Check if star has reached end of life
				if (warpStars[i].life >= 1.0) {
					warpStars[i].active = false;
					continue;
				}
				
				// Calculate accelerating movement (starts slow, gets faster)
				float accel = warpStars[i].life * warpStars[i].life; // Quadratic acceleration
				warpStars[i].x = warpStars[i].startX + warpStars[i].dx * accel * 20;
				warpStars[i].y = warpStars[i].startY + warpStars[i].dy * accel * 20;
				
				// Draw star trail effect
				for (int trail = 0; trail < 3; trail++) {
					float trailLife = warpStars[i].life - trail * 0.02;
					if (trailLife <= 0) break;
					
					float trailAccel = trailLife * trailLife;
					float trailX = warpStars[i].startX + warpStars[i].dx * trailAccel * 20;
					float trailY = warpStars[i].startY + warpStars[i].dy * trailAccel * 20;
					
					// Check if trail point is within bounds
					if (trailX >= 0 && trailX < cols && trailY >= 0 && trailY < rows) {
						int ix = (int)trailX;
						int iy = (int)trailY;
						int idx = pixelIndex(ix, iy);
						
						// Create white star with fading trail
						uint8_t brightness = warpStars[i].brightness * (1.0 - trail * 0.4) * (1.0 - warpStars[i].life * 0.2);
						myLedStrip.setPixelColor(idx, myLedStrip.Color(brightness, brightness, brightness));
					}
				}
			}
		}
		
		// Disable interrupts briefly for clean NeoPixel update
		noInterrupts();
		myLedStrip.show();
		interrupts();
	}
	
	// Handle periodic tasks less frequently for better web performance
	if (millis() - lastUpdate >= 500) {
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// Nebula Swirl animation - rotating cosmic clouds with changing colors
void animateNebula(unsigned long durationMs)
{
	static unsigned long lastUpdate = 0;
	static float rotationAngle = 0.0;
	static float colorPhase = 0.0;
	
	// Update animation every 80ms (adjusted by speed)
	unsigned long updateInterval = (unsigned long)(80 / animationSpeed);
	if (millis() - lastUpdate >= updateInterval) {
		lastUpdate = millis();
		
		const int cols = 32;
		const int rows = 7;
		const float centerX = cols / 2.0;
		const float centerY = rows / 2.0;
		
		// Update rotation and color phase
		rotationAngle += 0.08 * animationSpeed; // Slow rotation adjusted by speed
		colorPhase += 0.05 * animationSpeed;    // Color cycling speed adjusted
		if (rotationAngle >= 2 * PI) rotationAngle -= 2 * PI;
		if (colorPhase >= 2 * PI) colorPhase -= 2 * PI;
		
		// Clear display
		myLedStrip.clear();
		
		// Draw nebula swirl
		for (int x = 0; x < cols; x++) {
			for (int y = 0; y < rows; y++) {
				// Calculate distance and angle from center
				float dx = x - centerX;
				float dy = y - centerY;
				float distance = sqrt(dx * dx + dy * dy);
				float angle = atan2(dy, dx);
				
				// Create rotating spiral arms
				float spiralAngle = angle + rotationAngle + distance * 0.3;
				float spiralValue = sin(spiralAngle * 3) + sin(spiralAngle * 2);
				
				// Add radial waves
				float radialWave = sin(distance * 0.8 + rotationAngle * 2);
				
				// Combine effects
			float intensity = (spiralValue + radialWave + 2.0f) / 4.0f;
			intensity = max(0.0f, min(1.0f, intensity));
			
			// Apply distance falloff
			float maxDist = max(centerX, centerY);
			float falloff = 1.0f - (distance / (maxDist * 1.5f));
			falloff = max(0.0f, min(1.0f, falloff));
				
				if (intensity > 0.1) {
					// Calculate color based on position and time
					float colorAngle = angle + colorPhase + distance * 0.2;
					
					// Create rainbow colors
					uint8_t r = (uint8_t)((sin(colorAngle) + 1) * 127 * intensity);
					uint8_t g = (uint8_t)((sin(colorAngle + 2.094) + 1) * 127 * intensity); // +2π/3
					uint8_t b = (uint8_t)((sin(colorAngle + 4.188) + 1) * 127 * intensity); // +4π/3
					
					// Apply some purple/pink bias for nebula feel
					r = (r * 2 + b) / 3;
					b = (b * 3 + r) / 4;
					
					int idx = pixelIndex(x, y);
					myLedStrip.setPixelColor(idx, myLedStrip.Color(r, g, b));
				}
			}
		}
		
		// Disable interrupts briefly for clean NeoPixel update
		noInterrupts();
		myLedStrip.show();
		interrupts();
	}
	
	// Handle periodic tasks less frequently for better web performance
	if (millis() - lastUpdate >= 600) {
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// All LEDs red at 10% brightness
void animateAllRed(unsigned long durationMs)
{
	static bool isInitialized = false;
	static unsigned long lastUpdate = 0;
	
	// Initialize the red display once
	if (!isInitialized) {
		Serial.println("Starting All Red animation - 10% brightness");
		delay(2);
		// Clear all LEDs first
		myLedStrip.clear();
		delay(2);
		// Set all LEDs to red at 10% brightness (25 out of 255)
		uint8_t redValue = 25;
		for (int i = 0; i < ledStripNumpixels; i++) {
			myLedStrip.setPixelColor(i, myLedStrip.Color(redValue, 0, 0));
		}
		
		// Disable interrupts briefly for clean NeoPixel update
		noInterrupts();
		myLedStrip.show();
		interrupts();
		
		// Small delay to ensure clean data transmission
		delay(10);
		isInitialized = true;
		lastUpdate = millis();
		Serial.println("All LEDs set to red (10% brightness)");
	}
	
	// Just maintain the display and handle periodic tasks
	if (millis() - lastUpdate >= 100) {
		lastUpdate = millis();
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
	
	// Reset initialization when switching away from red mode
	if (currentAnimation != ANIMATION_RED) {
		isInitialized = false;
		myLedStrip.clear();
		myLedStrip.show();
	}
}

// Matrix-style falling characters (green rain) across a 32x7 matrix
// Runs for approximately `durationMs` milliseconds; stepDelayMs controls animation speed
void animateMatrixRainMs(unsigned long durationMs, int stepDelayMs)
{
	static int head[32];
	static bool isInitialized = false;
	static unsigned long lastStep = 0;
	
	const int cols = 32;
	const int rows = 7;
	int tailLen = 3;
	
	// Initialize heads once
	if (!isInitialized) {
		for (int c = 0; c < cols; c++) head[c] = -random(1, rows + 1);
		isInitialized = true;
		lastStep = millis();
	}
	
	// Only update animation if enough time has passed
	if (millis() - lastStep >= (unsigned long)stepDelayMs) {
		lastStep = millis();
		
		myLedStrip.clear();

		for (int c = 0; c < cols; c++)
		{
			head[c]++;

			if (head[c] > rows + random(2, 4) && random(0, 100) < 40)
			{
				head[c] = -random(1, rows);
			}

			for (int t = 0; t < tailLen; t++)
			{
				int row = head[c] - t;
				if (row >= 0 && row < rows)
				{
					int idx = pixelIndex(c, row);
					// Add safety bounds check for pixel index
					if (idx >= 0 && idx < ledStripNumpixels) {
						if (t == 0)
						{
							myLedStrip.setPixelColor(idx, myLedStrip.Color(180, 255, 180));
						}
						else
						{
							uint8_t g = (uint8_t)max(0, 200 - t * 50);
							myLedStrip.setPixelColor(idx, myLedStrip.Color(0, g, 0));
						}
					}
				}
			}
		}

		myLedStrip.show();
	}
	
	// Handle other tasks frequently
	handleOTA();
	handleWebServer();
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
	
	// Reset initialization when switching away from matrix mode
	if (currentAnimation != ANIMATION_MATRIX && currentAnimation != ANIMATION_AUTO) {
		isInitialized = false;
	}
}

// Calm burning fire effect across a 32x7 matrix
void animateFireCalm(unsigned long durationMs, int stepDelayMs)
{
	static int heat[32][7];
	static bool isInitialized = false;
	static unsigned long lastStep = 0;
	
	const int cols = 32;
	const int rows = 7;
	
	// Initialize heat map once
	if (!isInitialized) {
		for (int c = 0; c < cols; c++) {
			for (int r = 0; r < rows; r++) {
				heat[c][r] = 0;
			}
		}
		isInitialized = true;
		lastStep = millis();
	}
	
	// Only update animation if enough time has passed
	if (millis() - lastStep >= (unsigned long)stepDelayMs) {
		lastStep = millis();
		
		// cool down and propagate upward
		for (int c = 0; c < cols; c++)
		{
			// random ignition at the bottom
			if (random(0, 100) < 20)
			{
				// smaller ignition bursts to reduce overall brightness
				int add = (int)random(50, 140);
				int v = heat[c][0] + add;
				if (v > 255) v = 255;
				heat[c][0] = v;
			}

			// propagate upwards with some decay
			for (int r = rows - 1; r > 0; r--)
			{
				int a = heat[c][r - 1];
				int b = (r > 1) ? heat[c][r - 2] : 0;
				int val = (a + b) / 2;
				// a little random flicker
				{
					int dec = (int)random(0, 30);
					val = max(0, val - dec);
				}
				heat[c][r] = val;
			}

			// small decay at the bottom
			{
				int dec2 = (int)random(40, 100);
				heat[c][0] = max(0, heat[c][0] - dec2);
			}
		}

		// draw heatmap to LEDs
		myLedStrip.clear();
		for (int c = 0; c < cols; c++)
		{
			for (int r = 0; r < rows; r++)
			{
				int h = heat[c][r];
				// Manual bounds checking instead of constrain
				if (h < 0) h = 0;
				if (h > 255) h = 255;
				
				// dimmer color mapping to lower overall brightness
				uint8_t rcol = (uint8_t)min(255, h / 1);
				uint8_t gcol = (uint8_t)min(255, h / 2);
				uint8_t bcol = (uint8_t)min(60, h / 12);
				int idx = pixelIndex(c, r);
				myLedStrip.setPixelColor(idx, myLedStrip.Color(rcol, gcol, bcol));
			}
		}

		myLedStrip.show();
	}
	
	// Handle other tasks frequently
	handleOTA();
	handleWebServer();
	feedWatchdog();
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
	
	// Reset initialization when switching away from fire mode
	if (currentAnimation != ANIMATION_FIRE && currentAnimation != ANIMATION_AUTO) {
		isInitialized = false;
	}
}

// Color picker mode - displays selected color on entire matrix
void animateColorPicker(unsigned long durationMs)
{
	static unsigned long lastUpdate = 0;
	
	// Update display every 100ms
	if (millis() - lastUpdate >= 100) {
		lastUpdate = millis();
		
		// Fill entire matrix with selected color
		myLedStrip.clear();
		for (int i = 0; i < ledStripNumpixels; i++) {
			myLedStrip.setPixelColor(i, selectedColor);
		}
		
		myLedStrip.show();
	}
	
	// Handle other tasks
	handleOTA();
	handleWebServer();
	feedWatchdog();
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// Drawing mode - displays user-drawn image
void animateDrawMode(unsigned long durationMs)
{
	static unsigned long lastUpdate = 0;
	
	// Update display when grid changes or every 500ms for tasks
	if (needsGridUpdate || millis() - lastUpdate >= 500) {
		lastUpdate = millis();
		
		if (needsGridUpdate) {
			myLedStrip.clear();
			
// Draw pixels based on drawing grid with their stored colors
            for (int col = 0; col < 32; col++) {
                for (int row = 0; row < 7; row++) {
                    if (drawingGrid[col][row] != 0) {
                        int idx = pixelIndex(col, row);
                        myLedStrip.setPixelColor(idx, drawingGrid[col][row]);
					}
				}
			}
			
			myLedStrip.show();
			needsGridUpdate = false;
		}
	}
	
	// Handle other tasks
	handleOTA();
	handleWebServer();
	feedWatchdog();
	
	// Check for animation change for better responsiveness
	if (animationChanged) {
		return; // Exit immediately if animation changed
	}
}

// Clear the drawing grid
void clearDrawingGrid()
{
	for (int col = 0; col < 32; col++) {
		for (int row = 0; row < 7; row++) {
			drawingGrid[col][row] = 0;  // 0 = off
		}
	}
	needsGridUpdate = true;
}

// Set or clear a pixel in the drawing grid
void setDrawingPixel(int col, int row, bool state)
{
	if (col >= 0 && col < 32 && row >= 0 && row < 7) {
		drawingGrid[col][row] = state ? selectedColor : 0;  // Store color or 0 for off
		needsGridUpdate = true;
	}
}

// Watchdog callback function
void ISRwatchdog() {
	watchdogFlag = true;
}

// Feed the watchdog timer
void feedWatchdog() {
	lastWatchdogFeed = millis();
	watchdogFlag = false;
	ESP.wdtFeed();
}

// Temperature animation - display temperature from AHT10 sensor
void animateTemperature(unsigned long durationMs)
{
	static unsigned long lastUpdate = 0;
	static float lastTemperature = -999.0;
	
	unsigned long now = millis();
	if (now - lastUpdate >= durationMs) {
		lastUpdate = now;
		
		if (ahtSensorAvailable) {
			sensors_event_t humidity, temp;
			if (aht.getEvent(&humidity, &temp)) {
				float temperature = temp.temperature;
				
				// Only update display if temperature changed significantly or first reading
				if (abs(temperature - lastTemperature) > 0.1 || lastTemperature == -999.0) {
					lastTemperature = temperature;
					currentTemperature = temperature;  // Update global variable
					currentHumidity = humidity.relative_humidity;  // Update global humidity
					Serial.print("Temperature: ");
					Serial.print(temperature);
					Serial.println(" °C");
					
					displayTemperatureDigits(temperature);
				}
			} else {
				Serial.println("Failed to read from AHT10 sensor");
				displayTemperatureError();
			}
		} else {
			displayTemperatureError();
		}
	}
	
	// Handle periodic tasks
	if (millis() - lastUpdate >= 100) {
		handleOTA();
		handleWebServer();
		feedWatchdog();
	}
}

// Display temperature as digits on LED matrix
void displayTemperatureDigits(float temperature)
{
	myLedStrip.clear();
	
	// Round temperature to 1 decimal place
	int temp10 = (int)(temperature * 10 + 0.5);
	int digit1 = (temp10 / 100) % 10;  // tens
	int digit2 = (temp10 / 10) % 10;   // units
	int digit3 = temp10 % 10;          // decimal
	
	uint32_t digitColor = myLedStrip.Color(0, 255, 100); // Green-cyan color
	uint32_t decimalColor = myLedStrip.Color(255, 100, 0); // Orange for decimal point
	
	// Define 7-segment display patterns for digits (5x7 matrix)
	const bool digits[10][5][7] = {
		// 0
		{{0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {1,0,0,0,1,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,0,0,0}},
		// 1
		{{0,0,1,0,0,0,0}, {0,1,1,0,0,0,0}, {0,0,1,0,0,0,0}, {0,0,1,0,0,0,0}, {0,1,1,1,0,0,0}},
		// 2
		{{0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,0,1,1,0,0,0}, {0,1,0,0,0,0,0}, {1,1,1,1,1,0,0}},
		// 3
		{{0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,0,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,0,0,0}},
		// 4
		{{1,0,0,1,0,0,0}, {1,0,0,1,0,0,0}, {1,1,1,1,1,0,0}, {0,0,0,1,0,0,0}, {0,0,0,1,0,0,0}},
		// 5
		{{1,1,1,1,1,0,0}, {1,0,0,0,0,0,0}, {1,1,1,1,0,0,0}, {0,0,0,0,1,0,0}, {1,1,1,1,0,0,0}},
		// 6
		{{0,1,1,1,0,0,0}, {1,0,0,0,0,0,0}, {1,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,0,0,0}},
		// 7
		{{1,1,1,1,1,0,0}, {0,0,0,0,1,0,0}, {0,0,0,1,0,0,0}, {0,0,1,0,0,0,0}, {0,0,1,0,0,0,0}},
		// 8
		{{0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,0,0,0}},
		// 9
		{{0,1,1,1,0,0,0}, {1,0,0,0,1,0,0}, {0,1,1,1,1,0,0}, {0,0,0,0,1,0,0}, {0,1,1,1,0,0,0}}
	};
	
	// Display digits: tens at x=2-6, units at x=10-14, decimal at x=18-22
	// Tens digit (if not zero or if temperature >= 10)
	if (digit1 > 0 || temp10 >= 100) {
		for (int y = 0; y < 5; y++) {
			for (int x = 0; x < 5; x++) {
				if (digits[digit1][x][y]) {
					int pixelX = 2 + x;
					int pixelY = 1 + y;
					if (pixelX < 32 && pixelY < 7) {
						myLedStrip.setPixelColor(pixelIndex(pixelX, pixelY), digitColor);
					}
				}
			}
		}
	}
	
	// Units digit
	for (int y = 0; y < 5; y++) {
		for (int x = 0; x < 5; x++) {
			if (digits[digit2][x][y]) {
				int pixelX = 9 + x;
				int pixelY = 1 + y;
				if (pixelX < 32 && pixelY < 7) {
					myLedStrip.setPixelColor(pixelIndex(pixelX, pixelY), digitColor);
				}
			}
		}
	}
	
	// Decimal point
	myLedStrip.setPixelColor(pixelIndex(15, 5), decimalColor);
	
	// Decimal digit
	for (int y = 0; y < 5; y++) {
		for (int x = 0; x < 5; x++) {
			if (digits[digit3][x][y]) {
				int pixelX = 17 + x;
				int pixelY = 1 + y;
				if (pixelX < 32 && pixelY < 7) {
					myLedStrip.setPixelColor(pixelIndex(pixelX, pixelY), digitColor);
				}
			}
		}
	}
	
	// Display "°C" at the end
	// Simple ° symbol at x=24, C at x=26-28
	myLedStrip.setPixelColor(pixelIndex(24, 1), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(25, 1), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(24, 2), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(25, 2), myLedStrip.Color(255, 255, 255));
	
	// C letter
	myLedStrip.setPixelColor(pixelIndex(27, 1), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(28, 1), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(29, 1), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(27, 2), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(27, 3), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(27, 4), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(27, 5), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(28, 5), myLedStrip.Color(255, 255, 255));
	myLedStrip.setPixelColor(pixelIndex(29, 5), myLedStrip.Color(255, 255, 255));
	
	myLedStrip.show();
}

// Display red wavy line when temperature reading fails
void displayTemperatureError()
{
	static unsigned long lastWaveUpdate = 0;
	static int waveOffset = 0;
	
	if (millis() - lastWaveUpdate >= 200) {
		lastWaveUpdate = millis();
		waveOffset = (waveOffset + 1) % 32;
		
		myLedStrip.clear();
		
		// Create a red wavy line across the middle
		for (int x = 0; x < 32; x++) {
			int y = 3 + (int)(sin((x + waveOffset) * 0.5) * 1.5); // Wave oscillates around y=3
			if (y >= 0 && y < 7) {
				myLedStrip.setPixelColor(pixelIndex(x, y), myLedStrip.Color(255, 0, 0));
			}
		}
		
		myLedStrip.show();
	}
}

void setup()
{
	Serial.begin(115200);

	// Initialize watchdog timer
	ESP.wdtDisable();
	ESP.wdtEnable(WDTO_8S); // 8 second timeout
	secondTick.attach(1, ISRwatchdog); // Check every second
	feedWatchdog();
	Serial.println("Watchdog timer initialized");

	myLedStrip.begin(); // This initializes the NeoPixel library.
	myLedStrip.clear();
	

	lastDebounceTime = millis();
	Serial.println("Booting");

	pinMode(BUTTON_PIN, INPUT);

	connectWifi();
	initWebServer();
	pinMode(ledStripPin, OUTPUT);

	delay(2);

	for (int i = 1; i < ledStripNumpixels; i++)
	{

		Serial.print(i);
		Serial.print(" ");

		myLedStrip.setPixelColor(i, myLedStrip.Color(255, 0, 0));
		myLedStrip.show();
		delay(10);
		myLedStrip.setPixelColor(i, myLedStrip.Color(0, 0, 0));
	}
	delay(10);
	myLedStrip.show();

	Serial.println("pixend");
		// Initialize I2C for AHT10 sensor
	Wire.begin(AHT_SDA_PIN, AHT_SCL_PIN);
	delay(100);
	// Initialize AHT10 temperature sensor
	if (aht.begin()) {
		ahtSensorAvailable = true;
		Serial.println("AHT10 temperature sensor initialized successfully");
	} else {
		ahtSensorAvailable = false;
		Serial.println("AHT10 temperature sensor not found - temperature animation will show error pattern");
	}

}

void loop()
{
	// Check and feed watchdog
	if (watchdogFlag || (millis() - lastWatchdogFeed > 5000)) {
		feedWatchdog();
	}

	// Handle WiFi reconnection if connection is lost
	handleWiFiReconnection();
	
	// Handle WiFi configuration button (hold for 3 seconds to reset WiFi settings)
	handleWiFiConfigButton();

	handleOTA();
	handleWebServer();

	// Handle animation mode changes
	static AnimationMode lastAnimation = ANIMATION_TEMPERATURE;
	static unsigned long lastAnimationStart = 0;
	static int autoAnimationIndex = 0;
	const unsigned long AUTO_ANIMATION_DURATION = 10000; // 10 seconds per animation

	// Check if animation was changed via web interface
	if (animationChanged || currentAnimation != lastAnimation) {
		animationChanged = false;
		lastAnimation = currentAnimation;
		lastAnimationStart = millis();
		autoAnimationIndex = 0;
		myLedStrip.clear();
		myLedStrip.show();
		Serial.print("Animation mode changed to: ");
		Serial.println(getAnimationName(currentAnimation));
	}

	// Run animations based on current mode
	switch (currentAnimation) {
		case ANIMATION_TEMPERATURE:
			animateTemperature(1000); // Update temperature every second
			break;
			
		case ANIMATION_QIX_LINES:
			animateQixLines(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_STARFIELD:
			animateStarfield(50); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_STAR_WARP:
			animateStarWarp(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_NEBULA:
			animateNebula(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_RED:
			animateAllRed(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_MATRIX:
			animateMatrixRainMs(250L, (int)(90 / animationSpeed)); // Shorter duration
			break;
			
		case ANIMATION_FIRE:
			animateFireCalm(250UL, (int)(80 / animationSpeed)); // Shorter duration
			break;
			
		case ANIMATION_COLOR_PICKER:
			animateColorPicker(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_DRAW_MODE:
			animateDrawMode(250); // Shorter duration for better responsiveness
			break;
			
		case ANIMATION_AUTO:
		default:
			// Auto cycle through animations starting with temperature
			if (millis() - lastAnimationStart >= AUTO_ANIMATION_DURATION) {
				lastAnimationStart = millis();
				autoAnimationIndex = (autoAnimationIndex + 1) % 6;
				myLedStrip.clear();
				myLedStrip.show();
				Serial.print("Auto switching to animation: ");
				Serial.println(autoAnimationIndex);
			}
			
			// Run current auto animation
			switch (autoAnimationIndex) {
				case 0:
					animateTemperature(1000);  // Start auto cycle with temperature
					break;
				case 1:
					animateStarfield(50);
					break;
				case 2:
					animateStarWarp(50);
					break;
				case 3:
					animateNebula(50);
					break;
				case 4:
					animateMatrixRainMs(50L, (int)(90 / animationSpeed));
					break;
				case 5:
					animateFireCalm(50UL, (int)(80 / animationSpeed));
					break;
			}
			break;
	}

	// Small delay for responsiveness
	delay(10);
}

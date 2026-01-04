// webserver.h - Web server for animation control
#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>

// Animation modes
enum AnimationMode {
    ANIMATION_TEMPERATURE = 0,  // Temperature display from AHT10 sensor - FIRST IN AUTO CYCLE
    ANIMATION_QIX_LINES,        // Qix-style moving lines
    ANIMATION_STARFIELD,        // Starfield/Stars screen saver effect
    ANIMATION_STAR_WARP,        // First-person star warp travel effect
    ANIMATION_NEBULA,           // Rotating nebula swirl with changing colors
    ANIMATION_RED,              // All LEDs red at 10% brightness
    ANIMATION_AUTO,             // Cycle through all animations automatically
    ANIMATION_MATRIX,           // Matrix rain effect
    ANIMATION_FIRE,             // Fire effect
    ANIMATION_COLOR_PICKER,     // Color picker mode - solid color display
    ANIMATION_DRAW_MODE         // Drawing mode - pixel by pixel drawing
};

extern ESP8266WebServer webServer;
extern volatile AnimationMode currentAnimation;
extern volatile bool animationChanged;
extern float animationSpeed;

// Color picker and drawing variables
extern uint32_t selectedColor;
extern uint32_t drawingGrid[32][7];  // 32x7 matrix for pixel colors (0 = off, >0 = color)
extern bool needsGridUpdate;

// Temperature variables
extern float currentTemperature;
extern float currentHumidity;

void initWebServer();
void handleWebServer();
void handleRoot();
void handleSetAnimation();
void handleNotFound();

// Drawing functions
void clearDrawingGrid();
void setDrawingPixel(int col, int row, bool state);

const char* getAnimationName(AnimationMode mode);
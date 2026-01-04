# esp8266_NeoPixelString_balcony

This project runs colorful lighting effects on a 224-LED NeoPixel strip driven by an ESP8266.
author: jaresz https://github.com/jaresz

Overview
- LED strip: 224 x WS2812/NeoPixel LEDs
- Matrix layout: 32 columns x 7 rows (32x7) using a serpentine (zig-zag) wiring order
	- Top-left is LED 0, top-right is LED 31
	- Row 2 ends at LED 32 on the right (serpentine layout alternates direction each row)

Animations
- Starfield: moving stars crossing the matrix from left to right.
- Star Warp: first-person perspective warp-through-stars effect.
- Nebula Swirl: rotating colorful nebula/clouds with smooth color shifts.
- Matrix Rain: green "falling characters" (Matrix-style) across the 32x7 matrix.
- Calm Fire: a low-brightness, calm burning fire effect.

These animations are cycled automatically when the device is set to the automatic mode (each runs for approximately 10 seconds).
Wiring
- Data (middle wire) -> digital pin 13 (GPIO13) on the board (NodeMCU label: D7)
- +5V (power) -> LED V+
- GND -> LED GND
- Important: connect ESP8266 GND and LED strip GND together (common ground)

ASCII wiring diagram (logical):

	ESP8266/NodeMCU            LED Strip (WS2812)
	----------------            ------------------
	[USB / 5V supply]---+-------+ V+ (+5V)
											|       +-----------
											|       |  LED data -> (middle wire) -> `D7` (GPIO13)
											|       +-----------
								GND --+------------------> GND

Notes
- The data pin used in code is `ledStripPin = 13` and the LED count is `ledStripNumpixels = 224` (see `src/main.cpp`).
- Power: 224 NeoPixels can draw significant current at high brightness. Use a capable 5V power supply and avoid powering the strip from the ESP alone.

Build & Upload
- This project is a PlatformIO/Arduino project. Build and upload with PlatformIO in VS Code or run:

```bash
platformio run --target upload
```




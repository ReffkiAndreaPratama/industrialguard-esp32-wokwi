# IndustrialGuard Professional — ESP32 / Wokwi

Advanced+ industrial condition monitoring demo with a professional project structure and operational web dashboard. **Educational simulator only**, not a certified safety controller or actual predictive-maintenance model.

## Features
- DHT22 temperature/humidity telemetry
- Potentiometer-based vibration, current/load, and RPM simulation
- Three-level anomaly assessment: NORMAL / WARNING / CRITICAL
- Explainable heuristic machine-health score (0–100)
- AUTO critical-shutdown simulation via relay output, MANUAL motor control
- Three LED status indicators, buzzer alarm patterns, OLED status screen
- Physical MUTE and AUTO/MANUAL buttons
- Live HTTP dashboard, JSON API, recent event timeline, sample/alert counters
- Serial logging (115200 baud)

## Wokwi Web Editor (recommended)
1. Extract ZIP.
2. Create a new **ESP32** project at https://wokwi.com/.
3. Replace `diagram.json` and `sketch.ino` with the supplied files.
4. In Library Manager, add `Adafruit SSD1306`, `Adafruit GFX Library`, and `DHT sensor library for ESPx` (DHTesp).
5. Start simulation, open Serial Monitor, click DHT22 to adjust temperature and rotate potentiometers to simulate machine conditions.
6. The OLED, LEDs, relay indicator, and buzzer will respond.

There is no universal import-ZIP button in every Wokwi Web Editor workflow. The reliable route is to paste the diagram and sketch.

## Dashboard
HTTP server on port 80. The included `wokwi.toml` maps localhost:8180 → target:80 for Wokwi **Private IoT Gateway** in supported VS Code workflows. Open `http://localhost:8180` when the gateway is active. The dashboard may be inaccessible from a plain web-editor simulation without the gateway.

## Pin map
| Device | Wokwi pin | ESP32 pin |
|---|---|---|
| DHT22 | SDA | GPIO4 |
| Vibration potentiometer | SIG | GPIO34 |
| Current/load potentiometer | SIG | GPIO35 |
| RPM potentiometer | SIG | GPIO32 |
| OLED | SDA / SCL | GPIO21 / GPIO22 |
| Green LED | A | GPIO25 |
| Yellow LED | A | GPIO26 |
| Red LED | A | GPIO27 |
| Buzzer | terminal 2 | GPIO19 |
| Relay module | IN | GPIO18 |
| MUTE pushbutton | 1.l | GPIO23 |
| MODE pushbutton | 1.l | GPIO13 |

All LED cathodes are wired through independent 220Ω resistors to GND. Each pushbutton connects GPIO to GND with INPUT_PULLUP. All sensors/potentiometers/OLED use 3V3/GND. Relay module uses 5V/GND. This relay output is an indicator only; no mains load or motor is attached.

## Risk engine
- Temperature WARNING ≥38°C, CRITICAL ≥48°C.
- Vibration WARNING ≥55%, CRITICAL ≥80%.
- Simulated load WARNING ≥65%, CRITICAL ≥85%.
- Any critical threshold causes CRITICAL status. In AUTO, the simulated motor is stopped (relay output LOW). Restart is blocked in AUTO while CRITICAL persists.
- Health score = clamp(100 − max(0, temp−30) − 0.35×vibration − 0.30×load, 0, 100).
- RPM is an analog simulator control; the simulated RPM drops to zero when motor is stopped.
- These are demonstration heuristics, not a trained predictive maintenance algorithm.

## API
- `GET /` responsive dashboard
- `GET /api/status` JSON machine telemetry, events, counters
- `GET /api/control?mode=auto` or `?mode=manual`
- `GET /api/control?motor=start` or `?motor=stop`
- `GET /api/control?mute=toggle`
- `GET /api/control?test=alarm`
- A start request in AUTO+CRITICAL returns HTTP 409.

## PlatformIO
`src/main.cpp` mirrors `sketch.ino`; open this extracted directory in VS Code + PlatformIO. The `platformio.ini` declares third-party dependencies. `wokwi.toml` points to the default PlatformIO build outputs. Do not compile both sketch.ino and src/main.cpp as separate translation units in one project; Arduino Web Editor uses sketch.ino, PlatformIO uses src/main.cpp.

## Physical deployment notes
Potentiometers are analog proxies and are **not** industrial vibration/current/RPM sensors. A physical build needs calibrated industrial-rated sensors, isolation, proper relay driver and protection, watchdogs, interlocks, emergency stop, and a risk assessment. ESP32 GPIOs must not be connected directly to mains or high-voltage motor circuits. Never rely on this demo for machine safety.

## Validation
JSON syntax, component IDs, firmware GPIO mapping, source parity, and ZIP integrity were checked. Live Wokwi compilation/runtime testing was **not** performed here.

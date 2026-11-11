# Architecture

```text
DHT22 + vibration/load/RPM analog proxies
          ↓
 ESP32 sampling (1 Hz)
          ↓
 rule-based anomaly classification + health score
          ↓
 relay motor indicator / LEDs / buzzer / OLED
          ↓
 HTTP JSON API → browser dashboard + event timeline
```

## Modules (logical, single-file Web Editor compatibility)
- `Pins` and `Config` namespaces: hardware and thresholds
- `sampleSensors`: acquisition, validation and health evaluation
- `outputs`: actuators and alarm patterns
- `updateOled`: local visualization
- `statusJson` / `handleControl`: HTTP API
- `buttons`: physical control interface

The single-file firmware makes it compatible with Wokwi Web Editor while preserving distinct logical modules. PlatformIO uses an identical `src/main.cpp`.

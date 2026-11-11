# Manual simulation test plan

1. Start with DHT22 at 25°C, vibration/load pots low. Expect NORMAL and green LED.
2. Raise vibration potentiometer above 55%. Expect WARNING and yellow LED.
3. Raise vibration above 80%. Expect CRITICAL, red LED, repeating buzzer, relay OFF in AUTO.
4. In CRITICAL+AUTO, request `motor=start`. Expect HTTP 409.
5. Switch to MANUAL; start simulated motor. Verify relay changes state.
6. Press MUTE button. Verify buzzer silences while LEDs remain active.
7. Press MODE button. Verify mode toggles and event is logged.
8. Raise DHT22 temperature above 48°C. Expect CRITICAL independent of vibration.
9. Rotate RPM pot while motor running. Verify simulated RPM updates.
10. Verify JSON `/api/status` and dashboard refresh using Wokwi Private IoT Gateway.

These are recommended tests, not claims of completed live simulation.

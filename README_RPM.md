# ESP Motor Tester - RPM Measurement

## Overview
This project includes high-speed RPM measurement functionality using an interrupt-based approach optimized for motors up to 21,000 RPM.

## Hardware Setup
- **RPM Sensor Pin**: D6 (GPIO12)
- **Connection**: Connect your RPM sensor signal wire to pin D6
- **Pull-up**: The pin is configured with internal pull-up resistor
- **Trigger**: Rising edge detection (LOW to HIGH transition)
- **Max Speed**: Supports up to 21,000 RPM (350 Hz signal frequency)

## Features Implemented

### RPMCounter Class
- **High-speed interrupt-based signal detection** on pin D6
- **Ultra-fast debouncing** with 0.5ms (500µs) minimum interval between valid signals
- **Microsecond precision** timing for accurate high-speed measurement
- **Serial output** for each detected signal with RPM calculation
- **Signal counting** and timing measurement
- **RPM validation** with sanity checks for noise detection
- **Current RPM calculation** with stale data detection

### Serial Output
When a signal is detected, the serial monitor will show:
```
RPM Signal detected! Count: 5, Time: 15432, Time between signals: 3 ms, RPM: 20000.0
```

## Performance Specifications
- **Maximum RPM**: 21,000+ (350+ Hz) - optimized for high-speed measurement
- **Minimum signal interval**: 2.86ms at max speed
- **Debounce time**: 0.5ms (500µs)
- **Timing precision**: Microsecond level
- **Update rate**: Real-time via optimized interrupts
- **Serial output**: Limited to 1Hz max to avoid performance impact
- **Web updates**: Live 5-second refresh with toggle control

### Performance Improvements
- **Optimized ISR**: No serial printing in interrupt handler for maximum speed
- **Real-time calculation**: RPM calculated directly in ISR and stored in global variable
- **Minimal overhead**: Only prints to serial once per second maximum
- **Live web updates**: JavaScript-based live RPM display with toggle control
- **REST API**: `/api/rpm` endpoint for real-time data access

### Web Interface Features
The web interface now includes:
- **Live RPM display** with 5-second auto-refresh (toggleable)
- Real-time signal count updates
- RPM sensor status and pin information
- Last signal timestamp with live updates
- Debounce timing information
- **REST API endpoint** at `/api/rpm` returning JSON data

### API Endpoint
**GET /api/rpm** returns:
```json
{
  "rpm": 1500.5,
  "signalCount": 1234,
  "lastSignalTime": 45678,
  "timeBetweenSignals": 40,
  "timestamp": 123456789
}
```

## Code Structure

### Files Added/Modified
- `src/RPMCounter.h` - RPM counter class header
- `src/RPMCounter.cpp` - RPM counter implementation  
- `src/main.cpp` - Integration with main program
- `src/WebServer.cpp` - Web interface updates

### Key Functions
- `RPMCounter::begin(pin)` - Initialize RPM counter on specified pin
- `RPMCounter::update()` - Process pending signals (minimal overhead)
- `RPMCounter::handleSignal()` - Optimized ISR function (called automatically)
- `RPMCounter::getCurrentRPM()` - Get current RPM from cached value
- `RPMCounter::getTimeBetweenSignals()` - Get last signal interval

## Usage

1. **Upload the firmware** to your ESP8266
2. **Connect your RPM sensor** to pin D6
3. **Monitor serial output** at 115200 baud to see signal detection
4. **View web interface** at `http://esp-racepi-motor-tester.local`

## Signal Processing
- **Debounce time**: 0.5ms (500µs) for high-speed motors
- **Edge detection**: Rising edge (LOW to HIGH)
- **ISR protection**: Uses `IRAM_ATTR` for interrupt service routine
- **Timing**: Microsecond precision using `micros()` function
- **Noise filtering**: RPM validation with sanity checks
- **Signal counting**: Persistent counter increments on each valid signal

## Next Steps
- Add web endpoint for RPM data
- Implement RPM history and averaging
- Add configurable parameters (debounce time, signals per revolution)
- Create real-time RPM display on web interface
- Add data logging capabilities

## Testing
You can test the RPM counter by:
1. Connecting a button or switch to D6 and GND
2. Pressing/releasing to generate rising edge signals
3. Observing serial output for signal detection
4. Checking the web interface for updated signal counts
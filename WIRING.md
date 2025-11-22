# Controller Wiring Guide

## ESP32 Controller to nRF24L01+ Connections

| nRF24L01+ Pin | ESP32 Pin | Function | Description |
|---------------|-----------|----------|-------------|
| VCC | 3.3V | Power | 3.3V power supply |
| GND | GND | Ground | Common ground |
| CE | GPIO5 | Chip Enable | Controls transmit/receive mode |
| CSN | GPIO4 | SPI Chip Select | SPI slave select |
| SCK | GPIO18 | SPI Clock | Serial clock |
| MOSI | GPIO23 | SPI Master Out | Data from ESP32 to nRF24L01+ |
| MISO | GPIO19 | SPI Master In | Data from nRF24L01+ to ESP32 |

## Button Connections

| Button | ESP32 Pin | Function | Description |
|--------|-----------|----------|-------------|
| CONTROL | GPIO0 | Control Button | Single button for start/stop/reset (internal pull-up) |

## Button Wiring

```
Button Wiring:
ESP32 GPIO0 ---- Button ---- GND
               (internal pull-up)
               
When button is pressed: GPIO reads LOW
When button is released: GPIO reads HIGH
```

## Complete Wiring Diagram

```
ESP32              nRF24L01+           Button/LED
------             ----------           ----------
3.3V -------------- VCC
GND --------------- GND
GPIO5 ------------ CE
GPIO4 ------------ CSN
GPIO18 ----------- SCK
GPIO23 ----------- MOSI
GPIO19 ----------- MISO

GPIO0 ------------ CONTROL ---- GND
GPIO2 ------------ STATUS LED (built-in)
```

## Power Requirements

- **Voltage**: 3.3V (do NOT use 5V - will damage the module)
- **Current**: Up to 15mA during transmission
- **Decoupling**: Add 10µF capacitor between VCC and GND near the nRF24L01+ module

## Button Specifications

- **Type**: Momentary push button (single button)
- **Wiring**: Active LOW (internal pull-up resistor used)
- **Debounce**: 50ms software debounce implemented
- **Update Rate**: 20Hz (50ms polling interval)
- **Operation**: Press duration detection for different functions

## Operation

1. **Short Press** (< 2 seconds): Toggle start/stop timing
2. **Long Press** (≥ 2 seconds): Reset timer to 00:00 and stop

## Radio Configuration

- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power**: 0 dBm
- **CRC**: Enabled
- **Auto-ACK**: Enabled
- **Payload Size**: 32 bytes
- **Address**: 0xE7E7E7E7E7

## Troubleshooting

1. **Button not responding**: Check button connection to GND on GPIO0
2. **Radio not transmitting**: Verify 3.3V power and SPI connections
3. **Intermittent operation**: Add 10µF decoupling capacitor
4. **Short range**: Check antenna placement and power supply stability
5. **Status LED not working**: Built-in LED on GPIO2 may vary by ESP32 board
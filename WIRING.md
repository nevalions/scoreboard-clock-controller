# ESP32 Scoreboard Controller Wiring Guide

## Overview

Complete wiring guide for ESP32 scoreboard controller with nRF24L01+ radio, ST7735 TFT display, KY-040 rotary encoder, and dedicated control/preset buttons.

## Display

### ST7735 128x160 TFT Color Display

- **Display**: 128×160 pixels, 65K colors
- **Interface**: SPI (separate bus from radio)
- **Pins**: CS=GPIO27, DC=GPIO26, RST=GPIO25, MOSI=GPIO13, SCK=GPIO14

> **✅ NO PIN CONFLICTS**:
>
> - ST7735 TFT uses separate SPI pins from nRF24L01+ radio
> - ST7735: SCK=GPIO14, MOSI=GPIO13 (separate bus)
> - Radio: SCK=GPIO18, MOSI=GPIO23 (VSPI bus)

> **Note**: An earlier revision of this firmware supported a 1602A I2C LCD as an
> alternative display. That driver (`lcd_i2c`), the I2C scanner, and the
> `USE_ST7735_DISPLAY` toggle have been removed from the codebase — the ST7735 TFT
> is now the only supported display.

## Pin Assignments Summary

| Component                        | ESP32 Pin | Function                          |
| -------------------------------- | --------- | --------------------------------- |
| **Radio nRF24L01+**              |           |                                   |
| VCC                              | 3.3V      | Power                             |
| GND                              | GND       | Ground                            |
| CE                               | GPIO5     | Chip Enable                       |
| CSN                              | GPIO4     | SPI Chip Select                   |
| SCK                              | GPIO18    | SPI Clock                         |
| MOSI                             | GPIO23    | SPI Master Out                    |
| MISO                             | GPIO19    | SPI Master In                     |
| **ST7735 TFT Display**           |           |                                   |
| Pin 2 (VCC)                      | 3.3V      | Power                             |
| Pin 1 (GND)                      | GND       | Ground                            |
| Pin 7 (CS)                       | GPIO27    | SPI Chip Select                   |
| Pin 6 (RS/DC)                    | GPIO26    | Data/Command                      |
| Pin 5 (RES)                      | GPIO25    | Reset                             |
| Pin 4 (SDA/MOSI)                 | GPIO13    | SPI Data (separate from radio)    |
| Pin 3 (SCK)                      | GPIO14    | SPI Clock (separate from radio)   |
| Pin 8 (LEDA)                     | 3.3V      | Backlight (optional)              |
| **KY-040 Rotary Encoder**        |           |                                   |
| VCC                              | 3.3V      | Power                             |
| GND                              | GND       | Ground                            |
| CLK                              | GPIO33    | Clock (A)                         |
| DT                               | GPIO16    | Data (B)                          |
| SW                               | GPIO32    | Switch/Button                     |
| **Control Button**               |           |                                   |
| Button                           | GPIO0     | Start/Stop/Reset/Menu (Internal Pull-up) |
| **Preset / Start / Reset Buttons** |         |                                   |
| Preset 1                         | GPIO21    | Sport variant preset 1 (Internal Pull-up) |
| Preset 2                         | GPIO22    | Sport variant preset 2 (Internal Pull-up) |
| Preset 3                         | GPIO36    | Sport variant preset 3 (⚠️ input-only, needs external pull-up) |
| Preset 4                         | GPIO34    | Sport variant preset 4 (⚠️ input-only, needs external pull-up) |
| START                            | GPIO35    | Start/stop toggle (⚠️ input-only, needs external pull-up) |
| RESET                            | GPIO15    | Reset to sport default (Internal Pull-up) |
| **Status LED**                   |           |                                   |
| LED                              | GPIO2     | Link Quality Indicator            |

---

## nRF24L01+ Radio Module

### Connections

| nRF24L01+ Pin | ESP32 Pin | Description                              |
| ------------- | --------- | ---------------------------------------- |
| VCC           | 3.3V      | **⚠️ 3.3V ONLY - 5V will damage module** |
| GND           | GND       | Common ground                            |
| CE            | GPIO5     | Chip Enable (controls TX/RX mode)        |
| CSN           | GPIO4     | SPI Chip Select                          |
| SCK           | GPIO18    | SPI Clock                                |
| MOSI          | GPIO23    | Master Out Slave In                      |
| MISO          | GPIO19    | Master In Slave Out                      |

### Specifications

- **Voltage**: 3.3V (do NOT use 5V)
- **Current**: Up to 15mA during transmission
- **Decoupling**: Add 10µF capacitor between VCC and GND near module
- **Antenna**: Ensure proper antenna connection for range

---

## ST7735 128x160 TFT Display

### Connections

**Your ST7735 1.77" Module Pinout:**

| Module Pin | Pin Number | ESP32 Pin | Description  | Important Notes         |
| ---------- | ---------- | --------- | ------------ | ----------------------- |
| GND        | 1          | GND       | Ground       | Common ground           |
| VCC        | 2          | 3.3V      | Power        | 3.3V ONLY               |
| SCK        | 3          | GPIO14    | SPI Clock    | **Separate from radio** |
| SDA/MOSI   | 4          | GPIO13    | SPI Data     | **Separate from radio** |
| RES/RST    | 5          | GPIO25    | Reset        | Hardware reset          |
| RS/DC      | 6          | GPIO26    | Data/Command | Control signal          |
| CS         | 7          | GPIO27    | Chip Select  | SPI chip select         |
| LEDA       | 8          | 3.3V      | Backlight    | Optional backlight      |

### Physical Pin Layout

```
          ST7735 1.77" Module
          +-----------------+
       GND |1              8| LEDA  ← 3.3V (backlight)
       VCC |2              7| CS    ← GPIO27
       SCK |3              6| RS    ← GPIO26
       SDA |4              5| RES   ← GPIO25
          +-----------------+
```

### ✅ NO PIN CONFLICTS

**Separate SPI Buses Implemented:**

- ST7735 uses GPIO14 (SCK) and GPIO13 (MOSI) - separate from radio
- nRF24L01+ radio uses GPIO18 (SCK) and GPIO23 (MOSI) - VSPI bus
- **No conflicts** - both devices work reliably

**Final Pin Assignments (No Conflicts):**

```
Radio (VSPI):           SCK→GPIO18  MOSI→GPIO23  CSN→GPIO4
ST7735 (HSPI):          SCK→GPIO14  MOSI→GPIO13  CS→GPIO27
                         ✓SEPARATE!  ✓SEPARATE!  ✓OK
```

**Display Features:**

- **Resolution**: 128×160 pixels
- **Colors**: 65K (16-bit RGB565)
- **Interface**: SPI (up to 32MHz)
- **Graphics**: Lines, rectangles, text, test patterns

**Power Requirements:**

- **Voltage**: 3.3V (do NOT use 5V)
- **Current**: ~20-30mA (backlight dependent)
- **Decoupling**: Add 10µF capacitor near display

---

## KY-040 Rotary Encoder Module

### Connections

| KY-040 Pin | ESP32 Pin | Description | Important Notes    |
| ---------- | --------- | ----------- | ------------------ |
| VCC        | 3.3V      | Power       | 3.3V power supply  |
| GND        | GND       | Ground      | Common ground      |
| CLK        | GPIO33    | Clock (A)   | Standard GPIO pin  |
| DT         | GPIO16    | Data (B)    | Standard GPIO pin  |
| SW         | GPIO32    | Switch      | Button press       |

### Notes for GPIO33/16

- **Standard GPIO pins**: Full input/output capabilities
- **CLK/DT pull-ups**: `rotary_encoder_begin()` explicitly disables internal pull-ups on
  CLK/DT and relies on the KY-040 module's onboard 10kΩ pull-ups. If wiring a bare
  encoder without a module, add external 10kΩ pull-ups to CLK and DT.
- **SW pull-up**: Only the switch pin (GPIO32) gets an internal pull-up enabled in software.

### Software Configuration

- **Pull-ups**: Internal pull-up enabled on SW only; CLK/DT depend on the module's hardware pull-ups
- **Debounce**: ~2ms rotation debounce (quadrature transition table), direction resets to none after 40ms of no movement
- **Quadrature decoding**: 2-bit transition table for reliable 1-step-per-detent tracking

### If Using Bare Encoder (No Module)

Add these external components:

- **10kΩ pull-up resistors** on CLK and DT to 3.3V (optional - can use internal pull-ups)
- **10kΩ pull-up resistor** on SW to 3.3V (optional - can use internal pull-up)
- **0.1µF capacitors** on CLK/DT for debouncing (optional)

### Operation

- **Clockwise rotation**: CLK leads DT
- **Counter-clockwise**: DT leads CLK
- **Button press**: Active LOW when pressed

---

## Control Button

### Single Button Operation

| Connection | ESP32 Pin | Description                          |
| ---------- | --------- | ------------------------------------ |
| Button     | GPIO0     | Control button with internal pull-up |

### Wiring

```
ESP32 GPIO0 ---- Button ---- GND
                (internal pull-up)
```

### Button Functions (active only while the timer is running)

- **Short Press** (< 2 seconds): Toggle start/stop timer
- **Long Press** (≥ 2 seconds): Stop and reset timer to sport default
- **Double Tap** (< 500ms between presses): Enter sport selection menu

---

## Preset, Start, and Reset Buttons

These four dedicated buttons are wired and debounced the same way as the control
button (active-low, via `button_driver.c`), but act immediately regardless of timer state.

| Connection | ESP32 Pin | Description                                             | Pull-up                        |
| ---------- | --------- | -------------------------------------------------------- | ------------------------------- |
| Preset 1   | GPIO21    | Load preset variant 1 of the current sport group          | Internal (enabled in software) |
| Preset 2   | GPIO22    | Load preset variant 2 of the current sport group          | Internal (enabled in software) |
| Preset 3   | GPIO36    | Load preset variant 3 of the current sport group          | Requires external pull-up      |
| Preset 4   | GPIO34    | Load preset variant 4 of the current sport group          | Requires external pull-up      |
| START      | GPIO35    | Toggle start/stop (same as control button short press)    | Requires external pull-up      |
| RESET      | GPIO15    | Stop timer and reset to sport default                      | Internal (enabled in software) |

### Critical: GPIO34/35/36 Have No Internal Pull-Up

GPIO34, GPIO35, and GPIO36 are **input-only** ESP32 pins with no internal pull-up or
pull-down circuitry available. `button_driver.c` only enables the internal pull-up for
pins numbered below 34 (`if (pin < 34)`), so preset buttons 3 and 4 (GPIO36/GPIO34) and
the START button (GPIO35) **require an external pull-up resistor** (e.g. 10kΩ to 3.3V).
Without it, these inputs float and the button will trigger randomly.

```
3.3V ---[10kΩ]---+---- ESP32 GPIO34 / GPIO35 / GPIO36
                 |
              Button
                 |
                GND
```

### Wiring (Preset 1/2 and RESET — internal pull-up sufficient)

```
ESP32 GPIO21 / GPIO22 / GPIO15 ---- Button ---- GND
                                   (internal pull-up)
```

---

## Status LED

### Connection

| LED        | ESP32 Pin | Description                  |
| ---------- | --------- | ----------------------------- |
| Status LED | GPIO2     | Radio link quality indicator |

### Operation

Driven by a 50% success-rate threshold (`RADIO_LINK_SUCCESS_RATE_THRESHOLD` in
`include/radio_comm.h`) over the recent transmission window:

- **Solid**: Good radio link (success rate > 50%, recent activity)
- **Blinking**: Recent transmission failures / poor link
- **Off**: No radio link

---

## Power Requirements

### Component Power Consumption

| Component  | Voltage | Current                | Notes                |
| ---------- | ------- | ---------------------- | -------------------- |
| nRF24L01+  | 3.3V    | 15mA (max TX)          | Add 10µF decoupling  |
| ST7735 TFT | 3.3V    | 20-30mA + backlight    | Color graphics       |
| KY-040     | 3.3V    | <1mA                   | With pull-ups        |
| ESP32      | 3.3V    | 150-260mA              | Varies with CPU load |

### Total Requirements

- **Idle**: ~220mA
- **Active**: ~330mA (with radio TX + backlight)

**Recommended Supply**: 500mA+ 3.3V power supply
**Decoupling**: 100µF electrolytic + 0.1µF ceramic near ESP32

---

## Radio Configuration

| Setting          | Value              | Description                                  |
| ----------------- | ------------------- | ---------------------------------------------- |
| Channel           | 20                  | 2.420 GHz                                     |
| Data Rate         | 1 Mbps              | Standard rate                                 |
| Power              | 0 dBm               | Maximum power                                 |
| Payload           | 6 bytes (fixed)     | No dynamic payloads                           |
| CRC               | 1-byte, enabled     | Error checking                                |
| Auto-ACK          | Enabled on pipe 0   | Reliability                                   |
| Auto-Retransmit   | SETUP_RETR = 0x4F   | 1250µs delay, up to 15 retries                |
| Address           | 0xE7E7E7E7E7        | Device address                                |
| Networking        | None                | Plain point-to-point radio — no mesh, no node IDs, no route discovery |

---

## Troubleshooting

### Radio Issues

1. **No communication**: Check 3.3V power (NOT 5V)
2. **Intermittent**: Add 10µF decoupling capacitor
3. **Short range**: Verify antenna and power supply
4. **SPI errors**: Check MOSI/MISO not swapped

### Display Issues

**ST7735 TFT Display:**

1. **Blank screen**: Check 3.3V power (pins 1&2) and SPI connections
2. **Garbled graphics**: SPI wiring error (check pins 3&4 for SCK/SDA)
3. **No display**: Verify CS (pin7→GPIO27), DC (pin6→GPIO26), RST (pin5→GPIO25) connections
4. **Wrong colors**: Check RGB565 color format
5. **Flickering**: Add decoupling capacitor near display
6. **Partial display**: Check initialization sequence
7. **No backlight**: Connect pin 8 (LEDA) to 3.3V
8. **SPI conflicts**: ✅ **NONE** - separate SPI buses implemented

### Encoder Issues

1. **No response**: Check 3.3V power to KY-040
2. **Erratic**: CLK/DT swapped or missing pull-ups (CLK/DT rely on the module's own pull-ups — the ESP32 does not enable internal pull-ups on these two pins)
3. **Button not working**: Check SW connection to GPIO32
4. **Wrong direction**: Swap CLK and DT connections
5. **Multiple steps per detent**: Quadrature transition-table decoding in `rotary_encoder.c` should prevent this
6. **Button not detected**: GPIO32 has internal pull-up enabled

### Preset/START/RESET Button Issues

1. **Preset 3/4 or START trigger randomly / register without a press**: Missing external
   pull-up on GPIO34/35/36 — these input-only pins have no internal pull-up capability
   and `button_driver.c` does not enable one for pins ≥ 34. Add a 10kΩ pull-up to 3.3V.
2. **Preset 1/2 or RESET not working**: Verify wiring to GPIO21/22/15; internal pull-up
   is enabled automatically.

### General Issues

1. **System unstable**: Insufficient 3.3V power supply
2. **Status LED not working**: Verify GPIO2 wiring; some ESP32 boards use GPIO2 for the onboard LED
3. **Boot issues**: GPIO0 conflicts (avoid during boot)

---

## Assembly Order

1. **Power Connections**: Connect 3.3V and GND to all modules
2. **Radio Module**: Connect SPI pins (GPIO5,4,18,23,19)
3. **Display**: Connect ST7735 SPI pins using your module's pin numbers:
   - Pin 7 (CS) → GPIO27
   - Pin 6 (RS/DC) → GPIO26
   - Pin 5 (RES) → GPIO25
   - Pin 4 (SDA/MOSI) → GPIO13
   - Pin 3 (SCK) → GPIO14
   - Pin 2 (VCC) → 3.3V
   - Pin 1 (GND) → GND
   - Pin 8 (LEDA) → 3.3V (backlight)
4. **Rotary Encoder**: Connect encoder pins (GPIO33,16,32)
5. **Control Button**: Connect button to GPIO0
6. **Preset/Start/Reset Buttons**: Connect preset1/2 and RESET to GPIO21/22/15
   (internal pull-up), then connect preset3, preset4, and START to GPIO36/34/35
   **with external 10kΩ pull-ups to 3.3V** (mandatory — see Critical Assembly Notes below)
7. **Status LED**: Connect LED to GPIO2
8. **Verify**: Double-check all connections before powering on

### ⚠️ Critical Assembly Notes

**SPI BUS SEPARATION:**

- **Radio uses**: GPIO18 (SCK), GPIO23 (MOSI), GPIO4 (CSN) ✅
- **ST7735 uses**: GPIO14 (SCK), GPIO13 (MOSI), GPIO27 (CS) ✅
- **No pin conflicts**: Separate SPI buses implemented ✅
- **Result**: Reliable operation for both devices ✅

**Mandatory External Pull-Ups (GPIO34/35/36):**

- GPIO34 (Preset 4), GPIO35 (START), and GPIO36 (Preset 3) are input-only pins with no
  internal pull-up capability.
- `button_driver.c` enables the internal pull-up only for pins numbered below 34, so
  these three buttons **will not work reliably without external 10kΩ pull-up resistors
  to 3.3V**.

---

## Safety Notes

⚠️ **Critical Warnings**

- **NEVER connect 5V to nRF24L01+** - will permanently damage
- **Double-check polarity** before applying power
- **Use appropriate power supply** - insufficient power causes instability

✅ **Best Practices**

- Add decoupling capacitors near modules
- Use short wires to reduce noise
- Verify connections with multimeter before power
- Test modules individually before full assembly

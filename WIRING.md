# ESP32 Scoreboard Controller Wiring Guide

## Overview

Complete wiring guide for ESP32 scoreboard controller with nRF24L01+ radio, display options (1602A I2C LCD or ST7735 TFT), and KY-040 rotary encoder.

## Display Options

### Option 1: 1602A LCD with PCF8574T I2C Adapter (Default)

- **Display**: 16 characters × 2 lines, monochrome
- **Interface**: I2C
- **Pins**: SDA=GPIO21, SCL=GPIO22

### Option 2: ST7735 128x160 TFT Color Display

- **Display**: 128×160 pixels, 65K colors
- **Interface**: SPI (separate bus from radio)
- **Pins**: CS=GPIO27, DC=GPIO26, RST=GPIO25, MOSI=GPIO13, SCK=GPIO14
- **Configuration**: Set `USE_ST7735_DISPLAY = true` in main.c

> **✅ NO PIN CONFLICTS**:
>
> - ST7735 TFT uses separate SPI pins from nRF24L01+ radio
> - ST7735: SCK=GPIO14, MOSI=GPIO13 (separate bus)
> - Radio: SCK=GPIO18, MOSI=GPIO23 (VSPI bus)
> - Both displays use the same software interface, just change the constant in main.c.

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
| **Display Option 1: I2C LCD**    |           |                                   |
| VCC                              | 3.3V      | Power                             |
| GND                              | GND       | Ground                            |
| SDA                              | GPIO21    | I2C Data                          |
| SCL                              | GPIO22    | I2C Clock                         |
| **Display Option 2: ST7735 TFT** |           |                                   |
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
| CLK                              | GPIO34    | Clock (A) - Input Only            |
| DT                               | GPIO35    | Data (B) - Input Only             |
| SW                               | GPIO32    | Switch/Button                     |
| **Control Button**               |           |                                   |
| Button                           | GPIO0     | Control Button (Internal Pull-up) |
| **Status LED**                   |           |                                   |
| LED                              | GPIO17    | Link Quality Indicator            |

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

## 1602A LCD with PCF8574T I2C Adapter

### I2C Adapter to ESP32

| I2C Adapter Pin | ESP32 Pin | Description                |
| --------------- | --------- | -------------------------- |
| VCC             | 3.3V      | Power (3.3V-5V compatible) |
| GND             | GND       | Common ground              |
| SDA             | GPIO21    | I2C Data                   |
| SCL             | GPIO22    | I2C Clock                  |

### PCF8574T to LCD Connections

| PCF8574T Pin | LCD Pin | Function        | Description             |
| ------------ | ------- | --------------- | ----------------------- |
| P0           | RS (4)  | Register Select | 0=Command, 1=Data       |
| P1           | RW (5)  | Read/Write      | Grounded for write mode |
| P2           | EN (6)  | Enable          | LCD data latch signal   |
| P3           | A (15)  | Backlight       | Backlight control       |
| P4           | D4 (11) | Data Bit 4      | 4-bit data              |
| P5           | D5 (12) | Data Bit 5      | 4-bit data              |
| P6           | D6 (13) | Data Bit 6      | 4-bit data              |
| P7           | D7 (14) | Data Bit 7      | 4-bit data              |

### LCD Power Connections

| LCD Pin | Connection | Description                      |
| ------- | ---------- | -------------------------------- |
| VSS (1) | GND        | LCD ground                       |
| VDD (2) | 3.3V       | LCD power                        |
| VO (3)  | Contrast   | Contrast control (potentiometer) |
| K (16)  | GND        | Backlight cathode                |

### I2C Address Configuration

**Your module detected at: 0x20**

| A2  | A1  | A0  | I2C Address | Binary  |
| --- | --- | --- | ----------- | ------- |
| 0   | 0   | 0   | **0x20**    | 0100000 |
| 0   | 0   | 1   | 0x21        | 0100001 |
| 0   | 1   | 0   | 0x22        | 0100010 |
| 0   | 1   | 1   | 0x23        | 0100011 |
| 1   | 0   | 0   | 0x24        | 0100100 |
| 1   | 0   | 1   | 0x25        | 0100101 |
| 1   | 1   | 0   | 0x26        | 0100110 |
| 1   | 1   | 1   | 0x27        | 0100111 |

**Note**: Address pins (A0-A2) are jumpers on PCF8574T module. Your scanner found 0x20, so all jumpers should be OFF.

### Module Variants

- **PCF8574T**: 0x20-0x27 (SO-16 surface mount - your module)
- **PCF8574**: 0x20-0x27 (DIP package)
- **PCF8574A**: 0x38-0x3F (alternative address range)

---

## ST7735 128x160 TFT Display (Option 2)

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

**Recommendations:**

1. **Use 1602A I2C LCD** - No pin conflicts (recommended)
2. **Use ST7735 TFT** - No pin conflicts (fully supported)
3. Both displays use the same software interface

**Both displays work reliably without conflicts.**

**Display Features:**

- **Resolution**: 128×160 pixels
- **Colors**: 65K (16-bit RGB565)
- **Interface**: SPI (up to 32MHz)
- **Graphics**: Lines, rectangles, text, test patterns

**Power Requirements:**

- **Voltage**: 3.3V (do NOT use 5V)
- **Current**: ~20-30mA (backlight dependent)
- **Decoupling**: Add 10µF capacitor near display

### Software Configuration

To use ST7735 display, modify `main.c`:

```c
#define USE_ST7735_DISPLAY true   // Use ST7735 TFT
// #define USE_ST7735_DISPLAY false  // Use I2C LCD
```

### Display Comparison

| Feature    | 1602A I2C LCD   | ST7735 TFT      |
| ---------- | --------------- | --------------- |
| Size       | 16×2 characters | 128×160 pixels  |
| Colors     | Monochrome      | 65K colors      |
| Interface  | I2C             | SPI             |
| Power      | ~20mA           | ~30mA           |
| Features   | Text only       | Graphics + text |
| Cost       | Low             | Medium          |
| Visibility | Good            | Excellent       |

---

## KY-040 Rotary Encoder Module

### Connections

| KY-040 Pin | ESP32 Pin | Description | Important Notes    |
| ---------- | --------- | ----------- | ------------------ |
| VCC        | 3.3V      | Power       | 3.3V power supply  |
| GND        | GND       | Ground      | Common ground      |
| CLK        | GPIO34    | Clock (A)   | **Input-only pin** |
| DT         | GPIO35    | Data (B)    | **Input-only pin** |
| SW         | GPIO32    | Switch      | Button press       |

### ⚠️ Critical Notes for GPIO34/35

- **Input-only pins**: Cannot be configured as outputs
- **No internal pull-ups**: External pull-ups required for bare encoders
- **KY-040 module includes**: 10kΩ pull-up resistors and debouncing
- **Software pull-ups enabled**: Code enables pull-ups on all 3 pins (GPIO32 gets internal pull-up)

### Software Configuration

- **Pull-ups**: Enabled on CLK, DT, and SW pins in software
- **Debounce**: 2ms rotation debounce, 30ms button debounce
- **Quadrature decoding**: Proper falling-edge detection for 1 step per detent

### If Using Bare Encoder (No Module)

Add these external components:

- **10kΩ pull-up resistors** on CLK and DT to 3.3V
- **10kΩ pull-up resistor** on SW to 3.3V
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

### Button Functions

- **Short Press** (< 2 seconds): Toggle start/stop timer
- **Long Press** (≥ 2 seconds): Reset timer to sport default
- **Double Tap**: Change sport

---

## Status LED

### Connection

| LED        | ESP32 Pin | Description                  |
| ---------- | --------- | ---------------------------- |
| Status LED | GPIO17    | Radio link quality indicator |

### Operation

- **Solid**: Good radio link
- **Blinking**: Poor radio link
- **Off**: No radio link

---

## Power Requirements

### Component Power Consumption

| Component       | Voltage | Current                | Notes                |
| --------------- | ------- | ---------------------- | -------------------- |
| nRF24L01+       | 3.3V    | 15mA (max TX)          | Add 10µF decoupling  |
| 1602A LCD + I2C | 3.3V    | 1-2mA + 20mA backlight | Backlight optional   |
| ST7735 TFT      | 3.3V    | 20-30mA + backlight    | Color graphics       |
| KY-040          | 3.3V    | <1mA                   | With pull-ups        |
| ESP32           | 3.3V    | 150-260mA              | Varies with CPU load |

### Total Requirements

**With 1602A LCD:**

- **Idle**: ~200mA
- **Active**: ~300mA (with radio TX + backlight)

**With ST7735 TFT:**

- **Idle**: ~220mA
- **Active**: ~330mA (with radio TX + backlight)

**Recommended Supply**: 500mA+ 3.3V power supply
**Decoupling**: 100µF electrolytic + 0.1µF ceramic near ESP32

---

## Radio Configuration

| Setting   | Value        | Description     |
| --------- | ------------ | --------------- |
| Channel   | 76           | 2.476 GHz       |
| Data Rate | 1 Mbps       | Standard rate   |
| Power     | 0 dBm        | Maximum power   |
| CRC       | Enabled      | Error checking  |
| Auto-ACK  | Enabled      | Reliability     |
| Payload   | 32 bytes     | Max packet size |
| Address   | 0xE7E7E7E7E7 | Device address  |

---

## Troubleshooting

### Radio Issues

1. **No communication**: Check 3.3V power (NOT 5V)
2. **Intermittent**: Add 10µF decoupling capacitor
3. **Short range**: Verify antenna and power supply
4. **SPI errors**: Check MOSI/MISO not swapped

### Display Issues

**1602A I2C LCD:**

1. **Blank display**: Check I2C address (use scanner)
2. **Garbage text**: Wrong I2C address or speed
3. **No backlight**: Check 3.3V power and jumper settings
4. **PCF8574T not responding**: Verify address jumpers
5. **Wrong address**: Your module uses 0x20, not 0x27

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
2. **Erratic**: CLK/DT swapped or missing pull-ups
3. **Button not working**: Check SW connection to GPIO32
4. **Wrong direction**: Swap CLK and DT connections
5. **Multiple steps per detent**: Software debounce should fix this
6. **Button not detected**: GPIO32 has internal pull-up enabled

### General Issues

1. **System unstable**: Insufficient 3.3V power supply
2. **Status LED not working**: GPIO17 varies by ESP32 board
3. **Boot issues**: GPIO0 conflicts (avoid during boot)

### I2C Scanner

Use built-in scanner to verify LCD address:

```bash
idf.py flash monitor
# Look for: "Found device at address: 0x20"
```

---

## Assembly Order

1. **Power Connections**: Connect 3.3V and GND to all modules
2. **Radio Module**: Connect SPI pins (GPIO5,4,18,23,19)
3. **Display Selection**:
   - **For 1602A LCD**: Connect I2C pins (GPIO21,22)
   - **For ST7735 TFT**: Connect SPI pins using your module's pin numbers:
     - Pin 7 (CS) → GPIO27
     - Pin 6 (RS/DC) → GPIO26
     - Pin 5 (RES) → GPIO25
     - Pin 4 (SDA/MOSI) → GPIO13
     - Pin 3 (SCK) → GPIO14
     - Pin 2 (VCC) → 3.3V
     - Pin 1 (GND) → GND
     - Pin 8 (LEDA) → 3.3V (backlight)
4. **Rotary Encoder**: Connect encoder pins (GPIO34,35,32)
5. **Control Button**: Connect button to GPIO0
6. **Status LED**: Connect LED to GPIO17
7. **Software Configuration**: Set `USE_ST7735_DISPLAY` in main.c
8. **Verify**: Double-check all connections before powering on

### ⚠️ Critical Assembly Notes

**SPI BUS SEPARATION (ST7735 Option):**

- **Radio uses**: GPIO18 (SCK), GPIO23 (MOSI), GPIO4 (CSN) ✅
- **ST7735 uses**: GPIO14 (SCK), GPIO13 (MOSI), GPIO27 (CS) ✅
- **No pin conflicts**: Separate SPI buses implemented ✅
- **Result**: Reliable operation for both devices ✅

**Display Selection:**

- **1602A I2C LCD**: No pin conflicts ✅ (RECOMMENDED)
- **ST7735 TFT**: No pin conflicts ✅ (FULLY SUPPORTED)
- Only connect ONE display at a time
- Configure software with matching `USE_ST7735_DISPLAY` setting

**Display Selection:**

- Only connect ONE display (1602A OR ST7735)
- Configure software with matching `USE_ST7735_DISPLAY` setting
- Both displays use same software interface
- Both options work reliably without pin conflicts

---

## Safety Notes

⚠️ **Critical Warnings**

- **NEVER connect 5V to nRF24L01+** - will permanently damage
- **GPIO34/35 are input-only** - cannot be used as outputs
- **Double-check polarity** before applying power
- **Use appropriate power supply** - insufficient power causes instability

✅ **Best Practices**

- Add decoupling capacitors near modules
- Use short wires to reduce noise
- Verify connections with multimeter before power
- Test modules individually before full assembly

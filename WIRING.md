# ESP32 Scoreboard Controller Wiring Guide

## Overview

Complete wiring guide for ESP32 scoreboard controller with nRF24L01+ radio, 1602A LCD with PCF8574T I2C adapter, and KY-040 rotary encoder.

## Pin Assignments Summary

| Component                 | ESP32 Pin | Function                          |
| ------------------------- | --------- | --------------------------------- |
| **Radio nRF24L01+**       |           |                                   |
| VCC                       | 3.3V      | Power                             |
| GND                       | GND       | Ground                            |
| CE                        | GPIO5     | Chip Enable                       |
| CSN                       | GPIO4     | SPI Chip Select                   |
| SCK                       | GPIO18    | SPI Clock                         |
| MOSI                      | GPIO23    | SPI Master Out                    |
| MISO                      | GPIO19    | SPI Master In                     |
| **LCD I2C Adapter**       |           |                                   |
| VCC                       | 3.3V      | Power                             |
| GND                       | GND       | Ground                            |
| SDA                       | GPIO21    | I2C Data                          |
| SCL                       | GPIO22    | I2C Clock                         |
| **KY-040 Rotary Encoder** |           |                                   |
| VCC                       | 3.3V      | Power                             |
| GND                       | GND       | Ground                            |
| CLK                       | GPIO34    | Clock (A) - Input Only            |
| DT                        | GPIO35    | Data (B) - Input Only             |
| SW                        | GPIO32    | Switch/Button                     |
| **Control Button**        |           |                                   |
| Button                    | GPIO0     | Control Button (Internal Pull-up) |
| **Status LED**            |           |                                   |
| LED                       | GPIO17    | Link Quality Indicator            |

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

| Component | Voltage | Current                | Notes                |
| --------- | ------- | ---------------------- | -------------------- |
| nRF24L01+ | 3.3V    | 15mA (max TX)          | Add 10µF decoupling  |
| LCD + I2C | 3.3V    | 1-2mA + 20mA backlight | Backlight optional   |
| KY-040    | 3.3V    | <1mA                   | With pull-ups        |
| ESP32     | 3.3V    | 150-260mA              | Varies with CPU load |

### Total Requirements

- **Idle**: ~200mA
- **Active**: ~300mA (with radio TX + backlight)
- **Recommended Supply**: 500mA+ 3.3V power supply
- **Decoupling**: 100µF electrolytic + 0.1µF ceramic near ESP32

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

### LCD Issues

1. **Blank display**: Check I2C address (use scanner)
2. **Garbage text**: Wrong I2C address or speed
3. **No backlight**: Check 3.3V power and jumper settings
4. **PCF8574T not responding**: Verify address jumpers
5. **Wrong address**: Your module uses 0x20, not 0x27

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
3. **I2C LCD**: Connect I2C pins (GPIO21,22)
4. **Rotary Encoder**: Connect encoder pins (GPIO34,35,32)
5. **Control Button**: Connect button to GPIO0
6. **Status LED**: Connect LED to GPIO17
7. **Verify**: Double-check all connections before powering on

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

# ESP32 Controller Agent Guidelines

## Build Commands
- `idf.py build` - Build the project
- `idf.py flash monitor` - Flash and monitor serial output
- `idf.py clean` - Clean build artifacts
- `idf.py menuconfig` - Configure project settings

## Important Notes
- **AGENTS/LLMS ONLY**: DO NOT ATTEMPT TO FLASH ON BOARD - Only build the project, do not flash
- **Users**: Can build and flash normally using `idf.py flash monitor`

## Code Style Guidelines
- **Language**: C (ESP-IDF framework)
- **Headers**: Use `#pragma once` for include guards
- **Includes**: Group system headers first, then local headers with relative paths
- **Naming**: 
  - Functions: `snake_case` (e.g., `button_begin`, `radio_send_time`)
  - Variables: `snake_case` (e.g., `control_button`, `current_seconds`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `CONTROL_BUTTON_PIN`, `STATUS_LED_PIN`)
  - Types: `PascalCase` for typedefs (e.g., `Button`, `RadioComm`)
- **Logging**: Use `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGD` with TAG constant
- **Error Handling**: Return bool for success/failure, check return values
- **File Organization**: Headers in `include/`, implementations in `main/`
- **FreeRTOS**: Use `vTaskDelay()` with `pdMS_TO_TICKS()` for timing
- **GPIO**: Use `gpio_num_t` enum for pin definitions

## Pin Definitions
- **STATUS_LED_PIN**: GPIO_NUM_17 - Link status LED
- **CONTROL_BUTTON_PIN**: GPIO_NUM_0 - Single control button (start/stop/reset)
- **NRF24_CE_PIN**: GPIO_NUM_5 - Radio CE pin
- **NRF24_CSN_PIN**: GPIO_NUM_4 - Radio CSN pin
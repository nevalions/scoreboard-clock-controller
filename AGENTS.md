# ESP32 Controller Agent Guidelines

## Build Commands
- `idf.py build` - Build the project
- `idf.py flash monitor` - Flash and monitor serial output
- `idf.py clean` - Clean build artifacts
- `idf.py menuconfig` - Configure project settings

## Important Notes
- **DO NOT ATTEMPT TO FLASH ON BOARD** - Only build the project, do not flash

## Code Style Guidelines
- **Language**: C (ESP-IDF framework)
- **Headers**: Use `#pragma once` for include guards
- **Includes**: Group system headers first, then local headers with relative paths
- **Naming**: 
  - Functions: `snake_case` (e.g., `button_begin`, `radio_send_command`)
  - Variables: `snake_case` (e.g., `start_button`, `current_seconds`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `START_BUTTON_PIN`, `CMD_RUN`)
  - Types: `PascalCase` for typedefs (e.g., `Button`, `RadioComm`)
- **Logging**: Use `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGD` with TAG constant
- **Error Handling**: Return bool for success/failure, check return values
- **File Organization**: Headers in `include/`, implementations in `main/`
- **FreeRTOS**: Use `vTaskDelay()` with `pdMS_TO_TICKS()` for timing
- **GPIO**: Use `gpio_num_t` enum for pin definitions
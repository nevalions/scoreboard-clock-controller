#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "../../radio-common/include/radio_config.h"
#include "../../radio-common/include/radio_common.h"

// Radio communication structure - extends RadioCommon with controller-specific fields
typedef struct {
  RadioCommon base;  // radio-common base structure

  // Link status tracking
  uint32_t last_success_time;
  uint32_t last_failure_time;
  uint16_t success_count;
  uint16_t failure_count;
  bool link_good;
} RadioComm;

// Function declarations
bool radio_begin(RadioComm *radio, gpio_num_t ce, gpio_num_t csn);
bool radio_send_time(RadioComm *radio, uint16_t seconds, uint8_t sequence);
bool radio_is_transmit_complete(RadioComm *radio);
void radio_flush_tx(RadioComm *radio);
void radio_dump_registers(RadioComm *radio);
void radio_update_link_status(RadioComm *radio);
bool radio_check_link_quality(RadioComm *radio);

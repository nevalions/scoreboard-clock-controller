#pragma once

#include "../../radio-common/include/radio_common.h"
#include "../../radio-common/include/radio_config.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdbool.h>
#include <stdint.h>

// Radio timing constants
#define RADIO_TRANSMIT_TIMEOUT_MS 30
#define RADIO_LINK_SUCCESS_WINDOW_MS 5000
#define RADIO_LINK_FAILURE_WINDOW_MS 2000
#define RADIO_LINK_LOG_INTERVAL_MS 10000
#define RADIO_LINK_SUCCESS_RATE_THRESHOLD 0.5f
#define RADIO_LINK_QUALITY_THRESHOLD 0.7f

// Radio communication structure - extends RadioCommon with controller-specific
// fields
typedef struct {
  RadioCommon base; // radio-common base structure

  // Link status tracking
  uint32_t last_success_time;
  uint32_t last_failure_time;
  uint16_t success_count;
  uint16_t failure_count;

  // ACK payload tracking
  bool ack_received;    // true if last TX got an ACK payload
  uint8_t ack_sequence; // first byte of ACK payload (we use it as sequence)

  bool link_good;

  // Runtime state (moved from static variables)
  bool led_state;
  uint32_t last_log_time;
} RadioComm;

// Function declarations
bool radio_begin(RadioComm *radio, gpio_num_t ce, gpio_num_t csn);

// Send main payload: seconds + RGB + sequence
bool radio_send_time(RadioComm *radio, uint16_t seconds, uint8_t r, uint8_t g,
                     uint8_t b, uint8_t sequence);

bool radio_is_transmit_complete(RadioComm *radio);
void radio_flush_tx(RadioComm *radio);
void radio_dump_registers(RadioComm *radio);
void radio_update_link_status(RadioComm *radio);
bool radio_check_link_quality(RadioComm *radio);

// Helper: check if last TX got a matching ACK sequence
static inline bool radio_last_ack_ok(const RadioComm *radio,
                                     uint8_t sent_sequence) {
  return (radio && radio->ack_received && radio->ack_sequence == sent_sequence);
}

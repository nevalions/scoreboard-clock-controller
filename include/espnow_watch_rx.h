#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../../radio-common/include/espnow_link.h"

// Referee-watch uplink receiver: brings up WiFi (STA, no association) on
// ESPNOW_WIFI_CHANNEL and accepts encrypted ESP-NOW command frames from
// allowlisted watch MACs (espnow_watches.h). The nRF24 display broadcast
// is a separate radio and is unaffected.
//
// Received commands are queued from the WiFi task and drained by the main
// loop via espnow_watch_rx_poll(); per-watch sequence dedupe means a
// watch's retry burst can never double-toggle the clock.

// Returns false if WiFi/ESP-NOW init failed (controller works without
// watches - buttons and rotary are unaffected)
bool espnow_watch_rx_init(void);

// Non-blocking: true + fills *out_cmd when a watch command is pending
bool espnow_watch_rx_poll(espnow_cmd_t *out_cmd);

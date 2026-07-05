#pragma once

// ============================================================================
// REFEREE WATCH ALLOWLIST - edit per deployment
// ============================================================================
// Each entry is a paired watch's STA MAC (the watch logs it at boot).
// All-zero rows are ignored, so the firmware builds and runs before any
// watch is paired - the ESP-NOW receiver just accepts nothing.

#define ESPNOW_WATCH_COUNT 2
#define ESPNOW_WATCH_MACS                                                     \
  {                                                                           \
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},                                     \
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},                                     \
  }

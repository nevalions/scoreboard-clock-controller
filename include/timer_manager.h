#pragma once

#include <stdbool.h>
#include <stdint.h>

// Delay after reaching zero before broadcasting the null (0xFF) signal
// that tells displays to clear
#define TIMER_NULL_SIGNAL_DELAY_MS 3000

// Null signal value transmitted in place of seconds to clear displays
#define TIMER_NULL_SIGNAL 0xFF

typedef struct {
  uint16_t current_seconds;
  bool is_running;
  uint32_t timer_last_update;
  uint32_t zero_reached_timestamp;
} TimerManager;

void timer_manager_init(TimerManager *manager, uint16_t initial_seconds);
void timer_manager_update(TimerManager *manager);
void timer_manager_start(TimerManager *manager);
void timer_manager_stop(TimerManager *manager);
void timer_manager_start_stop(TimerManager *manager);
void timer_manager_reset(TimerManager *manager, uint16_t seconds);
bool timer_manager_should_send_null(const TimerManager *manager);
uint16_t timer_manager_get_seconds(const TimerManager *manager);
bool timer_manager_is_running(const TimerManager *manager);

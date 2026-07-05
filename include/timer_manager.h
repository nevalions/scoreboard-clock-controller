#pragma once

#include <stdbool.h>
#include <stdint.h>

// Delay after reaching zero before broadcasting the null (0xFF) signal
// that tells displays to clear
#define TIMER_NULL_SIGNAL_DELAY_MS 3000

// Null signal value transmitted in place of seconds to clear displays
#define TIMER_NULL_SIGNAL 0xFF

// Display is 2-digit: the clock never holds more than 99 seconds
#define TIMER_MAX_SECONDS 99

// Countdown state in milliseconds: stop preserves the exact remaining
// time (officials expect stop/start to keep the fraction of a second)
typedef struct {
  uint32_t remaining_ms;
  bool is_running;
  uint32_t last_update_ms;
  uint32_t zero_reached_timestamp;
} TimerManager;

void timer_manager_init(TimerManager *manager, uint16_t initial_seconds);
void timer_manager_update(TimerManager *manager);
void timer_manager_start(TimerManager *manager);
void timer_manager_stop(TimerManager *manager);
void timer_manager_start_stop(TimerManager *manager);
void timer_manager_reset(TimerManager *manager, uint16_t seconds);
bool timer_manager_should_send_null(const TimerManager *manager);

// Whole seconds for display: ceiling, so the clock shows "5" until the
// countdown actually crosses 4.0 (shot-clock convention)
uint16_t timer_manager_get_seconds(const TimerManager *manager);

// Remaining deciseconds: truncated, so 4.95s reads as 4.9 ("49").
// 0 means less than 100ms left (or expired - check remaining_ms)
uint16_t timer_manager_get_deciseconds(const TimerManager *manager);

uint32_t timer_manager_get_remaining_ms(const TimerManager *manager);

// Officials' correction while stopped: shift the clock by delta_ms,
// clamped to [0, TIMER_MAX_SECONDS]. Ignored while running
void timer_manager_adjust_ms(TimerManager *manager, int32_t delta_ms);

bool timer_manager_is_running(const TimerManager *manager);

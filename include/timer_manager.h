#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t current_seconds;
  bool is_running;
  bool null_sent;

  uint32_t timer_last_update;      // timestamp of last 1-second tick
  uint32_t zero_reached_timestamp; // when 0 was reached (for null signal)
} TimerManager;

void timer_manager_init(TimerManager *manager, uint16_t initial_seconds);
void timer_manager_update(TimerManager *manager);
void timer_manager_start_stop(TimerManager *manager);
void timer_manager_reset(TimerManager *manager, uint16_t seconds);
void timer_manager_adjust_time(TimerManager *manager, int adjustment);
bool timer_manager_should_send_null(const TimerManager *manager);
void timer_manager_mark_null_sent(TimerManager *manager);
uint16_t timer_manager_get_seconds(const TimerManager *manager);
bool timer_manager_is_running(const TimerManager *manager);

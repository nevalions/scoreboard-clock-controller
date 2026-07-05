#include "timer_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint32_t now_ms(void) {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void timer_manager_init(TimerManager *m, uint16_t initial_seconds) {
  m->remaining_ms = (uint32_t)initial_seconds * 1000;
  m->is_running = false;
  m->last_update_ms = now_ms();
  m->zero_reached_timestamp = 0;
}

void timer_manager_start(TimerManager *m) {
  m->is_running = true;
  m->last_update_ms = now_ms();
}

void timer_manager_stop(TimerManager *m) {
  // Accrue the partial second before freezing so stop preserves the
  // exact remaining time (stop at 3.4 resumes at 3.4)
  timer_manager_update(m);
  m->is_running = false;
}

void timer_manager_start_stop(TimerManager *m) {
  if (m->is_running)
    timer_manager_stop(m);
  else
    timer_manager_start(m);
}

void timer_manager_reset(TimerManager *m, uint16_t seconds) {
  m->remaining_ms = (uint32_t)seconds * 1000;
  m->zero_reached_timestamp = 0;
  m->last_update_ms = now_ms();
}

void timer_manager_update(TimerManager *m) {
  if (!m->is_running)
    return;

  uint32_t now = now_ms();
  uint32_t elapsed = now - m->last_update_ms;
  m->last_update_ms = now;

  m->remaining_ms = (elapsed >= m->remaining_ms) ? 0 : m->remaining_ms - elapsed;

  if (m->remaining_ms == 0 && m->zero_reached_timestamp == 0) {
    m->zero_reached_timestamp = now;
  }
}

bool timer_manager_should_send_null(const TimerManager *m) {
  if (m->remaining_ms != 0 || m->zero_reached_timestamp == 0)
    return false;

  return (now_ms() - m->zero_reached_timestamp) >= TIMER_NULL_SIGNAL_DELAY_MS;
}

uint16_t timer_manager_get_seconds(const TimerManager *m) {
  return (uint16_t)((m->remaining_ms + 999) / 1000);
}

uint16_t timer_manager_get_deciseconds(const TimerManager *m) {
  return (uint16_t)(m->remaining_ms / 100);
}

uint32_t timer_manager_get_remaining_ms(const TimerManager *m) {
  return m->remaining_ms;
}

void timer_manager_adjust_ms(TimerManager *m, int32_t delta_ms) {
  if (m->is_running)
    return;

  int64_t adjusted = (int64_t)m->remaining_ms + delta_ms;
  if (adjusted < 0)
    adjusted = 0;
  if (adjusted > (int64_t)TIMER_MAX_SECONDS * 1000)
    adjusted = (int64_t)TIMER_MAX_SECONDS * 1000;

  m->remaining_ms = (uint32_t)adjusted;
  if (m->remaining_ms > 0) {
    m->zero_reached_timestamp = 0;
  }
}

bool timer_manager_is_running(const TimerManager *m) { return m->is_running; }

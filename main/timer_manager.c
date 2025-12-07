#include "timer_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void timer_manager_init(TimerManager *m, uint16_t initial_seconds) {
  m->current_seconds = initial_seconds;
  m->is_running = false;
  m->null_sent = false;
  m->timer_last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
  m->zero_reached_timestamp = 0;
}

void timer_manager_start(TimerManager *m) {
  m->is_running = true;
  m->timer_last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void timer_manager_stop(TimerManager *m) { m->is_running = false; }

void timer_manager_start_stop(TimerManager *m) {
  if (m->is_running)
    timer_manager_stop(m);
  else
    timer_manager_start(m);
}

void timer_manager_reset(TimerManager *m, uint16_t seconds) {
  m->current_seconds = seconds;
  m->null_sent = false;
  m->zero_reached_timestamp = 0;
  m->timer_last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void timer_manager_update(TimerManager *m) {
  if (!m->is_running)
    return;

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  if (now - m->timer_last_update >= 1000) {
    m->timer_last_update = now;
    if (m->current_seconds > 0)
      m->current_seconds--;

    if (m->current_seconds == 0 && m->zero_reached_timestamp == 0) {
      m->zero_reached_timestamp = now;
    }
  }
}

uint16_t timer_manager_get_seconds(const TimerManager *m) {
  return m->current_seconds;
}

bool timer_manager_is_running(const TimerManager *m) { return m->is_running; }

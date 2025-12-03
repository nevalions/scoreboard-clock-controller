#include "timer_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMER_UPDATE_INTERVAL_MS 1000
#define ZERO_CLEAR_DELAY_MS 3000

static const char *TAG = "TIMER_MGR";

void timer_manager_init(TimerManager *manager, uint16_t initial_seconds) {
  manager->current_seconds = initial_seconds;
  manager->is_running = false;
  manager->null_sent = false;

  manager->timer_last_update = 0;
  manager->zero_reached_timestamp = 0;
}

void timer_manager_update(TimerManager *manager) {
  if (!manager->is_running)
    return;

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Only tick when a full second has passed
  if (now - manager->timer_last_update >= TIMER_UPDATE_INTERVAL_MS) {

    manager->timer_last_update = now;

    if (manager->current_seconds > 0) {
      manager->current_seconds--;
      ESP_LOGD(TAG, "Tick → %d", manager->current_seconds);

    } else {
      // Reached zero, stop timer
      manager->is_running = false;
      manager->zero_reached_timestamp = now;
      manager->null_sent = false;

      ESP_LOGI(TAG, "Timer reached zero → STOP");
    }
  }
}

void timer_manager_start_stop(TimerManager *manager) {
  manager->is_running = !manager->is_running;

  if (manager->is_running) {
    // Prevent instant decrement on start
    manager->timer_last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
  }

  ESP_LOGI(TAG, "Timer %s", manager->is_running ? "START" : "STOP");
}

void timer_manager_reset(TimerManager *manager, uint16_t seconds) {
  manager->current_seconds = seconds;
  manager->is_running = false;
  manager->null_sent = false;

  ESP_LOGI(TAG, "Timer RESET → %d", seconds);
}

void timer_manager_adjust_time(TimerManager *manager, int adjustment) {
  int new_val = manager->current_seconds + adjustment;

  if (new_val < 0)
    new_val = 0;
  if (new_val > 999)
    new_val = 999;

  manager->current_seconds = new_val;

  ESP_LOGI(TAG, "Time ADJUST %d → %d", adjustment, manager->current_seconds);
}

bool timer_manager_should_send_null(const TimerManager *manager) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  return (!manager->is_running && manager->current_seconds == 0 &&
          manager->zero_reached_timestamp > 0 &&
          (now - manager->zero_reached_timestamp) >= ZERO_CLEAR_DELAY_MS &&
          !manager->null_sent);
}

void timer_manager_mark_null_sent(TimerManager *manager) {
  manager->null_sent = true;
  ESP_LOGI(TAG, "Null-signal SENT");
}

uint16_t timer_manager_get_seconds(const TimerManager *manager) {
  return manager->current_seconds;
}

bool timer_manager_is_running(const TimerManager *manager) {
  return manager->is_running;
}

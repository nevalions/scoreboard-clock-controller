#include "../include/timer_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMER_UPDATE_INTERVAL_MS 1000
#define ZERO_CLEAR_DELAY_MS 3000

static const char *TAG = "TIMER_MGR";

static inline uint32_t get_current_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static inline bool time_elapsed(uint32_t start, uint32_t interval) {
    return (get_current_time_ms() - start) >= interval;
}

void timer_manager_init(TimerManager *manager, uint16_t initial_seconds) {
    manager->current_seconds = initial_seconds;
    manager->is_running = false;
    manager->null_sent = false;
    manager->timer_last_update = 0;
    manager->zero_reached_timestamp = 0;
}

void timer_manager_update(TimerManager *manager) {
    if (!manager->is_running) {
        return;
    }

    if (time_elapsed(manager->timer_last_update, TIMER_UPDATE_INTERVAL_MS)) {
        if (manager->current_seconds > 0) {
            manager->current_seconds--;
            manager->timer_last_update = get_current_time_ms();
            ESP_LOGI(TAG, "Timer counting down: %d seconds", manager->current_seconds);
        } else {
            manager->is_running = false;
            manager->zero_reached_timestamp = get_current_time_ms();
            manager->null_sent = false;
            ESP_LOGI(TAG, "Timer reached zero - stopped");
        }
    }
}

void timer_manager_start_stop(TimerManager *manager) {
    manager->is_running = !manager->is_running;
    ESP_LOGI(TAG, "Timer %s (running: %d)", 
             manager->is_running ? "started" : "stopped", manager->is_running);
}

void timer_manager_reset(TimerManager *manager, uint16_t seconds) {
    manager->current_seconds = seconds;
    manager->is_running = false;
    manager->null_sent = false;
    ESP_LOGI(TAG, "Timer reset to %d seconds", seconds);
}

void timer_manager_adjust_time(TimerManager *manager, int adjustment) {
    if (adjustment > 0 && manager->current_seconds < 999) {
        manager->current_seconds += adjustment;
    } else if (adjustment < 0 && manager->current_seconds > 0) {
        manager->current_seconds += adjustment;
    }
    ESP_LOGI(TAG, "Time adjusted by %d: %d seconds", adjustment, manager->current_seconds);
}

bool timer_manager_should_send_null(const TimerManager *manager) {
    return !manager->is_running && 
           manager->current_seconds == 0 && 
           manager->zero_reached_timestamp > 0 &&
           time_elapsed(manager->zero_reached_timestamp, ZERO_CLEAR_DELAY_MS) &&
           !manager->null_sent;
}

void timer_manager_mark_null_sent(TimerManager *manager) {
    manager->null_sent = true;
    ESP_LOGI(TAG, "Null signal sent to clear display");
}

uint16_t timer_manager_get_seconds(const TimerManager *manager) {
    return manager->current_seconds;
}

bool timer_manager_is_running(const TimerManager *manager) {
    return manager->is_running;
}
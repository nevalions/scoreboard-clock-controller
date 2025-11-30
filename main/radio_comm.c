#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "radio_comm.h"

static const char *TAG = "RADIO_COMM";

bool radio_begin(RadioComm *radio, gpio_num_t ce, gpio_num_t csn) {
  ESP_LOGI(TAG, "Initializing nRF24L01+ transmitter");

  memset(radio, 0, sizeof(RadioComm));
  radio->led_state = false;
  radio->last_log_time = 0;
  
  // Initialize link status LED
  gpio_config_t led_conf = {
    .pin_bit_mask = (1ULL << RADIO_STATUS_LED_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&led_conf);
  gpio_set_level(RADIO_STATUS_LED_PIN, 0);

  // Initialize using radio-common
  if (!radio_common_init(&radio->base, ce, csn)) {
    ESP_LOGE(TAG, "Radio initialization failed");
    return false;
  }
  
  // Configure radio settings
  if (!radio_common_configure(&radio->base)) {
    ESP_LOGE(TAG, "Radio configuration failed");
    return false;
  }

  // Power up the radio and set to TX mode for initial configuration
  nrf24_power_up(&radio->base);
  nrf24_write_register(&radio->base, NRF24_REG_CONFIG, RADIO_CONFIG_TX_MODE);
  vTaskDelay(pdMS_TO_TICKS(2)); // Small delay to ensure mode switch

  // Ensure radio is powered up and in TX mode
  nrf24_power_up(&radio->base);
  uint8_t config = nrf24_read_register(&radio->base, NRF24_REG_CONFIG);
  config |= NRF24_CONFIG_PWR_UP;  // Ensure PWR_UP is set
  config &= ~NRF24_CONFIG_PRIM_RX;  // Ensure TX mode (PRIM_RX = 0)
  nrf24_write_register(&radio->base, NRF24_REG_CONFIG, config);
  vTaskDelay(pdMS_TO_TICKS(2)); // Small delay to ensure mode switch

  ESP_LOGI(TAG, "nRF24L01+ transmitter initialized successfully");
  return true;
}

bool radio_send_time(RadioComm *radio, uint16_t seconds, uint8_t r, uint8_t g, uint8_t b, uint8_t sequence) {
  if (!radio || !radio->base.initialized) {
    ESP_LOGE(TAG, "Radio not initialized");
    return false;
  }

  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Flush any pending TX data
  radio_flush_tx(radio);

  // Prepare payload - seconds, RGB, and sequence
  uint8_t payload[RADIO_PAYLOAD_SIZE];
  memset(payload, 0, RADIO_PAYLOAD_SIZE);
  payload[0] = (seconds >> 8) & 0xFF;
  payload[1] = seconds & 0xFF;
  payload[2] = r;
  payload[3] = g;
  payload[4] = b;
  payload[5] = sequence;

  // Set to transmitter mode (clear PRIM_RX bit)
  uint8_t config = nrf24_read_register(&radio->base, NRF24_REG_CONFIG);
  config &= ~NRF24_CONFIG_PRIM_RX;
  nrf24_write_register(&radio->base, NRF24_REG_CONFIG, config);
  vTaskDelay(pdMS_TO_TICKS(1));
  
  // Pulse CE to start transmission
  gpio_set_level(radio->base.ce_pin, 0);
  nrf24_write_payload(&radio->base, payload, RADIO_PAYLOAD_SIZE);
  gpio_set_level(radio->base.ce_pin, 1);
  
   // Wait for transmission to complete (max 30ms to allow for retries)
   uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
   while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - start_time < RADIO_TRANSMIT_TIMEOUT_MS) {
    uint8_t status = nrf24_get_status(&radio->base);
    if (status & NRF24_STATUS_TX_DS) {
      // Transmission successful
      nrf24_write_register(&radio->base, NRF24_REG_STATUS, NRF24_STATUS_TX_DS);
      gpio_set_level(radio->base.ce_pin, 0);
      radio->success_count++;
      radio->last_success_time = current_time;
      ESP_LOGI(TAG, "Time sent: %d seconds, RGB(%d,%d,%d), seq: %d (success #%d)", seconds, r, g, b, sequence, radio->success_count);
      return true;
    }
    if (status & NRF24_STATUS_MAX_RT) {
      // Max retries reached
      nrf24_write_register(&radio->base, NRF24_REG_STATUS, NRF24_STATUS_MAX_RT);
      gpio_set_level(radio->base.ce_pin, 0);
      radio->failure_count++;
      radio->last_failure_time = current_time;
      ESP_LOGW(TAG, "Transmission failed - max retries (failure #%d)", radio->failure_count);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // Timeout
  gpio_set_level(radio->base.ce_pin, 0);
  radio->failure_count++;
  radio->last_failure_time = current_time;
  ESP_LOGW(TAG, "Transmission timeout (failure #%d)", radio->failure_count);
  return false;
}

bool radio_is_transmit_complete(RadioComm* radio) {
  uint8_t status = nrf24_get_status(&radio->base);
  return (status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0;
}

void radio_flush_tx(RadioComm* radio) {
  nrf24_flush_tx(&radio->base);
  ESP_LOGD(TAG, "Flushing TX buffer");
}

void radio_dump_registers(RadioComm* radio) {
  radio_common_dump_registers(&radio->base);
}

void radio_update_link_status(RadioComm* radio) {
  if (!radio->base.initialized) {
    gpio_set_level(RADIO_STATUS_LED_PIN, 0);
    return;
  }

  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
   // Check if we've had recent successful transmissions
   bool recent_success = (current_time - radio->last_success_time) < RADIO_LINK_SUCCESS_WINDOW_MS;
   bool recent_failure = (current_time - radio->last_failure_time) < RADIO_LINK_FAILURE_WINDOW_MS;
  
  // Determine link quality
  if (radio->success_count == 0) {
    radio->link_good = false;
  } else {
    uint16_t total_attempts = radio->success_count + radio->failure_count;
    float success_rate = (float)radio->success_count / total_attempts;
    radio->link_good = success_rate > RADIO_LINK_SUCCESS_RATE_THRESHOLD && recent_success;
  }

  // Update LED based on link status
  if (radio->link_good) {
    gpio_set_level(RADIO_STATUS_LED_PIN, 1); // LED on for good link
  } else if (recent_failure) {
    // Blink LED rapidly for recent failures
    radio->led_state = !radio->led_state;
    gpio_set_level(RADIO_STATUS_LED_PIN, radio->led_state ? 1 : 0);
  } else {
    gpio_set_level(RADIO_STATUS_LED_PIN, 0); // LED off for no link
  }

  // Log link status periodically
  if (current_time - radio->last_log_time > RADIO_LINK_LOG_INTERVAL_MS) {
    radio->last_log_time = current_time;
    if (radio->success_count + radio->failure_count > 0) {
      float success_rate = (float)radio->success_count / (radio->success_count + radio->failure_count) * 100.0f;
      ESP_LOGI(TAG, "Link Status: %s | Success Rate: %.1f%% | Success: %d, Failures: %d", 
               radio->link_good ? "GOOD" : "POOR", success_rate, radio->success_count, radio->failure_count);
    } else {
      ESP_LOGI(TAG, "Link Status: NO TRANSMISSIONS YET");
    }
  }
}

bool radio_check_link_quality(RadioComm* radio) {
  if (!radio->base.initialized) {
    return false;
  }

  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
  // Check if we've had any successful transmissions recently
  if (radio->success_count == 0) {
    return false;
  }

  // Check success rate and recent activity
  uint16_t total_attempts = radio->success_count + radio->failure_count;
  float success_rate = (float)radio->success_count / total_attempts;
  bool recent_success = (current_time - radio->last_success_time) < RADIO_LINK_SUCCESS_WINDOW_MS;

  return success_rate > RADIO_LINK_QUALITY_THRESHOLD && recent_success;
}
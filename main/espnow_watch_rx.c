#include "espnow_watch_rx.h"
#include "espnow_watches.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "ESPNOW_RX";

static const uint8_t watch_macs[ESPNOW_WATCH_COUNT][6] = ESPNOW_WATCH_MACS;

// Last accepted sequence per watch_id (1-based); -1 = none yet
static int16_t last_sequence[ESPNOW_MAX_WATCHES + 1];

static QueueHandle_t cmd_queue;

static bool mac_allowlisted(const uint8_t *mac) {
  static const uint8_t zero[6] = {0};
  for (int i = 0; i < ESPNOW_WATCH_COUNT; i++) {
    if (memcmp(watch_macs[i], zero, 6) == 0) {
      continue; // unpaired slot
    }
    if (memcmp(watch_macs[i], mac, 6) == 0) {
      return true;
    }
  }
  return false;
}

// WiFi task context: validate, dedupe, queue. Never touch app state here
static void espnow_recv_cb(const esp_now_recv_info_t *info,
                           const uint8_t *data, int len) {
  if (len != (int)sizeof(EspNowCommand)) {
    return;
  }
  if (!mac_allowlisted(info->src_addr)) {
    ESP_LOGW(TAG, "Frame from non-allowlisted MAC dropped");
    return;
  }

  EspNowCommand cmd;
  memcpy(&cmd, data, sizeof(cmd));

  if (cmd.magic != ESPNOW_CMD_MAGIC || cmd.watch_id == 0 ||
      cmd.watch_id > ESPNOW_MAX_WATCHES) {
    return;
  }

  // Retry-burst dedupe: same (watch_id, sequence) as last accepted = repeat
  if (last_sequence[cmd.watch_id] == (int16_t)cmd.sequence) {
    return;
  }
  last_sequence[cmd.watch_id] = (int16_t)cmd.sequence;

  uint8_t command = cmd.command;
  if (xQueueSend(cmd_queue, &command, 0) != pdTRUE) {
    ESP_LOGW(TAG, "Command queue full - dropped");
  }
}

bool espnow_watch_rx_init(void) {
  for (int i = 0; i <= ESPNOW_MAX_WATCHES; i++) {
    last_sequence[i] = -1;
  }

  cmd_queue = xQueueCreate(4, sizeof(uint8_t));
  if (!cmd_queue) {
    return false;
  }

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS init failed");
    return false;
  }

  if (esp_netif_init() != ESP_OK ||
      esp_event_loop_create_default() != ESP_OK) {
    return false;
  }

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  if (esp_wifi_init(&cfg) != ESP_OK) {
    return false;
  }
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  if (esp_wifi_start() != ESP_OK) {
    return false;
  }
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  ESP_LOGI(TAG, "Controller MAC %02x:%02x:%02x:%02x:%02x:%02x (put this in "
                "each watch's watch_config.h)",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    return false;
  }
  esp_now_set_pmk((const uint8_t *)ESPNOW_PMK);

  // Register each paired watch as an encrypted peer
  static const uint8_t zero[6] = {0};
  int paired = 0;
  for (int i = 0; i < ESPNOW_WATCH_COUNT; i++) {
    if (memcmp(watch_macs[i], zero, 6) == 0) {
      continue;
    }
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, watch_macs[i], 6);
    peer.channel = ESPNOW_WIFI_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = true;
    memcpy(peer.lmk, ESPNOW_LMK, 16);
    if (esp_now_add_peer(&peer) == ESP_OK) {
      paired++;
    }
  }

  if (esp_now_register_recv_cb(espnow_recv_cb) != ESP_OK) {
    return false;
  }

  ESP_LOGI(TAG, "Watch uplink ready on WiFi channel %d, %d watch(es) paired",
           ESPNOW_WIFI_CHANNEL, paired);
  return true;
}

bool espnow_watch_rx_poll(espnow_cmd_t *out_cmd) {
  if (!cmd_queue || !out_cmd) {
    return false;
  }

  uint8_t command;
  if (xQueueReceive(cmd_queue, &command, 0) != pdTRUE) {
    return false;
  }

  *out_cmd = (espnow_cmd_t)command;
  return true;
}

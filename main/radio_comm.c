#include "../include/radio_comm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "RADIO_COMM";

static uint8_t spi_transfer(RadioComm* radio, uint8_t data) {
  uint8_t rx_data;
  spi_transaction_t trans = {
    .length = 8,
    .tx_buffer = &data,
    .rx_buffer = &rx_data
  };
  spi_device_transmit(radio->spi, &trans);
  return rx_data;
}

uint8_t nrf24_read_register(RadioComm* radio, uint8_t reg) {
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_R_REGISTER | reg);
  uint8_t value = spi_transfer(radio, NRF24_CMD_NOP);
  gpio_set_level(radio->csn_pin, 1);
  return value;
}

bool nrf24_write_register(RadioComm* radio, uint8_t reg, uint8_t value) {
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_W_REGISTER | reg);
  spi_transfer(radio, value);
  gpio_set_level(radio->csn_pin, 1);
  return true;
}

uint8_t nrf24_get_status(RadioComm* radio) {
  gpio_set_level(radio->csn_pin, 0);
  uint8_t status = spi_transfer(radio, NRF24_CMD_NOP);
  gpio_set_level(radio->csn_pin, 1);
  return status;
}

bool nrf24_write_payload(RadioComm* radio, uint8_t* data, uint8_t length) {
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_TX_PAYLOAD);
  for (uint8_t i = 0; i < length; i++) {
    spi_transfer(radio, data[i]);
  }
  gpio_set_level(radio->csn_pin, 1);
  return true;
}

bool radio_begin(RadioComm *radio, gpio_num_t ce, gpio_num_t csn) {
  ESP_LOGI(TAG, "Initializing nRF24L01+ transmitter");

  memset(radio, 0, sizeof(RadioComm));
  radio->ce_pin = ce;
  radio->csn_pin = csn;

  // Initialize addresses
  uint8_t default_tx[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
  uint8_t default_rx[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
  memcpy(radio->tx_address, default_tx, 5);
  memcpy(radio->rx_address, default_rx, 5);

  // Configure GPIO pins
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << ce) | (1ULL << csn),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&io_conf);

  gpio_set_level(ce, 0);
  gpio_set_level(csn, 1);

  // Configure SPI bus
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = NRF24_MOSI_PIN,
    .miso_io_num = NRF24_MISO_PIN,
    .sclk_io_num = NRF24_SCK_PIN,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
  };

  spi_device_interface_config_t dev_cfg = {
    .clock_speed_hz = 1000000,
    .mode = 0,
    .spics_io_num = -1,
    .queue_size = 7
  };

  spi_bus_initialize(NRF24_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  spi_bus_add_device(NRF24_SPI_HOST, &dev_cfg, &radio->spi);

  // Power up and configure nRF24L01+ as transmitter
  nrf24_write_register(radio, NRF24_REG_CONFIG, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  
  // Enable CRC, power up, primary transmitter mode
  nrf24_write_register(radio, NRF24_REG_CONFIG, NRF24_CONFIG_EN_CRC | NRF24_CONFIG_PWR_UP);
  vTaskDelay(pdMS_TO_TICKS(5));

  // Set data rate to 1Mbps, 0dBm power
  nrf24_write_register(radio, NRF24_REG_RF_SETUP, 0x06);

  // Set channel (2.4GHz + channel)
  nrf24_write_register(radio, NRF24_REG_RF_CH, 76);

  // Set address width to 5 bytes
  nrf24_write_register(radio, NRF24_REG_SETUP_AW, 0x03);

  // Set TX address
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_W_REGISTER | NRF24_REG_TX_ADDR);
  for (int i = 0; i < 5; i++) {
    spi_transfer(radio, radio->tx_address[i]);
  }
  gpio_set_level(radio->csn_pin, 1);

  // Set RX address for ACK
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_W_REGISTER | NRF24_REG_RX_ADDR_P0);
  for (int i = 0; i < 5; i++) {
    spi_transfer(radio, radio->tx_address[i]);
  }
  gpio_set_level(radio->csn_pin, 1);

  // Set payload size
  nrf24_write_register(radio, NRF24_REG_RX_PW_P0, NRF24_PAYLOAD_SIZE);

  // Configure auto retransmission: 750us delay, up to 3 retries
  nrf24_write_register(radio, NRF24_REG_SETUP_RETR, 0x2F);

  // Enable auto-acknowledgment on pipe 0
  nrf24_write_register(radio, NRF24_REG_EN_AA, 0x01);

  // Enable pipe 0
  nrf24_write_register(radio, NRF24_REG_EN_RXADDR, 0x01);

  // Clear status flags
  nrf24_write_register(radio, NRF24_REG_STATUS, NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);

  ESP_LOGI(TAG, "nRF24L01+ transmitter initialized successfully");
  radio->initialized = true;
  return true;
}

bool radio_send_command(RadioComm *radio, uint8_t command, uint16_t seconds, uint8_t sequence) {
  if (!radio->initialized) {
    ESP_LOGE(TAG, "Radio not initialized");
    return false;
  }

  // Flush any pending TX data
  radio_flush_tx(radio);

  // Prepare payload
  uint8_t payload[NRF24_PAYLOAD_SIZE];
  memset(payload, 0, NRF24_PAYLOAD_SIZE);
  payload[0] = command;
  payload[1] = (seconds >> 8) & 0xFF;
  payload[2] = seconds & 0xFF;
  payload[3] = sequence;

  // Set to transmitter mode (clear PRIM_RX bit)
  uint8_t config = nrf24_read_register(radio, NRF24_REG_CONFIG);
  config &= ~NRF24_CONFIG_PRIM_RX;
  nrf24_write_register(radio, NRF24_REG_CONFIG, config);
  vTaskDelay(pdMS_TO_TICKS(1));
  
  // Pulse CE to start transmission
  gpio_set_level(radio->ce_pin, 0);
  nrf24_write_payload(radio, payload, NRF24_PAYLOAD_SIZE);
  gpio_set_level(radio->ce_pin, 1);
  
  // Wait for transmission to complete (max 20ms to allow for retries)
  uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - start_time < 20) {
    uint8_t status = nrf24_get_status(radio);
    if (status & NRF24_STATUS_TX_DS) {
      // Transmission successful
      nrf24_write_register(radio, NRF24_REG_STATUS, NRF24_STATUS_TX_DS);
      gpio_set_level(radio->ce_pin, 0);
      ESP_LOGI(TAG, "Command sent: %d, seconds: %d, seq: %d", command, seconds, sequence);
      return true;
    }
    if (status & NRF24_STATUS_MAX_RT) {
      // Max retries reached
      nrf24_write_register(radio, NRF24_REG_STATUS, NRF24_STATUS_MAX_RT);
      gpio_set_level(radio->ce_pin, 0);
      ESP_LOGW(TAG, "Transmission failed - max retries");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // Timeout
  gpio_set_level(radio->ce_pin, 0);
  ESP_LOGW(TAG, "Transmission timeout");
  return false;
}

bool radio_is_transmit_complete(RadioComm* radio) {
  uint8_t status = nrf24_get_status(radio);
  return (status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0;
}

void radio_flush_tx(RadioComm* radio) {
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_FLUSH_TX);
  gpio_set_level(radio->csn_pin, 1);
  ESP_LOGD(TAG, "Flushing TX buffer");
}

void nrf24_power_up(RadioComm* radio) {
  uint8_t config = nrf24_read_register(radio, NRF24_REG_CONFIG);
  nrf24_write_register(radio, NRF24_REG_CONFIG, config | NRF24_CONFIG_PWR_UP);
  vTaskDelay(pdMS_TO_TICKS(5));
}

void nrf24_power_down(RadioComm* radio) {
  uint8_t config = nrf24_read_register(radio, NRF24_REG_CONFIG);
  nrf24_write_register(radio, NRF24_REG_CONFIG, config & ~NRF24_CONFIG_PWR_UP);
}

void radio_dump_registers(RadioComm* radio) {
  ESP_LOGI(TAG, "=== nRF24L01+ Register Dump ===");
  
  // Read and display key registers
  uint8_t config = nrf24_read_register(radio, NRF24_REG_CONFIG);
  uint8_t status = nrf24_get_status(radio);
  uint8_t rf_setup = nrf24_read_register(radio, NRF24_REG_RF_SETUP);
  uint8_t rf_ch = nrf24_read_register(radio, NRF24_REG_RF_CH);
  uint8_t setup_aw = nrf24_read_register(radio, NRF24_REG_SETUP_AW);
  uint8_t setup_retr = nrf24_read_register(radio, NRF24_REG_SETUP_RETR);
  uint8_t en_aa = nrf24_read_register(radio, NRF24_REG_EN_AA);
  uint8_t en_rxaddr = nrf24_read_register(radio, NRF24_REG_EN_RXADDR);
  uint8_t fifo_status = nrf24_read_register(radio, NRF24_REG_FIFO_STATUS);
  
  ESP_LOGI(TAG, "CONFIG:    0x%02X (PWR_UP:%d, PRIM_RX:%d, CRC:%d)", 
           config, 
           (config & NRF24_CONFIG_PWR_UP) ? 1 : 0,
           (config & NRF24_CONFIG_PRIM_RX) ? 1 : 0,
           (config & NRF24_CONFIG_EN_CRC) ? 1 : 0);
  
  ESP_LOGI(TAG, "STATUS:    0x%02X (TX_DS:%d, RX_DR:%d, MAX_RT:%d)", 
           status,
           (status & NRF24_STATUS_TX_DS) ? 1 : 0,
           (status & NRF24_STATUS_RX_DR) ? 1 : 0,
           (status & NRF24_STATUS_MAX_RT) ? 1 : 0);
  
  ESP_LOGI(TAG, "RF_SETUP:  0x%02X (RF_PWR:%d, RF_DR:%d)", 
           rf_setup,
           (rf_setup & NRF24_RF_SETUP_RF_PWR) >> 1,
           (rf_setup & NRF24_RF_SETUP_RF_DR) ? 2 : 1);
  
  ESP_LOGI(TAG, "RF_CH:     0x%02X (Channel: %d, Freq: %.3f GHz)", 
           rf_ch, rf_ch, 2.4 + rf_ch * 0.001);
  
  ESP_LOGI(TAG, "SETUP_AW:  0x%02X (Addr width: %d bytes)", setup_aw, setup_aw + 2);
  ESP_LOGI(TAG, "SETUP_RETR: 0x%02X (ARD:%d us, ARC:%d)", 
           setup_retr, 
           ((setup_retr & 0xF0) >> 4) * 250 + 250,
           setup_retr & 0x0F);
  ESP_LOGI(TAG, "EN_AA:     0x%02X", en_aa);
  ESP_LOGI(TAG, "EN_RXADDR: 0x%02X", en_rxaddr);
  ESP_LOGI(TAG, "FIFO_STATUS: 0x%02X", fifo_status);
  
  // Read TX address
  uint8_t tx_addr[5];
  gpio_set_level(radio->csn_pin, 0);
  spi_transfer(radio, NRF24_CMD_R_REGISTER | NRF24_REG_TX_ADDR);
  for (int i = 0; i < 5; i++) {
    tx_addr[i] = spi_transfer(radio, NRF24_CMD_NOP);
  }
  gpio_set_level(radio->csn_pin, 1);
  
  ESP_LOGI(TAG, "TX_ADDR:   %02X:%02X:%02X:%02X:%02X", 
           tx_addr[0], tx_addr[1], tx_addr[2], tx_addr[3], tx_addr[4]);
  
  // Check if module responds (basic connectivity test)
  if (config == 0x00 || config == 0xFF) {
    ESP_LOGE(TAG, "Radio module may not be connected (CONFIG: 0x%02X)", config);
  } else {
    ESP_LOGI(TAG, "Radio module appears to be connected");
  }
  
  ESP_LOGI(TAG, "=== End Register Dump ===");
}
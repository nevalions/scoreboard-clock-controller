#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

// Control commands
#define CMD_STOP        0
#define CMD_RUN         1
#define CMD_RESET       2

// Radio module configuration (nRF24L01+)
#define NRF24_CE_PIN      GPIO_NUM_5
#define NRF24_CSN_PIN     GPIO_NUM_4

// Link status LED
#define LINK_STATUS_LED_PIN GPIO_NUM_13

// SPI Configuration
#define NRF24_SPI_HOST    SPI2_HOST
#define NRF24_MOSI_PIN    GPIO_NUM_23
#define NRF24_MISO_PIN    GPIO_NUM_19
#define NRF24_SCK_PIN     GPIO_NUM_18

// nRF24L01+ Commands
#define NRF24_CMD_R_REGISTER    0x00
#define NRF24_CMD_W_REGISTER    0x20
#define NRF24_CMD_RX_PAYLOAD    0x61
#define NRF24_CMD_TX_PAYLOAD    0xA0
#define NRF24_CMD_FLUSH_TX      0xE1
#define NRF24_CMD_FLUSH_RX      0xE2
#define NRF24_CMD_REUSE_TX_PL   0xE3
#define NRF24_CMD_ACTIVATE      0x50
#define NRF24_CMD_R_RX_PL_WID   0x60
#define NRF24_CMD_W_TX_PAYLOAD_NOACK 0xB0
#define NRF24_CMD_W_ACK_PAYLOAD 0xA8
#define NRF24_CMD_NOP           0xFF

// nRF24L01+ Registers
#define NRF24_REG_CONFIG      0x00
#define NRF24_REG_EN_AA       0x01
#define NRF24_REG_EN_RXADDR   0x02
#define NRF24_REG_SETUP_AW    0x03
#define NRF24_REG_SETUP_RETR  0x04
#define NRF24_REG_RF_CH       0x05
#define NRF24_REG_RF_SETUP    0x06
#define NRF24_REG_STATUS      0x07
#define NRF24_REG_OBSERVE_TX  0x08
#define NRF24_REG_CD          0x09
#define NRF24_REG_RX_ADDR_P0  0x0A
#define NRF24_REG_RX_ADDR_P1  0x0B
#define NRF24_REG_RX_ADDR_P2  0x0C
#define NRF24_REG_RX_ADDR_P3  0x0D
#define NRF24_REG_RX_ADDR_P4  0x0E
#define NRF24_REG_RX_ADDR_P5  0x0F
#define NRF24_REG_TX_ADDR     0x10
#define NRF24_REG_RX_PW_P0    0x11
#define NRF24_REG_RX_PW_P1    0x12
#define NRF24_REG_RX_PW_P2    0x13
#define NRF24_REG_RX_PW_P3    0x14
#define NRF24_REG_RX_PW_P4    0x15
#define NRF24_REG_RX_PW_P5    0x16
#define NRF24_REG_FIFO_STATUS 0x17
#define NRF24_REG_DYNPD       0x1C
#define NRF24_REG_FEATURE     0x1D

// nRF24L01+ Configuration bits
#define NRF24_CONFIG_MASK_RX_DR  0x40
#define NRF24_CONFIG_MASK_TX_DS  0x20
#define NRF24_CONFIG_MASK_MAX_RT 0x10
#define NRF24_CONFIG_EN_CRC      0x08
#define NRF24_CONFIG_CRCO        0x04
#define NRF24_CONFIG_PWR_UP      0x02
#define NRF24_CONFIG_PRIM_RX     0x01

#define NRF24_STATUS_RX_DR       0x40
#define NRF24_STATUS_TX_DS       0x20
#define NRF24_STATUS_MAX_RT      0x10

#define NRF24_RF_SETUP_PLL_LOCK  0x10
#define NRF24_RF_SETUP_RF_DR     0x08
#define NRF24_RF_SETUP_RF_PWR    0x06
#define NRF24_RF_SETUP_LNA_HCURR 0x01

// Payload size
#define NRF24_PAYLOAD_SIZE 32

// Radio communication structure
typedef struct {
    bool initialized;
    gpio_num_t ce_pin;
    gpio_num_t csn_pin;
    spi_device_handle_t spi;

    // Radio configuration
    uint8_t tx_address[5];
    uint8_t rx_address[5];
    
    // Link status tracking
    uint32_t last_success_time;
    uint32_t last_failure_time;
    uint16_t success_count;
    uint16_t failure_count;
    bool link_good;
} RadioComm;

// Function declarations
bool radio_begin(RadioComm* radio, gpio_num_t ce, gpio_num_t csn);
bool radio_send_command(RadioComm* radio, uint8_t command, uint16_t seconds, uint8_t sequence);
bool radio_is_transmit_complete(RadioComm* radio);
void radio_flush_tx(RadioComm* radio);
void radio_dump_registers(RadioComm* radio);
void radio_update_link_status(RadioComm* radio);
bool radio_check_link_quality(RadioComm* radio);

// Low-level SPI functions
uint8_t nrf24_read_register(RadioComm* radio, uint8_t reg);
bool nrf24_write_register(RadioComm* radio, uint8_t reg, uint8_t value);
bool nrf24_write_payload(RadioComm* radio, uint8_t* data, uint8_t length);
uint8_t nrf24_get_status(RadioComm* radio);
void nrf24_power_up(RadioComm* radio);
void nrf24_power_down(RadioComm* radio);
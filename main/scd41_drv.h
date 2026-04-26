#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

#define SCD41_I2C_ADDR  0X62



esp_err_t scd41_i2c_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl);
esp_err_t scd41_i2c_scan(i2c_port_t port);
esp_err_t scd41_soft_reset(i2c_port_t port);
esp_err_t scd41_start_periodic(i2c_port_t port);
esp_err_t scd41_read_measurement(i2c_port_t port, uint16_t *co2_ppm, float *temp_c, float *rh_percent);
bool scd41_check_crc(const uint8_t *data, size_t word_start);
bool scd41_data_ready_check(i2c_port_t port);
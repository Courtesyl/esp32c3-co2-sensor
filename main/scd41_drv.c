#include "scd41_drv.h"
#include "esp_log.h"


static const char *TAG = "scd41_drv";

esp_err_t scd41_i2c_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl){

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    esp_err_t err = i2c_param_config(port, &conf);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_param_config fail: %d", err);
        return err;
    }
    err = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_driver_install fail: %d", err);
        return err;
    }
    ESP_LOGI(TAG, "i2c init port %d (SDA = %d, SCL = %d)", port, sda, scl);
    return ESP_OK;
}

esp_err_t scd41_i2c_scan(i2c_port_t port){
    ESP_LOGI(TAG, "scan start port %d", port);
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++){
        i2c_cmd_handle_t cmd  = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(port, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK){
            ESP_LOGI(TAG, "i2c device found on 0x%02x", addr);
            found++;
        }

    }
    if(found == 0){

        ESP_LOGW(TAG, "no i2c devices");
    }
    else{
        ESP_LOGI(TAG, "scan complete, %d devices found", found);
    }
    return ESP_OK;   
}

esp_err_t scd41_soft_reset(i2c_port_t port){
    ESP_LOGI(TAG, "SCD41 soft reset (0x3646)");
    uint8_t cmd[] = {0x36, 0x46};
    esp_err_t err = i2c_master_write_to_device(port, SCD41_I2C_ADDR, cmd, 2, 1000 / portTICK_PERIOD_MS);
    if (err == ESP_OK){
        vTaskDelay(pdMS_TO_TICKS(50));
        ESP_LOGI(TAG, "soft reset complete");
    } else{
        ESP_LOGE(TAG, "soft reset failed: %s", esp_err_to_name(err));
    }
    return err;
}
esp_err_t scd41_start_periodic(i2c_port_t port){
    ESP_LOGI(TAG, "scd41 start periodic measurement");
    uint8_t cmd[] = {0x21, 0xB1};
    esp_err_t err = i2c_master_write_to_device(port, SCD41_I2C_ADDR, cmd, 2, 1000 / portTICK_PERIOD_MS);
    if(err == ESP_OK){
        ESP_LOGI(TAG, "periodic measurement started");
    }
    else{
        ESP_LOGE(TAG, "periodic measurement failed: %s", esp_err_to_name(err));
    }
    return err;
}
bool scd41_check_crc(const uint8_t *data, size_t word_start){
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++){
        crc ^= data[word_start + i];
        for(int bit = 0; bit < 8; bit++){
            if (crc & 0x80){
                crc = (crc << 1) ^ 0x31;
            }else{
                crc <<= 1;
            }
        }
    }
    return crc == data[word_start + 2];
}
bool scd41_data_ready_check(i2c_port_t port){
    uint8_t cmd[] =  {0xE4, 0xB8};
    uint8_t data[3];
    esp_err_t err = i2c_master_write_read_device(port, SCD41_I2C_ADDR, cmd, 2, data, 3, 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK || !scd41_check_crc(data, 0)){
        ESP_LOGW(TAG, "data check failed");
        return false;
    }
    uint16_t status = (data[0] << 8) | data[1];
    bool ready = (status & 0x07FF) != 0;
    ESP_LOGD(TAG, "data ready: 0x%04x (%s)", status, ready? "READY": "WAIT");
    return ready;
}
esp_err_t scd41_read_measurement(i2c_port_t port, uint16_t *co2_ppm, float *temp_c, float *rh_percent){
    ESP_LOGI(TAG, "read measurement (0xEC05)");
    uint8_t cmd[] = {0xec, 0x05};
    uint8_t data[9];
    esp_err_t err = i2c_master_write_read_device(port, SCD41_I2C_ADDR, cmd, 2, data, 9, 1000 / portTICK_PERIOD_MS);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "read fail: %s", esp_err_to_name(err));
        return err;
    }
    if(!scd41_check_crc(data, 0)){
        ESP_LOGE(TAG, "scd41 co2 crc err");
        return ESP_FAIL;
    }
    if(!scd41_check_crc(data, 3)){
        ESP_LOGE(TAG, "scd41 temp crc err");
        return ESP_FAIL;
    }
    if(!scd41_check_crc(data, 6)){
        ESP_LOGE(TAG, "scd41 RH crc err");
        return ESP_FAIL;
    }
    *co2_ppm = (data[0] << 8) | data[1];
    *temp_c = -45.0f + 175.0f * ((data[3] << 8) | data[4]) / 65535.0f;
    *rh_percent = 100.0f * ((data[6] << 8) | data[7]) / 65535.0f;
    ESP_LOGI(TAG, "c02 = %u ppm, temp = %.1f°C, RH = %.1f%%", *co2_ppm, *temp_c, *rh_percent);
    return ESP_OK;
}

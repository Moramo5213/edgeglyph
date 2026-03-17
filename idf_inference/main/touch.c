#include "touch.h"

#include "driver/i2c.h"
#include "esp_err.h"

#define TOUCH_SDA 4
#define TOUCH_SCL 5
#define TOUCH_ADDR 0x15
#define I2C_MASTER_NUM I2C_NUM_0

void touch_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_SDA,
        .scl_io_num = TOUCH_SCL,
        .sda_pullup_en = 1,
        .scl_pullup_en = 1,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

bool touch_read(int *x, int *y) {
    uint8_t status, data[4];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x02, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &status, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS) != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        return false;
    }
    i2c_cmd_link_delete(cmd);
    if (status == 0) return false;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x03, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 4, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS) != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        return false;
    }
    i2c_cmd_link_delete(cmd);

    *x = ((data[0] & 0x0F) << 8) | data[1];
    *y = ((data[2] & 0x0F) << 8) | data[3];
    return true;
}

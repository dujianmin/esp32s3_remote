#include "cst816t.h"

uint8_t cst816t_read_len(uint16_t reg_addr, uint8_t *data, uint8_t len)
{
    uint8_t res = 0;
    res = i2c_master_write_read_device(I2C_PORT_NUM_TP, CST816T_ADDR, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);

    return res;
}

uint8_t cst816t_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_PORT_NUM_TP, CST816T_ADDR, write_buf, sizeof(write_buf), 1000 / portTICK_PERIOD_MS);

    return ret;
}

uint8_t cst816t_chipId(void)
{
    uint8_t id = 0;
    uint8_t res = 0;
    res = cst816t_read_len(0xA7, &id, 1);

    return 0;
}
static esp_err_t cst816t_get_touch_points_num(uint8_t *touch_points_num)
{
    uint8_t res = 0;
    res = cst816t_read_len(0x02, touch_points_num, 1);
    return res;
}

esp_err_t cst816t_read_pos(uint8_t *touch_points_num, uint16_t *x, uint16_t *y)
{
    uint8_t data[4];

    cst816t_get_touch_points_num(touch_points_num);
    if (0 == *touch_points_num)
    {
        *x = 0;
        *y = 0;
        return 1;
    }
    else
    {
        cst816t_read_len(0x03, data, 4);

        *x = ((data[0] & 0x0f) << 8) | data[1];
        *y = ((data[2] & 0x0f) << 8) | data[3];

        // printf("===========%d==========\n", *x);
    }

    return ESP_OK;
}
static esp_err_t i2c2_master_init(void)
{
    int i2c_master_port = I2C_PORT_NUM_TP;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_TP_I2C_SDA,
        .scl_io_num = GPIO_TP_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void cst816t_test_task(void *pvParameter)
{
    // i2c2_master_init();
    uint8_t res = 0;
    uint8_t touch_points_num = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    while (1)
    {

        res = cst816t_read_pos(&touch_points_num, &x, &y);
        if (res == ESP_OK)
        {

            // cst816t_debug("成功 点数:%d X:%d Y:%d \r\n", touch_points_num, x, y);

            if (x < 130 && y < 150)
            {

                cst816t_debug("区域: 左上\r\n");
            }
            else if (x < 130 && y >= 150)
            {
                cst816t_debug("区域: 左下\r\n");
            }
            else if (x >= 130 && y < 150)
            {
                cst816t_debug("区域: 右上\r\n");
            }
            else
            {
                cst816t_debug("区域: 右下\r\n");
            }
        }
        else
        {
            // cst816t_debug("失败 点数:%d X:%d Y:%d \r\n",touch_points_num,x,y);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}



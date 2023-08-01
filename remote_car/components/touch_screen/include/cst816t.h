#ifndef __cst816t__h
#define __cst816t__h

#include "driver/i2c.h"
#include <esp_log.h>
#define CST816T_ADDR 0X15

#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define I2C_PORT_NUM_TP     (0)
#define GPIO_TP_I2C_SCL     (GPIO_NUM_18)
#define GPIO_TP_I2C_SDA     (GPIO_NUM_17)

#if 1
#define my_debug( tag, format, ... ) \
ESP_LOGW(tag," %s:%d",__FILE__, __LINE__); \
printf(format,##__VA_ARGS__);
#elif 1
#define my_debug( tag, format, ... ) \
ESP_LOGW(tag,""); \
printf(format,##__VA_ARGS__);
#else
#define my_debug( tag, format, ... ) ;
#endif

#if 1
#define cst816t_debug(format, ...) my_debug("[ cst816t]- ",format,##__VA_ARGS__);
#else
#define cst816t_debug(format, ...) ;
#endif

void cst816t_test_task(void *pvParameter);


#endif

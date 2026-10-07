
#ifndef VEML6031_H
#define VEML6031_H

#include "main.h"

#define VEML_ADDR           0x29U

#define VEML_REG_CONF_0     0x00U
#define VEML_REG_CONF_1     0x01U
#define VEML_REG_THDH_L     0x04U
#define VEML_REG_THDL_L     0x06U
#define VEML_REG_ALS_L      0x10U
#define VEML_REG_IR_L       0x12U
#define VEML_REG_ID_L       0x14U
#define VEML_REG_INT_FLAG   0x17U

#define VEML_ID_VALUE       0x0001U

/* Thoi gian tich phan, CONF_0 bit 6:4 */
#define VEML_IT_3MS125      0U
#define VEML_IT_6MS25       1U
#define VEML_IT_12MS5       2U
#define VEML_IT_25MS        3U
#define VEML_IT_50MS        4U
#define VEML_IT_100MS       5U
#define VEML_IT_200MS       6U
#define VEML_IT_400MS       7U

/* Do khuech dai, CONF_1 bit 4:3 */
#define VEML_GAIN_X1        0U
#define VEML_GAIN_X2        1U
#define VEML_GAIN_X0_66     2U
#define VEML_GAIN_X0_5      3U

/* Kich thuoc diode hieu dung, CONF_1 bit 6 */
#define VEML_PD_FULL        0U   /* 4/4 */
#define VEML_PD_QUARTER     1U   /* 1/4, chia do nhay cho 4 */

#define VEML_I2C_TIMEOUT_MS 100U

HAL_StatusTypeDef veml_init(I2C_HandleTypeDef *hi2c, uint8_t it, uint8_t gain, uint8_t pddiv);
HAL_StatusTypeDef veml_shutdown(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef veml_check_id(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef veml_read_als(I2C_HandleTypeDef *hi2c, uint16_t *raw);
HAL_StatusTypeDef veml_read_ir(I2C_HandleTypeDef *hi2c, uint16_t *raw);
HAL_StatusTypeDef veml_read_lux(I2C_HandleTypeDef *hi2c, float *lux);

/* Do phan giai lx/count cua cau hinh dang dung, bang 11/12 datasheet */
float             veml_resolution(void);

/* Thoi gian toi thieu phai cho sau veml_init truoc khi so lieu co nghia */
uint32_t          veml_settle_ms(void);

#endif /* VEML6031_H */

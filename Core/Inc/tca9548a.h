
#ifndef TCA9548A_H
#define TCA9548A_H

#include "main.h"

#define TCA_MUX_SUN         0x70U
#define TCA_MUX_CES         0x71U
#define TCA_MUX_FES         0x72U

#define TCA_CH_COUNT        8U

#define TCA_I2C_TIMEOUT_MS  100U

HAL_StatusTypeDef tca_select(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t ch);
HAL_StatusTypeDef tca_release(I2C_HandleTypeDef *hi2c, uint8_t mux);
HAL_StatusTypeDef tca_write_mask(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t mask);
HAL_StatusTypeDef tca_read_mask(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t *mask);

#endif /* TCA9548A_H */

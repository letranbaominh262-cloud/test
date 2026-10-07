
#include "tca9548a.h"

HAL_StatusTypeDef tca_write_mask(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t mask)
{
    if (hi2c == NULL) return HAL_ERROR;
    return HAL_I2C_Master_Transmit(hi2c, (uint16_t)(mux << 1), &mask, 1U, TCA_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef tca_read_mask(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t *mask)
{
    if (hi2c == NULL || mask == NULL) return HAL_ERROR;
    return HAL_I2C_Master_Receive(hi2c, (uint16_t)(mux << 1), mask, 1U, TCA_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef tca_select(I2C_HandleTypeDef *hi2c, uint8_t mux, uint8_t ch)
{
    if (ch >= TCA_CH_COUNT) return HAL_ERROR;
    return tca_write_mask(hi2c, mux, (uint8_t)(1U << ch));
}

HAL_StatusTypeDef tca_release(I2C_HandleTypeDef *hi2c, uint8_t mux)
{
    return tca_write_mask(hi2c, mux, 0x00U);
}

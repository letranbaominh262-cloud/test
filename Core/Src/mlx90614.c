
#include "mlx90614.h"

static uint8_t mlx_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0U;
    uint8_t i, j;

    for (i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0U; j < 8U; j++)
        {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ MLX_PEC_POLY) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

HAL_StatusTypeDef mlx_read_raw(I2C_HandleTypeDef *hi2c, uint8_t cmd, uint16_t *raw)
{
    uint8_t rx[3];
    uint8_t pec_in[5];
    if (hi2c == NULL || raw == NULL) return HAL_ERROR;
    if (HAL_I2C_Mem_Read(hi2c, (uint16_t)(MLX_ADDR << 1), cmd, I2C_MEMADD_SIZE_8BIT, rx, 3U, MLX_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    pec_in[0] = (uint8_t)(MLX_ADDR << 1);
    pec_in[1] = cmd;
    pec_in[2] = (uint8_t)((MLX_ADDR << 1) | 1U);
    pec_in[3] = rx[0];
    pec_in[4] = rx[1];

    if (mlx_crc8(pec_in, 5U) != rx[2]) return HAL_ERROR;
    *raw = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    return HAL_OK;
}

HAL_StatusTypeDef mlx_read_object(I2C_HandleTypeDef *hi2c, float *temp_c)
{
    uint16_t raw;
    if (temp_c == NULL) return HAL_ERROR;
    if (mlx_read_raw(hi2c, MLX_RAM_TOBJ1, &raw) != HAL_OK) return HAL_ERROR;
    if ((raw & MLX_ERR_FLAG) != 0U)  return HAL_ERROR;
    if (raw < MLX_TOBJ_MIN_RAW)      return HAL_ERROR;
    *temp_c = ((float)raw * MLX_LSB_K) - MLX_KELVIN_0;
    return HAL_OK;
}

HAL_StatusTypeDef mlx_read_ambient(I2C_HandleTypeDef *hi2c, float *temp_c)
{
    uint16_t raw;
    if (temp_c == NULL) return HAL_ERROR;
    if (mlx_read_raw(hi2c, MLX_RAM_TA, &raw) != HAL_OK) return HAL_ERROR;
    if ((raw < MLX_TA_MIN_RAW) || (raw > MLX_TA_MAX_RAW)) return HAL_ERROR;
    *temp_c = ((float)raw * MLX_LSB_K) - MLX_KELVIN_0;
    return HAL_OK;
}

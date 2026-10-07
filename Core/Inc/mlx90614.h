
#ifndef MLX90614_H
#define MLX90614_H

#include "main.h"

#define MLX_ADDR            0x5AU

#define MLX_RAM_TA          0x06U
#define MLX_RAM_TOBJ1       0x07U
#define MLX_RAM_TOBJ2       0x08U

/* T(K) = raw x 0.02 */
#define MLX_LSB_K           0.02f
#define MLX_KELVIN_0        273.15f

#define MLX_ERR_FLAG        0x8000U

/* Gioi han hop le theo datasheet */
#define MLX_TOBJ_MIN_RAW    0x27ADU  /* -70.01 do C */
#define MLX_TA_MIN_RAW      0x2DE4U  /* -38.2  do C */
#define MLX_TA_MAX_RAW      0x4DC4U  /* +125   do C */

#define MLX_PEC_POLY        0x07U    /* CRC-8, x^8 + x^2 + x + 1 */
#define MLX_I2C_TIMEOUT_MS  100U
#define MLX_POR_DELAY_MS    250U

HAL_StatusTypeDef mlx_read_raw(I2C_HandleTypeDef *hi2c, uint8_t cmd, uint16_t *raw);
HAL_StatusTypeDef mlx_read_object(I2C_HandleTypeDef *hi2c, float *temp_c);
HAL_StatusTypeDef mlx_read_ambient(I2C_HandleTypeDef *hi2c, float *temp_c);

#endif /* MLX90614_H */

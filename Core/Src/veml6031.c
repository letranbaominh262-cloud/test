
#include "veml6031.h"

/* Nho lai cau hinh da ghi, de quy doi count sang lux */
static uint8_t cfg_it    = VEML_IT_6MS25;
static uint8_t cfg_gain  = VEML_GAIN_X0_5;
static uint8_t cfg_pddiv = VEML_PD_QUARTER;

/* Bang 11 datasheet, lx/count khi dung ca 4/4 diode.
 * Hang = IT (3.125 ms .. 400 ms), cot = gain (x1, x2, x0.66, x0.5) */
static const float res_tab[8][4] =
{
    { 0.6574f, 0.3287f, 0.9961f, 1.3148f },   /* 3.125 ms */
    { 0.3287f, 0.1644f, 0.4980f, 0.6574f },   /* 6.25 ms  */
    { 0.1644f, 0.0822f, 0.2490f, 0.3287f },   /* 12.5 ms  */
    { 0.0822f, 0.0411f, 0.1245f, 0.1644f },   /* 25 ms    */
    { 0.0411f, 0.0205f, 0.0623f, 0.0822f },   /* 50 ms    */
    { 0.0205f, 0.0103f, 0.0311f, 0.0411f },   /* 100 ms   */
    { 0.0103f, 0.0051f, 0.0156f, 0.0205f },   /* 200 ms   */
    { 0.0051f, 0.0026f, 0.0078f, 0.0103f }    /* 400 ms   */
};

/* IT ma hoa 0..7 doi ra mili giay, lam tron len */
static const uint16_t it_ms_tab[8] = { 4U, 7U, 13U, 25U, 50U, 100U, 200U, 400U };

static HAL_StatusTypeDef veml_read16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *val)
{
    uint8_t rx[2];

    if (hi2c == NULL || val == NULL) return HAL_ERROR;

    if (HAL_I2C_Mem_Read(hi2c, (uint16_t)(VEML_ADDR << 1), reg,
                         I2C_MEMADD_SIZE_8BIT, rx, 2U,
                         VEML_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *val = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);   /* byte thap truoc */
    return HAL_OK;
}

HAL_StatusTypeDef veml_init(I2C_HandleTypeDef *hi2c, uint8_t it, uint8_t gain, uint8_t pddiv)
{
    uint8_t cfg[2];

    if (hi2c == NULL || it > 7U || gain > 3U || pddiv > 1U) return HAL_ERROR;

    /* CONF_0: IT o bit 6:4, ALS_MODE = auto, ngat tat, ALS_ON_0 = 0 la BAT */
    cfg[0] = (uint8_t)(it << 4);

    /* CONF_1: ALS_ON_1 = 0 la BAT, PDDIV bit 6, GAIN bit 4:3,
     *         ALS_CAL bit 0 phai bang 1 sau khi cap nguon */
    cfg[1] = (uint8_t)((pddiv << 6) | (gain << 3) | 0x01U);

    /* Datasheet: CONF_0 va CONF_1 phai ghi cung mot lan, khong tach roi duoc.
     * Ghi lenh 0x00 kem 2 byte thi chip tu tang dia chi sang 0x01. */
    if (HAL_I2C_Mem_Write(hi2c, (uint16_t)(VEML_ADDR << 1), VEML_REG_CONF_0,
                          I2C_MEMADD_SIZE_8BIT, cfg, 2U,
                          VEML_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    cfg_it    = it;
    cfg_gain  = gain;
    cfg_pddiv = pddiv;

    return HAL_OK;
}

HAL_StatusTypeDef veml_shutdown(I2C_HandleTypeDef *hi2c)
{
    uint8_t cfg[2];
    if (hi2c == NULL) return HAL_ERROR;
    cfg[0] = (uint8_t)((cfg_it << 4) | 0x01U);
    cfg[1] = (uint8_t)(0x80U | (cfg_pddiv << 6) | (cfg_gain << 3) | 0x01U);
    return HAL_I2C_Mem_Write(hi2c, (uint16_t)(VEML_ADDR << 1), VEML_REG_CONF_0, I2C_MEMADD_SIZE_8BIT, cfg, 2U, VEML_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef veml_check_id(I2C_HandleTypeDef *hi2c)
{
    uint16_t id;
    if (veml_read16(hi2c, VEML_REG_ID_L, &id) != HAL_OK) return HAL_ERROR;
    return (id == VEML_ID_VALUE) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef veml_read_als(I2C_HandleTypeDef *hi2c, uint16_t *raw)
{
    return veml_read16(hi2c, VEML_REG_ALS_L, raw);
}

HAL_StatusTypeDef veml_read_ir(I2C_HandleTypeDef *hi2c, uint16_t *raw)
{
    return veml_read16(hi2c, VEML_REG_IR_L, raw);
}

float veml_resolution(void)
{
    float r = res_tab[cfg_it][cfg_gain];

    /* Dung 1/4 diode thi do nhay giam dung 4 lan */
    return (cfg_pddiv == VEML_PD_QUARTER) ? (r * 4.0f) : r;
}

HAL_StatusTypeDef veml_read_lux(I2C_HandleTypeDef *hi2c, float *lux)
{
    uint16_t raw;
    if (lux == NULL) return HAL_ERROR;
    if (veml_read_als(hi2c, &raw) != HAL_OK) return HAL_ERROR;
    *lux = (float)raw * veml_resolution();
    return HAL_OK;
}

uint32_t veml_settle_ms(void)
{
    /* Cho 2 chu ky tich phan cho chac */
    return (uint32_t)it_ms_tab[cfg_it] * 2U + 5U;
}

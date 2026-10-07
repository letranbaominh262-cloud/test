
#include "drv8908.h"

static uint8_t op_ctrl1_shadow;
static uint8_t op_ctrl2_shadow;
static uint8_t pwm_ctrl1_shadow;
static uint8_t coil_hb_p(uint8_t coil) { return (uint8_t)(2U * coil - 1U); }
static uint8_t coil_hb_n(uint8_t coil) { return (uint8_t)(2U * coil);      }

static HAL_StatusTypeDef drv_xfer(SPI_HandleTypeDef *hspi, uint16_t tx, uint16_t *rx)
{
    HAL_StatusTypeDef st;
    uint16_t rx_tmp = 0;

    HAL_GPIO_WritePin(DRV_CS_PORT, DRV_CS_PIN, GPIO_PIN_RESET);
    st = HAL_SPI_TransmitReceive(hspi, (uint8_t *)&tx, (uint8_t *)&rx_tmp, 1U, DRV_SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(DRV_CS_PORT, DRV_CS_PIN, GPIO_PIN_SET);

    if (rx != NULL) *rx = rx_tmp;
    return st;
}

HAL_StatusTypeDef drv8908_write_reg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t val)
{
    if (hspi == NULL) return HAL_ERROR;
    // bit14 = 0 -> Write
    return drv_xfer(hspi, (uint16_t)(((uint16_t)(reg & 0x3FU) << 8) | val), NULL);
}

HAL_StatusTypeDef drv8908_read_reg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *val)
{
    uint16_t rx = 0;

    if (hspi == NULL || val == NULL) return HAL_ERROR;

    // bit14 = 1 -> read
    if (drv_xfer(hspi, (uint16_t)(0x4000U | ((uint16_t)(reg & 0x3FU) << 8)), &rx) != HAL_OK)
    {
        return HAL_ERROR;
    }
    *val = (uint8_t)(rx & 0xFFU);
    return HAL_OK;
}

HAL_StatusTypeDef drv8908_get_status(SPI_HandleTypeDef *hspi, uint8_t *ic_stat)
{
    return drv8908_read_reg(hspi, DRV_REG_IC_STAT, ic_stat);
}

uint8_t drv8908_fault_pin(void)
{
    return (HAL_GPIO_ReadPin(DRV_FAULT_PORT, DRV_FAULT_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
}

HAL_StatusTypeDef drv8908_clear_faults(SPI_HandleTypeDef *hspi)
{
    uint8_t cfg;

    if (hspi == NULL) return HAL_ERROR;
    if (drv8908_read_reg(hspi, DRV_REG_CONFIG_CTRL, &cfg) != HAL_OK) return HAL_ERROR;
    cfg = (uint8_t)((cfg & (uint8_t)~DRV_CFG_IC_ID_MASK) | DRV_CFG_CLR_FLT);
    return drv8908_write_reg(hspi, DRV_REG_CONFIG_CTRL, cfg);
}

HAL_StatusTypeDef drv8908_init(SPI_HandleTypeDef *hspi)
{
    uint8_t cfg;

    if (hspi == NULL) return HAL_ERROR;

    HAL_GPIO_WritePin(DRV_CS_PORT, DRV_CS_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(DRV_SLEEP_PORT, DRV_SLEEP_PIN, GPIO_PIN_SET);
    HAL_Delay(DRV_WAKE_DELAY_MS);

    if (drv8908_clear_faults(hspi) != HAL_OK) return HAL_ERROR;

    if (drv8908_read_reg(hspi, DRV_REG_CONFIG_CTRL, &cfg) != HAL_OK) return HAL_ERROR;
    if ((cfg & DRV_CFG_IC_ID_MASK) != DRV_CFG_IC_ID_DRV8908) return HAL_ERROR;

    op_ctrl1_shadow  = 0x00U;
    op_ctrl2_shadow  = 0x00U;
    pwm_ctrl1_shadow = 0x00U;

    if (drv8908_write_reg(hspi, DRV_REG_OP_CTRL_1,  op_ctrl1_shadow)  != HAL_OK) return HAL_ERROR;
    if (drv8908_write_reg(hspi, DRV_REG_OP_CTRL_2,  op_ctrl2_shadow)  != HAL_OK) return HAL_ERROR;
    if (drv8908_write_reg(hspi, DRV_REG_PWM_CTRL_1, pwm_ctrl1_shadow) != HAL_OK) return HAL_ERROR;

    if (drv8908_write_reg(hspi, DRV_REG_PWM_MAP_CTRL_1, 0x08U) != HAL_OK) return HAL_ERROR; // HB2=CH2, HB1=CH1
    if (drv8908_write_reg(hspi, DRV_REG_PWM_MAP_CTRL_2, 0x1AU) != HAL_OK) return HAL_ERROR; // HB4=CH4, HB3=CH3
    if (drv8908_write_reg(hspi, DRV_REG_PWM_MAP_CTRL_3, 0x2CU) != HAL_OK) return HAL_ERROR; // HB6=CH6, HB5=CH5

    if (drv8908_write_reg(hspi, DRV_REG_OLD_CTRL_2, DRV_OLD_OP_KEEP_DRIVING) != HAL_OK) return HAL_ERROR;
    if (drv8908_write_reg(hspi, DRV_REG_OLD_CTRL_1, DRV_OLD_DIS_HB7 | DRV_OLD_DIS_HB8) != HAL_OK) return HAL_ERROR;	    // Turnoff open load on HB7, HB8

    return HAL_OK;
}

HAL_StatusTypeDef drv8908_sleep(SPI_HandleTypeDef *hspi)
{
    (void)drv8908_all_off(hspi);
    HAL_GPIO_WritePin(DRV_SLEEP_PORT, DRV_SLEEP_PIN, GPIO_PIN_RESET);
    return HAL_OK;
}

static void hb_set_bits(uint8_t hb, uint8_t hs, uint8_t ls)
{
    uint8_t *shadow = (hb <= 4U) ? &op_ctrl1_shadow : &op_ctrl2_shadow;
    uint8_t  pos    = (uint8_t)(((hb - 1U) % 4U) * 2U);      /* 0, 2, 4, 6 */

    *shadow &= (uint8_t)~(0x3U << pos);
    *shadow |= (uint8_t)(((hs ? 2U : 0U) | (ls ? 1U : 0U)) << pos);
}

HAL_StatusTypeDef drv8908_coil_set(SPI_HandleTypeDef *hspi, uint8_t coil, drv_coil_mode_t mode)
{
    uint8_t hb_p, hb_n;

    if (hspi == NULL || coil < 1U || coil > DRV_COIL_COUNT) return HAL_ERROR;

    hb_p = coil_hb_p(coil);
    hb_n = coil_hb_n(coil);

    switch (mode)
    {
        case DRV_COIL_FWD:
            hb_set_bits(hb_p, 1U, 0U);
            hb_set_bits(hb_n, 0U, 1U);
            break;

        case DRV_COIL_REV:
            hb_set_bits(hb_p, 0U, 1U);
            hb_set_bits(hb_n, 1U, 0U);
            break;

        case DRV_COIL_BRAKE:
            hb_set_bits(hb_p, 0U, 1U);
            hb_set_bits(hb_n, 0U, 1U);
            break;

        case DRV_COIL_OFF:
        default:
            hb_set_bits(hb_p, 0U, 0U);
            hb_set_bits(hb_n, 0U, 0U);
            break;
    }
    if (coil <= 2U)
    {
        return drv8908_write_reg(hspi, DRV_REG_OP_CTRL_1, op_ctrl1_shadow);
    }
    return drv8908_write_reg(hspi, DRV_REG_OP_CTRL_2, op_ctrl2_shadow);
}

HAL_StatusTypeDef drv8908_coil_duty(SPI_HandleTypeDef *hspi, uint8_t coil, uint8_t duty)
{
    uint8_t hb_p, hb_n, ch_p;

    if (hspi == NULL || coil < 1U || coil > DRV_COIL_COUNT) return HAL_ERROR;

    hb_p = coil_hb_p(coil);
    hb_n = coil_hb_n(coil);
    ch_p = hb_p;

    if (duty == 0U)
    {
        pwm_ctrl1_shadow &= (uint8_t)~((1U << (hb_p - 1U)) | (1U << (hb_n - 1U)));
        return drv8908_write_reg(hspi, DRV_REG_PWM_CTRL_1, pwm_ctrl1_shadow);
    }

    if (drv8908_write_reg(hspi, (uint8_t)(DRV_REG_PWM_DUTY_CH1 + ch_p - 1U), duty) != HAL_OK)
    {
        return HAL_ERROR;
    }

    pwm_ctrl1_shadow &= (uint8_t)~(1U << (hb_n - 1U));
    pwm_ctrl1_shadow |= (uint8_t)(1U << (hb_p - 1U));

    return drv8908_write_reg(hspi, DRV_REG_PWM_CTRL_1, pwm_ctrl1_shadow);
}

HAL_StatusTypeDef drv8908_coil_freq(SPI_HandleTypeDef *hspi, uint8_t coil, uint8_t freq_sel)
{
    uint8_t reg, val, ch, pos;

    if (hspi == NULL || coil < 1U || coil > DRV_COIL_COUNT || freq_sel > 3U) return HAL_ERROR;

    ch  = coil_hb_p(coil);
    reg = (ch <= 4U) ? DRV_REG_PWM_FREQ_CTRL_1 : DRV_REG_PWM_FREQ_CTRL_2;
    pos = (uint8_t)(((ch - 1U) % 4U) * 2U);

    if (drv8908_read_reg(hspi, reg, &val) != HAL_OK) return HAL_ERROR;

    val &= (uint8_t)~(0x3U << pos);
    val |= (uint8_t)(freq_sel << pos);

    return drv8908_write_reg(hspi, reg, val);
}

HAL_StatusTypeDef drv8908_all_off(SPI_HandleTypeDef *hspi)
{
    if (hspi == NULL) return HAL_ERROR;

    op_ctrl1_shadow  = 0x00U;
    op_ctrl2_shadow  = 0x00U;
    pwm_ctrl1_shadow = 0x00U;

    if (drv8908_write_reg(hspi, DRV_REG_OP_CTRL_1,  op_ctrl1_shadow)  != HAL_OK) return HAL_ERROR;
    if (drv8908_write_reg(hspi, DRV_REG_OP_CTRL_2,  op_ctrl2_shadow)  != HAL_OK) return HAL_ERROR;

    return drv8908_write_reg(hspi, DRV_REG_PWM_CTRL_1, pwm_ctrl1_shadow);
}

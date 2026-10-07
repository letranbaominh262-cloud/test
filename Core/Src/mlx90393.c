
#include "mlx90393.h"

#include <math.h>

/* Nho lai cau hinh da ghi, de quy doi count sang micro Tesla */
static uint8_t cfg_gain = M393_GAIN_DEF;
static uint8_t cfg_res  = M393_RES_DEF;

/* Bang 18 datasheet, uT/LSB, HALLCONF = 0xC, TC tat, 35 do C.
 * Hang = GAIN_SEL 0..7, cot = RES 0..3 */
static const float sens_xy[8][4] =
{
    { 0.787f, 1.573f, 3.146f, 6.292f },
    { 0.629f, 1.258f, 2.517f, 5.034f },
    { 0.472f, 0.944f, 1.888f, 3.775f },
    { 0.393f, 0.787f, 1.573f, 3.146f },
    { 0.315f, 0.629f, 1.258f, 2.517f },
    { 0.262f, 0.524f, 1.049f, 2.097f },
    { 0.210f, 0.419f, 0.839f, 1.678f },
    { 0.157f, 0.315f, 0.629f, 1.258f }
};

static const float sens_z[8][4] =
{
    { 1.267f, 2.534f,  5.068f, 10.137f },
    { 1.014f, 2.027f,  4.055f,  8.109f },
    { 0.760f, 1.521f,  3.041f,  6.082f },
    { 0.634f, 1.267f,  2.534f,  5.068f },
    { 0.507f, 1.014f,  2.027f,  4.055f },
    { 0.422f, 0.845f,  1.689f,  3.379f },
    { 0.338f, 0.676f,  1.352f,  2.703f },
    { 0.253f, 0.507f,  1.014f,  2.027f }
};

/* Gui mot khung lenh roi doc lai n byte tra loi. Byte dau luon la trang thai. */
static HAL_StatusTypeDef m393_xfer(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                   const uint8_t *tx, uint8_t tx_len,
                                   uint8_t *rx, uint8_t rx_len)
{
    if (hi2c == NULL || tx == NULL || rx == NULL) return HAL_ERROR;

    if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(addr << 1), (uint8_t *)tx, tx_len,
                                M393_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Master_Receive(hi2c, (uint16_t)(addr << 1), rx, rx_len,
                                  M393_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef m393_reset(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint8_t tx = M393_CMD_RT;
    uint8_t st;

    if (m393_xfer(hi2c, addr, &tx, 1U, &st, 1U) != HAL_OK) return HAL_ERROR;

    HAL_Delay(2);   /* TPOR toi da 1.5 ms */
    return HAL_OK;
}

HAL_StatusTypeDef m393_read_reg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint16_t *val)
{
    uint8_t tx[2];
    uint8_t rx[3];

    if (val == NULL) return HAL_ERROR;

    /* Dia chi thanh ghi phai dich trai 2 bit */
    tx[0] = M393_CMD_RR;
    tx[1] = (uint8_t)(reg << 2);

    if (m393_xfer(hi2c, addr, tx, 2U, rx, 3U) != HAL_OK) return HAL_ERROR;
    if ((rx[0] & M393_ST_ERROR) != 0U) return HAL_ERROR;

    *val = ((uint16_t)rx[1] << 8) | (uint16_t)rx[2];   /* byte cao truoc */
    return HAL_OK;
}

HAL_StatusTypeDef m393_write_reg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint16_t val)
{
    uint8_t tx[4];
    uint8_t st;

    tx[0] = M393_CMD_WR;
    tx[1] = (uint8_t)(val >> 8);
    tx[2] = (uint8_t)(val & 0xFFU);
    tx[3] = (uint8_t)(reg << 2);

    if (m393_xfer(hi2c, addr, tx, 4U, &st, 1U) != HAL_OK) return HAL_ERROR;

    return ((st & M393_ST_ERROR) != 0U) ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef m393_check(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint16_t c1;
    uint8_t  hallconf, gain;

    if (m393_read_reg(hi2c, addr, M393_REG_CONF1, &c1) != HAL_OK) return HAL_ERROR;

    hallconf = (uint8_t)((c1 & M393_C1_HALLCONF_MSK) >> M393_C1_HALLCONF_POS);
    gain     = (uint8_t)((c1 & M393_C1_GAIN_MSK)     >> M393_C1_GAIN_POS);

    /* Sau khi cap nguon, hai truong nay phai bang gia tri mac dinh cua
     * datasheet. Trat la ban do bit dat sai cho, khong phai chip hong. */
    return ((hallconf == M393_HALLCONF_DEF) && (gain == M393_GAIN_DEF))
           ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef m393_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t gain, uint8_t res)
{
    uint16_t c1, c3;

    if (gain > 7U || res > 3U) return HAL_ERROR;

    /* CONF1: chi doi GAIN_SEL, giu nguyen HALLCONF va cac bit khac */
    if (m393_read_reg(hi2c, addr, M393_REG_CONF1, &c1) != HAL_OK) return HAL_ERROR;
    c1 = (uint16_t)((c1 & (uint16_t)~M393_C1_GAIN_MSK)
                    | ((uint16_t)gain << M393_C1_GAIN_POS));
    if (m393_write_reg(hi2c, addr, M393_REG_CONF1, c1) != HAL_OK) return HAL_ERROR;

    /* CONF3: dat cung mot RES cho ca ba truc, giu nguyen DIG_FILT va OSR */
    if (m393_read_reg(hi2c, addr, M393_REG_CONF3, &c3) != HAL_OK) return HAL_ERROR;
    c3 = (uint16_t)(c3 & (uint16_t)~M393_C3_RES_MSK);
    c3 = (uint16_t)(c3 | ((uint16_t)res << M393_C3_RESX_POS)
                       | ((uint16_t)res << M393_C3_RESY_POS)
                       | ((uint16_t)res << M393_C3_RESZ_POS));
    if (m393_write_reg(hi2c, addr, M393_REG_CONF3, c3) != HAL_OK) return HAL_ERROR;

    cfg_gain = gain;
    cfg_res  = res;

    return HAL_OK;
}

/* Doi 16 bit tho ra so co dau, theo dung kieu ma RES_XYZ quy dinh */
static int32_t m393_decode(uint16_t raw)
{
    /* RES 0 va 1: bu hai. RES 2 va 3: khong dau, diem 0 nam o 32768 */
    if (cfg_res <= 1U)
    {
        return (int32_t)(int16_t)raw;
    }
    return (int32_t)raw - 32768;
}

HAL_StatusTypeDef m393_measure(I2C_HandleTypeDef *hi2c, uint8_t addr,
                               int32_t *x, int32_t *y, int32_t *z)
{
    uint8_t tx;
    uint8_t rx[7];
    uint8_t st;
    uint8_t i;

    if (x == NULL || y == NULL || z == NULL) return HAL_ERROR;

    /* 1. Ra lenh do mot lan cho ca ba truc */
    tx = (uint8_t)(M393_CMD_SM | M393_AXIS_XYZ);
    if (m393_xfer(hi2c, addr, &tx, 1U, &st, 1U) != HAL_OK) return HAL_ERROR;
    if ((st & M393_ST_ERROR) != 0U) return HAL_ERROR;

    /* 2. Doc ket qua. Doc som thi bit ERROR len, cu thu lai cho den khi xong.
     * Cach nay tu thich nghi voi moi cau hinh OSR va DIG_FILT. */
    tx = (uint8_t)(M393_CMD_RM | M393_AXIS_XYZ);

    for (i = 0U; i < M393_CONV_RETRY; i++)
    {
        HAL_Delay(M393_CONV_STEP_MS);

        if (m393_xfer(hi2c, addr, &tx, 1U, rx, 7U) != HAL_OK) return HAL_ERROR;

        if ((rx[0] & M393_ST_ERROR) == 0U)
        {
            /* Thu tu tra ve la X, Y, Z, moi truc byte cao truoc */
            *x = m393_decode(((uint16_t)rx[1] << 8) | (uint16_t)rx[2]);
            *y = m393_decode(((uint16_t)rx[3] << 8) | (uint16_t)rx[4]);
            *z = m393_decode(((uint16_t)rx[5] << 8) | (uint16_t)rx[6]);
            return HAL_OK;
        }
    }

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef m393_read_ut(I2C_HandleTypeDef *hi2c, uint8_t addr,
                               float *bx, float *by, float *bz)
{
    int32_t rx, ry, rz;

    if (bx == NULL || by == NULL || bz == NULL) return HAL_ERROR;
    if (m393_measure(hi2c, addr, &rx, &ry, &rz) != HAL_OK) return HAL_ERROR;

    *bx = (float)rx * sens_xy[cfg_gain][cfg_res];
    *by = (float)ry * sens_xy[cfg_gain][cfg_res];
    *bz = (float)rz * sens_z[cfg_gain][cfg_res];

    return HAL_OK;
}

float m393_norm_ut(float bx, float by, float bz)
{
    return sqrtf((bx * bx) + (by * by) + (bz * bz));
}

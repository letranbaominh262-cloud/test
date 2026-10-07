
#ifndef MLX90393_H
#define MLX90393_H

#include "main.h"

/* Tu ke 3 truc Melexis MLX90393 (Triaxis, Hall effect), tren board MTQ_V10.
 * Noi vao I2C3. Dung tien to M393_ de khong dung voi mlx90614.h.
 *
 * Con nay KHONG giong cac chip I2C thong thuong: no khong co dia chi thanh ghi
 * de doc thang. Moi giao dich la mot LENH, va lenh nao cung tra ve mot byte
 * trang thai. Muon do phai ra lenh SM roi doi, sau do ra lenh RM de lay so lieu.
 *
 * So lieu ve la 3 so 16 bit X, Y, Z, byte cao truoc. Doi ra micro Tesla bang
 * cach nhan voi do nhay tra bang 18 datasheet, phu thuoc GAIN_SEL va RES_XYZ.
 *
 * BAY: kieu so doi theo RES_XYZ
 *   RES 0, 1 -> bu hai, co dau, 0 uT ung voi 0
 *   RES 2, 3 -> khong dau, 0 uT ung voi 32768, phai tru di
 * Bat TCMP_EN cung lam RES 0 va 1 chuyen sang khong dau. Thu vien nay
 * de TCMP_EN = 0.
 */

/* Dia chi 7 bit, 2 bit thap do chan A0/A1 quyet dinh (ban ABA-011-RE) */
#define M393_ADDR_00        0x0CU
#define M393_ADDR_01        0x0DU
#define M393_ADDR_10        0x0EU
#define M393_ADDR_11        0x0FU

/* Lenh. Bon lenh dau con OR them mat na truc ben duoi */
#define M393_CMD_SB         0x10U   /* Start Burst              */
#define M393_CMD_SM         0x30U   /* Start Single Measurement */
#define M393_CMD_RM         0x40U   /* Read Measurement         */
#define M393_CMD_RR         0x50U   /* Read Register            */
#define M393_CMD_WR         0x60U   /* Write Register           */
#define M393_CMD_EX         0x80U   /* Exit mode                */
#define M393_CMD_HR         0xD0U   /* Memory Recall            */
#define M393_CMD_RT         0xF0U   /* Reset                    */

/* Mat na truc, dung chung cho SB / SM / RM */
#define M393_AXIS_T         0x01U
#define M393_AXIS_X         0x02U
#define M393_AXIS_Y         0x04U
#define M393_AXIS_Z         0x08U
#define M393_AXIS_XYZ       (M393_AXIS_X | M393_AXIS_Y | M393_AXIS_Z)

/* Bit trong byte trang thai */
#define M393_ST_BURST       0x80U
#define M393_ST_WOC         0x40U
#define M393_ST_SM          0x20U
#define M393_ST_ERROR       0x10U
#define M393_ST_SED         0x08U
#define M393_ST_RS          0x04U

/* Thanh ghi cau hinh */
#define M393_REG_CONF1      0x00U   /* HALLCONF, GAIN_SEL, Z_SERIES, BIST   */
#define M393_REG_CONF2      0x01U   /* BURST_SEL, TCMP_EN, COMM_MODE, ...   */
#define M393_REG_CONF3      0x02U   /* RES_X/Y/Z, DIG_FILT, OSR, OSR2       */

/* Vi tri bit trong CONF1 va CONF3 */
#define M393_C1_HALLCONF_POS    0U
#define M393_C1_HALLCONF_MSK    0x000FU
#define M393_C1_GAIN_POS        4U
#define M393_C1_GAIN_MSK        0x0070U

#define M393_C3_RESX_POS        5U
#define M393_C3_RESY_POS        7U
#define M393_C3_RESZ_POS        9U
#define M393_C3_RES_MSK         0x07E0U   /* ca 6 bit RES_X, RES_Y, RES_Z   */

/* Gia tri mac dinh theo datasheet, dung de tu kiem tra */
#define M393_HALLCONF_DEF       0x0CU
#define M393_GAIN_DEF           0x07U
#define M393_RES_DEF            0x00U

#define M393_I2C_TIMEOUT_MS     100U
#define M393_CONV_RETRY         20U     /* so lan thu lai khi so lieu chua san sang */
#define M393_CONV_STEP_MS       2U

HAL_StatusTypeDef m393_reset(I2C_HandleTypeDef *hi2c, uint8_t addr);
HAL_StatusTypeDef m393_read_reg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint16_t *val);
HAL_StatusTypeDef m393_write_reg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint16_t val);

/* Doc CONF1 va doi chieu voi gia tri mac dinh cua datasheet.
 * Dung de xac nhan chip co mat VA ban do bit trong thu vien dat dung cho. */
HAL_StatusTypeDef m393_check(I2C_HandleTypeDef *hi2c, uint8_t addr);

/* gain 0..7, res 0..3. Dat cho ca ba truc cung mot res. */
HAL_StatusTypeDef m393_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t gain, uint8_t res);

/* Ra lenh do roi doi lay ket qua. Tra ve so da bu dau, chua doi don vi. */
HAL_StatusTypeDef m393_measure(I2C_HandleTypeDef *hi2c, uint8_t addr,
                               int32_t *x, int32_t *y, int32_t *z);

/* Nhu tren nhung tra ve micro Tesla */
HAL_StatusTypeDef m393_read_ut(I2C_HandleTypeDef *hi2c, uint8_t addr,
                               float *bx, float *by, float *bz);

/* Do lon vector, de kiem tra: xoay board thi gia tri nay phai gan nhu khong doi */
float             m393_norm_ut(float bx, float by, float bz);

#endif /* MLX90393_H */

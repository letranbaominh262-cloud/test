
#ifndef DRV8908_H
#define DRV8908_H

#include "main.h"

// Register
#define DRV_REG_IC_STAT         0x00U
#define DRV_REG_OCP_STAT_1      0x01U
#define DRV_REG_OCP_STAT_2      0x02U
#define DRV_REG_OLD_STAT_1      0x04U
#define DRV_REG_OLD_STAT_2      0x05U
#define DRV_REG_CONFIG_CTRL     0x07U
#define DRV_REG_OP_CTRL_1       0x08U   // HB1..HB4
#define DRV_REG_OP_CTRL_2       0x09U   // HB5..HB8
#define DRV_REG_PWM_CTRL_1      0x0BU   // enable pwm
#define DRV_REG_PWM_CTRL_2      0x0CU
#define DRV_REG_FW_CTRL_1       0x0DU
#define DRV_REG_PWM_MAP_CTRL_1  0x0FU   // HB1, HB2
#define DRV_REG_PWM_MAP_CTRL_2  0x10U   // HB3, HB4
#define DRV_REG_PWM_MAP_CTRL_3  0x11U   // HB5, HB6
#define DRV_REG_PWM_MAP_CTRL_4  0x12U   // HB7, HB8
#define DRV_REG_PWM_FREQ_CTRL_1 0x13U   // channel PWM 1..4
#define DRV_REG_PWM_FREQ_CTRL_2 0x14U   // channel PWM 5..8
#define DRV_REG_PWM_DUTY_CH1    0x15U   // CH2..CH8 = 0x16..0x1C
#define DRV_REG_SR_CTRL_1       0x1DU
#define DRV_REG_OLD_CTRL_1      0x1FU   // HBx_OLD_DIS: 1 = off open load
#define DRV_REG_OLD_CTRL_2      0x20U

// OLD_CTRL_1
#define DRV_OLD_DIS_HB1         (1U << 0)
#define DRV_OLD_DIS_HB2         (1U << 1)
#define DRV_OLD_DIS_HB3         (1U << 2)
#define DRV_OLD_DIS_HB4         (1U << 3)
#define DRV_OLD_DIS_HB5         (1U << 4)
#define DRV_OLD_DIS_HB6         (1U << 5)
#define DRV_OLD_DIS_HB7         (1U << 6)
#define DRV_OLD_DIS_HB8         (1U << 7)
#define DRV_OLD_DIS_ALL         0xFFU

/* OLD_CTRL_2 */
#define DRV_OLD_OP_KEEP_DRIVING (1U << 6)   // OLD_OP
#define DRV_OLD_NO_NFAULT       (1U << 7)   // OLD_REP

// IC_STAT
#define DRV_STAT_OTSD           (1U << 6)
#define DRV_STAT_OTW            (1U << 5)
#define DRV_STAT_OLD            (1U << 4)
#define DRV_STAT_OCP            (1U << 3)
#define DRV_STAT_UVLO           (1U << 2)
#define DRV_STAT_OVP            (1U << 1)
#define DRV_STAT_NPOR           (1U << 0)

// CONFIG_CTRL
#define DRV_CFG_CLR_FLT         (1U << 0)
#define DRV_CFG_EXT_OVP         (1U << 1)
#define DRV_CFG_OTW_REP         (1U << 2)
#define DRV_CFG_OCP_REP         (1U << 3)
#define DRV_CFG_IC_ID_MASK      0x70U
#define DRV_CFG_IC_ID_DRV8908   0x20U

// Frequency PWM
#define DRV_PWM_FREQ_80HZ       0U
#define DRV_PWM_FREQ_100HZ      1U
#define DRV_PWM_FREQ_200HZ      2U
#define DRV_PWM_FREQ_2000HZ     3U

// Direct current
typedef enum {
    DRV_COIL_OFF = 0,
    DRV_COIL_FWD,
    DRV_COIL_REV,
    DRV_COIL_BRAKE
} drv_coil_mode_t;

#define DRV_COIL_COUNT          3U

#define DRV_SPI_TIMEOUT_MS      100U
#define DRV_WAKE_DELAY_MS       1U

// Pin
#define DRV_CS_PORT             GPIOA
#define DRV_CS_PIN              GPIO_PIN_15
#define DRV_SLEEP_PORT          GPIOC
#define DRV_SLEEP_PIN           GPIO_PIN_7
#define DRV_FAULT_PORT          GPIOC
#define DRV_FAULT_PIN           GPIO_PIN_8

HAL_StatusTypeDef drv8908_write_reg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t val);
HAL_StatusTypeDef drv8908_read_reg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *val);
HAL_StatusTypeDef drv8908_init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef drv8908_sleep(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef drv8908_clear_faults(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef drv8908_get_status(SPI_HandleTypeDef *hspi, uint8_t *ic_stat);
uint8_t           drv8908_fault_pin(void);
HAL_StatusTypeDef drv8908_coil_set(SPI_HandleTypeDef *hspi, uint8_t coil, drv_coil_mode_t mode);
HAL_StatusTypeDef drv8908_coil_duty(SPI_HandleTypeDef *hspi, uint8_t coil, uint8_t duty);   /* 0..255 */
HAL_StatusTypeDef drv8908_coil_freq(SPI_HandleTypeDef *hspi, uint8_t coil, uint8_t freq_sel);
HAL_StatusTypeDef drv8908_all_off(SPI_HandleTypeDef *hspi);

#endif /* DRV8908_H */

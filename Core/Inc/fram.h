
#ifndef FRAM_H
#define FRAM_H

#include "main.h"

#define FRAM_SIZE           0x40000U   /* 256 KB */  //Byte
#define FRAM_ADDR_MAX       0x3FFFFU   /* address 18 bit */

/* Opcode - datasheet Table 2 */
#define FRAM_OP_WREN        0x06U
#define FRAM_OP_WRDI        0x04U
#define FRAM_OP_RDSR        0x05U
#define FRAM_OP_WRSR        0x01U
#define FRAM_OP_WRITE       0x02U
#define FRAM_OP_READ        0x03U
#define FRAM_OP_FSTRD       0x0BU
#define FRAM_OP_SSWR        0x42U
#define FRAM_OP_SSRD        0x4BU
#define FRAM_OP_RDID        0x9FU
#define FRAM_OP_RUID        0x4CU
#define FRAM_OP_WRSN        0xC2U
#define FRAM_OP_RDSN        0xC3U
#define FRAM_OP_DPD         0xBAU
#define FRAM_OP_HBN         0xB9U

/* Status register */
#define FRAM_SR_WEL         (1U << 1)
#define FRAM_SR_BP0         (1U << 2)
#define FRAM_SR_BP1         (1U << 3)
#define FRAM_SR_WPEN        (1U << 7)

#define FRAM_BP_NONE        0x00U
#define FRAM_BP_UPPER_1_4   FRAM_SR_BP0
#define FRAM_BP_UPPER_1_2   FRAM_SR_BP1
#define FRAM_BP_ALL         (FRAM_SR_BP0 | FRAM_SR_BP1)

#define FRAM_ID_LEN         9U
#define FRAM_ID_CONT        0x7FU
#define FRAM_MANUF_ID       0xC2U

#define FRAM_CHUNK          64U

#define FRAM_SPI_TIMEOUT_MS 100U

#define FRAM_CS_PORT        GPIOB
#define FRAM_CS_PIN         GPIO_PIN_12

HAL_StatusTypeDef fram_read_id(SPI_HandleTypeDef *hspi, uint8_t *id);   /* id[FRAM_ID_LEN] */
HAL_StatusTypeDef fram_check_id(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef fram_read_status(SPI_HandleTypeDef *hspi, uint8_t *sr);
HAL_StatusTypeDef fram_write_status(SPI_HandleTypeDef *hspi, uint8_t sr);

HAL_StatusTypeDef fram_read(SPI_HandleTypeDef *hspi, uint32_t addr, uint8_t *data, uint32_t len);
HAL_StatusTypeDef fram_write(SPI_HandleTypeDef *hspi, uint32_t addr, const uint8_t *data, uint32_t len);

#endif /* FRAM_H */

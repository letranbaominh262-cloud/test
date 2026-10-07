
#ifndef FLASH_H
#define FLASH_H

#include "main.h"

#define FLASH_CHIP_SIZE          0x2000000U   // 32 MB
#define FLASH_PG_SIZE     256U
#define FLASH_SECT_SIZE   0x1000U      // 4 KB
#define FLASH_BLK_SIZE    0x10000U     // 64 KB

// Opcode
#define FLASH_OP_WREN       0x06U
#define FLASH_OP_WRDI       0x04U
#define FLASH_OP_RDSR1      0x05U
#define FLASH_OP_RDSR2      0x07U
#define FLASH_OP_RDID       0x9FU
#define FLASH_OP_4READ      0x13U        // read, 4 byte address
#define FLASH_OP_4PP        0x12U        // write 1 page, 4 byte address
#define FLASH_OP_4SE        0x21U        // erase 1 sector 4 KB
#define FLASH_OP_4BE        0xDCU        // erase 1 block 64 KB
#define FLASH_OP_CE         0xC7U        // erase all chip
#define FLASH_OP_CLSR       0x30U        // erase error flag P_ERR/E_ERR

// Status Register 1
#define FLASH_SR1_WIP       (1U << 0)    // busying read/write
#define FLASH_SR1_WEL       (1U << 1)
#define FLASH_SR1_E_ERR     (1U << 5)    // erase error
#define FLASH_SR1_P_ERR     (1U << 6)    // write error

#define FLASH_ID_LEN        3U
#define FLASH_MANUF_ID      0x01U
#define FLASH_DEV_ID_MSB    0x60U
#define FLASH_DEV_ID_LSB    0x19U

#define FLASH_TIMEOUT_PP_MS     10U      // tPP max 1.2 ms
#define FLASH_TIMEOUT_SE_MS     400U     // tSE max 250 ms
#define FLASH_TIMEOUT_BE_MS     1000U    // tBE max 725 ms
#define FLASH_TIMEOUT_CE_MS     400000U  // tCE max 360 s

#define FLASH_CHUNK             64U
#define FLASH_SPI_TIMEOUT_MS    100U

#define FLASH_CS_PORT       GPIOA
#define FLASH_CS_PIN        GPIO_PIN_4

HAL_StatusTypeDef flash_read_id(SPI_HandleTypeDef *hspi, uint8_t *id);
HAL_StatusTypeDef flash_check_id(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef flash_read_status(SPI_HandleTypeDef *hspi, uint8_t *sr1);

HAL_StatusTypeDef flash_read(SPI_HandleTypeDef *hspi, uint32_t addr, uint8_t *data, uint32_t len);
HAL_StatusTypeDef flash_write(SPI_HandleTypeDef *hspi, uint32_t addr, const uint8_t *data, uint32_t len);

HAL_StatusTypeDef flash_erase_sector(SPI_HandleTypeDef *hspi, uint32_t addr);  /* 4 KB */
HAL_StatusTypeDef flash_erase_block(SPI_HandleTypeDef *hspi, uint32_t addr);   /* 64 KB */
HAL_StatusTypeDef flash_erase_chip(SPI_HandleTypeDef *hspi);

#endif /* FLASH_H */

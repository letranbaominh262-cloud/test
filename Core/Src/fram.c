#include "fram.h"


static HAL_StatusTypeDef fram_xfer(SPI_HandleTypeDef *hspi, uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef st;

    HAL_GPIO_WritePin(FRAM_CS_PORT, FRAM_CS_PIN, GPIO_PIN_RESET);

    if (rx != NULL)
    {
        st = HAL_SPI_TransmitReceive(hspi, tx, rx, len, FRAM_SPI_TIMEOUT_MS);
    }
    else
    {
        st = HAL_SPI_Transmit(hspi, tx, len, FRAM_SPI_TIMEOUT_MS);
    }

    HAL_GPIO_WritePin(FRAM_CS_PORT, FRAM_CS_PIN, GPIO_PIN_SET);

    return st;
}

static HAL_StatusTypeDef fram_write_enable(SPI_HandleTypeDef *hspi)
{
    uint8_t op = FRAM_OP_WREN;
    return fram_xfer(hspi, &op, NULL, 1U);
}

HAL_StatusTypeDef fram_read_id(SPI_HandleTypeDef *hspi, uint8_t *id)
{
    uint8_t tx[1U + FRAM_ID_LEN] = { 0 };
    uint8_t rx[1U + FRAM_ID_LEN] = { 0 };

    if (hspi == NULL || id == NULL) return HAL_ERROR;

    tx[0] = FRAM_OP_RDID;
    if (fram_xfer(hspi, tx, rx, 1U + FRAM_ID_LEN) != HAL_OK) return HAL_ERROR;

    for (uint32_t i = 0U; i < FRAM_ID_LEN; i++)
    {
        id[i] = rx[1U + i];
    }
    return HAL_OK;
}

HAL_StatusTypeDef fram_check_id(SPI_HandleTypeDef *hspi)
{
    uint8_t id[FRAM_ID_LEN];
    uint32_t i;

    if (fram_read_id(hspi, id) != HAL_OK) return HAL_ERROR;

    for (i = 0U; i < 6U; i++)
    {
        if (id[i] != FRAM_ID_CONT) return HAL_ERROR;
    }
    return (id[6] == FRAM_MANUF_ID) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef fram_read_status(SPI_HandleTypeDef *hspi, uint8_t *sr)
{
    uint8_t tx[2] = { FRAM_OP_RDSR, 0x00U };
    uint8_t rx[2] = { 0, 0 };

    if (hspi == NULL || sr == NULL) return HAL_ERROR;
    if (fram_xfer(hspi, tx, rx, 2U) != HAL_OK) return HAL_ERROR;

    *sr = rx[1];
    return HAL_OK;
}

HAL_StatusTypeDef fram_write_status(SPI_HandleTypeDef *hspi, uint8_t sr)
{
    uint8_t tx[2];

    if (hspi == NULL) return HAL_ERROR;
    if (fram_write_enable(hspi) != HAL_OK) return HAL_ERROR;

    tx[0] = FRAM_OP_WRSR;
    tx[1] = sr;
    return fram_xfer(hspi, tx, NULL, 2U);   // WEL tu xoa khi CS len
}

HAL_StatusTypeDef fram_read(SPI_HandleTypeDef *hspi, uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t  tx[4U + FRAM_CHUNK];
    uint8_t  rx[4U + FRAM_CHUNK];
    uint32_t done = 0U;

    if (hspi == NULL || data == NULL || len == 0U) return HAL_ERROR;
    if (addr > FRAM_ADDR_MAX || len > FRAM_SIZE - addr) return HAL_ERROR;

    while (done < len)
    {
        uint32_t n = len - done;
        uint32_t a = addr + done;
        uint32_t i;

        if (n > FRAM_CHUNK) n = FRAM_CHUNK;

        tx[0] = FRAM_OP_READ;
        tx[1] = (uint8_t)(a >> 16);
        tx[2] = (uint8_t)(a >> 8);
        tx[3] = (uint8_t)a;
        /* Byte tu tx[4] tro di la byte rac de tao clock, FRAM bo qua MOSI khi doc */
        if (fram_xfer(hspi, tx, rx, (uint16_t)(4U + n)) != HAL_OK) return HAL_ERROR;

        for (i = 0U; i < n; i++)
        {
            data[done + i] = rx[4U + i];
        }
        done += n;
    }
    return HAL_OK;
}

HAL_StatusTypeDef fram_write(SPI_HandleTypeDef *hspi, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint8_t  tx[4U + FRAM_CHUNK];
    uint32_t done = 0U;

    if (hspi == NULL || data == NULL || len == 0U) return HAL_ERROR;
    if (addr > FRAM_ADDR_MAX || len > FRAM_SIZE - addr) return HAL_ERROR;

    while (done < len)
    {
        uint32_t n = len - done;
        uint32_t a = addr + done;
        uint32_t i;

        if (n > FRAM_CHUNK) n = FRAM_CHUNK;

        // WEL bi xoa sau moi lenh WRITE -> phai WREN lai cho tung chunk
        if (fram_write_enable(hspi) != HAL_OK) return HAL_ERROR;

        tx[0] = FRAM_OP_WRITE;
        tx[1] = (uint8_t)(a >> 16);
        tx[2] = (uint8_t)(a >> 8);
        tx[3] = (uint8_t)a;
        for (i = 0U; i < n; i++)
        {
            tx[4U + i] = data[done + i];
        }
        if (fram_xfer(hspi, tx, NULL, (uint16_t)(4U + n)) != HAL_OK) return HAL_ERROR;

        done += n;
    }
    return HAL_OK;
}

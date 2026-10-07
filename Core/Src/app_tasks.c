#include "app_tasks.h"
#include "fram.h"
#include "flash.h"
#include "ntc.h"
#include "drv8908.h"
#include "csense.h"
#include "tca9548a.h"
#include "mlx90614.h"
#include "veml6031.h"
#include "mlx90393.h"
#include <stdio.h>
#include <string.h>

/********** Define **********/
//FRAM
#define FRAM_TEST_ADDR      0x0100U
#define FRAM_TEST_LEN       10U
//Flash
#define FLASH_TEST_ADDR     0x000000U
#define FLASH_TEST_LEN      16U
//CAN1
#define CAN_TX_ID           0x123U
//RS485
#define RS485_TEST_LEN      16U

#define TEMPERATURE			0
/********** Variable **********/
int32_t bringup_data[1];
// Fram
uint8_t fram_id[FRAM_ID_LEN];              	// ID: 7F 7F 7F 7F 7F 7F C2 25 C8
uint8_t fram_frame[FRAM_TEST_LEN];
uint8_t fram_data[FRAM_TEST_LEN];
uint8_t fram_sr;
// Flash
uint8_t flash_id[FLASH_ID_LEN];        		// ID: 01 60 19
uint8_t flash_sr;
uint8_t flash_frame[FLASH_TEST_LEN];
uint8_t flash_data[FLASH_TEST_LEN];
volatile uint8_t flash_erased_ok;
// CAN1
volatile uint32_t can_tx_count, can_rx_count;
volatile uint32_t can_rx_id, can_rx_dlc;
uint8_t  can_tx_data[8];
uint8_t  can_rx_data[8];
volatile uint32_t can_last_err;
volatile uint32_t can_bus_off;
// CAN2 Thruster
volatile uint32_t can2_tx_count, can2_rx_count;
volatile uint32_t can2_rx_id, can2_rx_dlc;
uint8_t  can2_rx_data[8];
volatile uint32_t can2_last_err;
volatile uint32_t can2_bus_off;
volatile HAL_StatusTypeDef can2_st_start;
// RS485
uint8_t  rs485_tx[RS485_TEST_LEN];
uint8_t  rs485_rx[RS485_TEST_LEN];
volatile uint32_t rs485_ok_count;
volatile uint32_t rs485_err_count;
volatile uint8_t  rs485_match;
// NTC
volatile uint32_t ntc_raw;
volatile float    ntc_res;
volatile float    ntc_temp_c;
// Test all FRAM (256 KB) and Flash (32 MB)
#define BIG_BUF_LEN     4096U
static uint8_t big_buf[BIG_BUF_LEN];
volatile uint8_t  big_run = 1;
volatile uint8_t  big_step;             // 1 FRAM ghi, 2 FRAM doc, 3 Flash xoa, 4 Flash ghi, 5 Flash doc, 6 xong
volatile uint32_t big_addr;
volatile uint32_t big_seed;
volatile uint32_t big_count;
volatile HAL_StatusTypeDef fram_all_st, flash_all_st;
volatile uint32_t fram_all_err, flash_all_err;
volatile uint32_t fram_all_first, flash_all_first;
volatile uint32_t fram_all_wr_ms, fram_all_rd_ms;
volatile uint32_t flash_all_er_ms, flash_all_wr_ms, flash_all_rd_ms;
volatile int32_t  ntc_temp_mc;
volatile HAL_StatusTypeDef ntc_st_init, ntc_st_read;
// Half_bridge
volatile HAL_StatusTypeDef drv_st_init, drv_st_cmd;
volatile uint8_t  drv_ic_stat;           // IC_STAT: bit0 NPOR, bit1 OVP, bit2 UVLO, bit3 OCP, bit4 OLD, bit5 OTW, bit6 OTSD
volatile uint8_t  drv_fault_pin;
volatile uint8_t  drv_op1, drv_op2;
// Current Sensor
volatile uint32_t i1_zero, i2_zero, i3_zero;
volatile int32_t  i1_ma, i2_ma, i3_ma;
volatile uint32_t i1_raw;
// Sensor hub
volatile float    ces_tobj_c;      // opto temperature
volatile float    ces_ta_c;        // cover temperature
volatile uint16_t ces_raw;         // raw data

volatile float    fes_tobj_c;      // opto temperature
volatile float    fes_ta_c;        // cover temperature
volatile uint16_t fes_raw;         // raw data
// Sun Sensor
volatile uint16_t sun_raw;
volatile float    sun_lux;
// MTQ
volatile float    mag_x, mag_y, mag_z, mag_norm;
volatile HAL_StatusTypeDef mag_st_chk, mag_st_read;

// Flags for Debug reporting
volatile uint8_t flag_ntc = 0;
volatile uint8_t flag_fram = 0;
volatile uint8_t flag_flash = 0;
volatile uint8_t flag_drv = 0;
volatile uint8_t flag_csense = 0;

volatile uint8_t flag_can1 = 0;
volatile uint8_t flag_can2 = 0;
volatile uint8_t flag_rs485 = 0;

volatile uint8_t flag_sun = 0;
volatile uint8_t flag_earth = 0;
volatile uint8_t flag_mag = 0;

/* Synchronization for bring-up commands (only semaphore, no mutex) */
static volatile uint8_t bringup_req = 0;
osSemaphoreId_t bringup_sem = NULL;

extern UART_HandleTypeDef huart8; 	// Debug
extern SPI_HandleTypeDef hspi1; 	// Flash
extern SPI_HandleTypeDef hspi2; 	// Fram
extern SPI_HandleTypeDef hspi3;     // DRV8908
extern FDCAN_HandleTypeDef hfdcan1;	// CAN0 -> PC104
extern FDCAN_HandleTypeDef hfdcan2;	// CAN1 -> Thruster
extern UART_HandleTypeDef huart2;  	// RS485
extern ADC_HandleTypeDef hadc1;		// NTC & Csensor
extern ADC_HandleTypeDef hadc3;		// Csensor
extern I2C_HandleTypeDef hi2c2;     // Sensor Hub
extern I2C_HandleTypeDef hi2c3;     // MTQ


static void debug_send(const char *str)
{
	HAL_UART_Transmit(&huart8, (uint8_t *)str, (uint16_t)strlen(str), 200);
}

static void WdgTask(void *argument)
{
	for (;;)
	{
		HAL_GPIO_TogglePin(WDG_PORT, WDG_PIN);   /* WDI MAX6369 */
		osDelay(100);
	}
}

/* ========== DebugTask (COMMENTED OUT - not used in bringup mode) ========== */
//static void DebugTask(void *argument)
//{
//	char log_buf[128];
//	uint8_t reported = 0;
//
//	debug_send("\r\n==================================================\r\n");
//	debug_send("     ADCS BRING-UP FIRMWARE RUNNING (FREERTOS)     \r\n");
//	debug_send("==================================================\r\n");
//
//	// Doi cho den khi tat ca cac module khoi tao xong (hoac bi treo o dau do)
//	// Cho 5 giay de cac task khac chay khoi tao
//	osDelay(5000);
//
//	for (;;)
//	{
//		if (!reported)
//		{
//			debug_send("\r\n--- [BAO CAO TRANG THAI KHOI TAO TUNG MODULE] ---\r\n");
//
//			// Bringup
//			if (flag_ntc) debug_send("[OK] NTC Sensor\r\n"); else debug_send("[FAIL] NTC Sensor\r\n");
//			if (flag_fram) debug_send("[OK] FRAM\r\n"); else debug_send("[FAIL] FRAM\r\n");
//			if (flag_flash) debug_send("[OK] Flash\r\n"); else debug_send("[FAIL] Flash\r\n");
//			if (flag_drv) debug_send("[OK] DRV8908\r\n"); else debug_send("[FAIL] DRV8908\r\n");
//			if (flag_csense) debug_send("[OK] Current Sensor\r\n"); else debug_send("[FAIL] Current Sensor\r\n");
//
//			// Com
//			if (flag_can1) debug_send("[OK] CAN1 (PC104)\r\n"); else debug_send("[FAIL] CAN1 (PC104)\r\n");
//			if (flag_can2) debug_send("[OK] CAN2 (Thruster)\r\n"); else debug_send("[FAIL] CAN2 (Thruster)\r\n");
//			if (flag_rs485) debug_send("[OK] RS485\r\n"); else debug_send("[FAIL] RS485\r\n");
//
//			// Sensor
//			if (flag_sun) debug_send("[OK] Sun Sensor\r\n"); else debug_send("[FAIL] Sun Sensor\r\n");
//			if (flag_earth) debug_send("[OK] Earth Sensors\r\n"); else debug_send("[FAIL] Earth Sensors\r\n");
//			if (flag_mag) debug_send("[OK] Magnetometer (MTQ)\r\n"); else debug_send("[FAIL] Magnetometer (MTQ)\r\n");
//
//			debug_send("==================================================\r\n\r\n");
//			reported = 1;
//		}
//
//		// Dinh ky moi giay in dong trang thai tong hop (Heartbeat)
//		snprintf(log_buf, sizeof(log_buf),
//			"[ALIVE] NTC: %ld mC | Coil1: %ld mA | MagNorm: %d uT | CAN1_TX: %lu | RS485_Match: %s\r\n",
//			(long)ntc_temp_mc,
//			(long)i1_ma,
//			(int)mag_norm,
//			(unsigned long)can_tx_count,
//			flag_rs485 ? "OK" : "FAIL");
//		debug_send(log_buf);
//
//		osDelay(1000);
//	}
//}

/* ========== big_test helpers (COMMENTED OUT - not used in bringup mode) ========== */
//static uint8_t big_pattern(uint32_t a)
//{
//	return (uint8_t)((a ^ (a >> 8) ^ (a >> 16) ^ (a >> 24)) + big_seed);
//}
//
//static void big_fill(uint32_t addr)
//{
//	for (uint32_t i = 0; i < BIG_BUF_LEN; i++) big_buf[i] = big_pattern(addr + i);
//}
//
//static uint32_t big_check(uint32_t addr, volatile uint32_t *first)
//{
//	uint32_t err = 0;
//	for (uint32_t i = 0; i < BIG_BUF_LEN; i++)
//	{
//		if (big_buf[i] != big_pattern(addr + i))
//		{
//			if (*first == 0xFFFFFFFFU) *first = addr + i;
//			err++;
//		}
//	}
//	return err;
//}
//
//static void big_test_all(void)
//{
//	uint32_t t;
//
//	big_seed = HAL_GetTick();
//	fram_all_st  = HAL_OK; fram_all_err  = 0; fram_all_first  = 0xFFFFFFFFU;
//	flash_all_st = HAL_OK; flash_all_err = 0; flash_all_first = 0xFFFFFFFFU;
//
//	// 1. FRAM ghi toan bo 256 KB
//	big_step = 1; t = HAL_GetTick();
//	for (big_addr = 0; big_addr < FRAM_SIZE && fram_all_st == HAL_OK; big_addr += BIG_BUF_LEN)
//	{
//		big_fill(big_addr);
//		fram_all_st = fram_write(&hspi2, big_addr, big_buf, BIG_BUF_LEN);
//	}
//	fram_all_wr_ms = HAL_GetTick() - t;
//
//	// 2. FRAM doc lai toan bo va so sanh
//	big_step = 2; t = HAL_GetTick();
//	for (big_addr = 0; big_addr < FRAM_SIZE && fram_all_st == HAL_OK; big_addr += BIG_BUF_LEN)
//	{
//		fram_all_st = fram_read(&hspi2, big_addr, big_buf, BIG_BUF_LEN);
//		fram_all_err += big_check(big_addr, &fram_all_first);
//	}
//	fram_all_rd_ms = HAL_GetTick() - t;
//
//	// 3. Flash xoa toan bo, tung block 64 KB
//	big_step = 3; t = HAL_GetTick();
//	for (big_addr = 0; big_addr < FLASH_CHIP_SIZE && flash_all_st == HAL_OK; big_addr += FLASH_BLK_SIZE)
//	{
//		flash_all_st = flash_erase_block(&hspi1, big_addr);
//	}
//	flash_all_er_ms = HAL_GetTick() - t;
//
//	// 4. Flash ghi toan bo 32 MB
//	big_step = 4; t = HAL_GetTick();
//	for (big_addr = 0; big_addr < FLASH_CHIP_SIZE && flash_all_st == HAL_OK; big_addr += BIG_BUF_LEN)
//	{
//		big_fill(big_addr);
//		flash_all_st = flash_write(&hspi1, big_addr, big_buf, BIG_BUF_LEN);
//	}
//	flash_all_wr_ms = HAL_GetTick() - t;
//
//	// 5. Flash doc lai toan bo va so sanh
//	big_step = 5; t = HAL_GetTick();
//	for (big_addr = 0; big_addr < FLASH_CHIP_SIZE && flash_all_st == HAL_OK; big_addr += BIG_BUF_LEN)
//	{
//		flash_all_st = flash_read(&hspi1, big_addr, big_buf, BIG_BUF_LEN);
//		flash_all_err += big_check(big_addr, &flash_all_first);
//	}
//	flash_all_rd_ms = HAL_GetTick() - t;
//
//	big_step = 6;
//	big_count++;
//}

/* ==================================================================
 * BringupTask: command-driven
 * Waits on bringup_sem, reads bringup_req, executes corresponding test.
 * Command map:
 *   0x01 = Flash
 *   0x02 = FRAM
 *   0x03 = CAN1
 *   0x04 = CAN2
 *   0x05 = RS485
 *   0x06 = Half-bridge (DRV8908)
 * ================================================================== */
static void BringupTask(void *argument)
{
	(void)argument;

	/* === OLD BringupTask body (COMMENTED OUT) ===
	HAL_GPIO_WritePin(ENLS_PORT, ENLS_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(EN12_PORT, EN12_PIN, GPIO_PIN_SET);
	// NTC
	ntc_st_init = ntc_init(&hadc1);
	if (ntc_st_init == HAL_OK) flag_ntc = 1;

	// Fram
	fram_read_id(&hspi2, fram_id);
	fram_read_status(&hspi2, &fram_sr);
	for (uint32_t i = 0; i < FRAM_TEST_LEN; i++)
	{
		fram_frame[i] = (uint8_t)(0xA0 + i);
	}
	fram_write(&hspi2, FRAM_TEST_ADDR, fram_frame, FRAM_TEST_LEN);
	fram_read(&hspi2, FRAM_TEST_ADDR, fram_data, FRAM_TEST_LEN);
	if (fram_check_id(&hspi2) == HAL_OK && memcmp(fram_data, fram_frame, FRAM_TEST_LEN) == 0)
	{
		flag_fram = 1;
	}

	// Flash
	flash_read_id(&hspi1, flash_id);
	flash_read_status(&hspi1, &flash_sr);
	flash_erase_sector(&hspi1, FLASH_TEST_ADDR);
	flash_read(&hspi1, FLASH_TEST_ADDR, flash_data, FLASH_TEST_LEN);
	flash_erased_ok = 1;
	for (uint32_t i = 0; i < FLASH_TEST_LEN; i++)
	{
		if (flash_data[i] != 0xFF) flash_erased_ok = 0;
	}
	for (uint32_t i = 0; i < FLASH_TEST_LEN; i++)
	{
		flash_frame[i] = (uint8_t)(0x50 + i);      // 50 51 52 ... 5F
	}
	flash_write(&hspi1, FLASH_TEST_ADDR, flash_frame, FLASH_TEST_LEN);
	flash_read(&hspi1, FLASH_TEST_ADDR, flash_data, FLASH_TEST_LEN);
	if (flash_check_id(&hspi1) == HAL_OK && flash_erased_ok && memcmp(flash_data, flash_frame, FLASH_TEST_LEN) == 0)
	{
		flag_flash = 1;
	}

	// Half_Bridge
	drv_st_init = drv8908_init(&hspi3);
	for (uint8_t c = 1; c <= 3; c++)
	{
		drv8908_coil_freq(&hspi3, c, DRV_PWM_FREQ_2000HZ);
	}
	if (drv_st_init == HAL_OK) flag_drv = 1;

	// Current Sensor
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
	drv8908_all_off(&hspi3);
	osDelay(10);
	csense_calibrate(&hadc1, CSENSE_CH_COIL1, (uint32_t *)&i1_zero);
	csense_calibrate(&hadc1, CSENSE_CH_COIL2, (uint32_t *)&i2_zero);
	csense_calibrate(&hadc3, CSENSE_CH_COIL3, (uint32_t *)&i3_zero);
	if (i1_zero > 0 && i2_zero > 0 && i3_zero > 0) flag_csense = 1;

	for (;;)
	{
		ntc_read_raw(&hadc1, (uint32_t *)&ntc_raw);
		ntc_read_res(&hadc1, (float *)&ntc_res);
		ntc_st_read = ntc_read_mc(&hadc1, (int32_t *)&ntc_temp_mc);
		ntc_temp_c  = ntc_temp_mc / 1000.0f;
		bringup_data[TEMPERATURE] = ntc_temp_mc;
		fram_read(&hspi2, FRAM_TEST_ADDR, fram_data, FRAM_TEST_LEN);
		flash_read(&hspi1, FLASH_TEST_ADDR, flash_data, FLASH_TEST_LEN);
		drv8908_coil_duty(&hspi3, 1,100);
		drv_st_cmd = drv8908_coil_set(&hspi3, 1, DRV_COIL_FWD);
		drv8908_clear_faults(&hspi3);
		osDelay(100);
		drv8908_get_status(&hspi3, (uint8_t *)&drv_ic_stat);
		drv8908_read_reg(&hspi3, DRV_REG_OP_CTRL_1, (uint8_t *)&drv_op1);
		drv8908_read_reg(&hspi3, DRV_REG_OP_CTRL_2, (uint8_t *)&drv_op2);
		drv_fault_pin = drv8908_fault_pin();
		csense_read_ma(&hadc1, CSENSE_CH_COIL1, i1_zero, (int32_t *)&i1_ma);
		csense_read_ma(&hadc1, CSENSE_CH_COIL2, i2_zero, (int32_t *)&i2_ma);
		csense_read_ma(&hadc3, CSENSE_CH_COIL3, i3_zero, (int32_t *)&i3_ma);
		csense_read_raw(&hadc1, CSENSE_CH_COIL1, (uint32_t *)&i1_raw);
		osDelay(1000);
	}
	=== END OLD BringupTask body === */

	for (;;)
	{
		/* Block until CmdTask signals us */
		osSemaphoreAcquire(bringup_sem, osWaitForever);

		/* Read command (semaphore guarantees single-producer/single-consumer) */
		uint8_t cmd = bringup_req;
		bringup_req = 0;

		switch (cmd)
		{
			/* ---- 0x01: Flash test ---- */
			case 0x01:
			{
				flash_read_id(&hspi1, flash_id);
				flash_read_status(&hspi1, &flash_sr);
				flash_erase_sector(&hspi1, FLASH_TEST_ADDR);
				flash_read(&hspi1, FLASH_TEST_ADDR, flash_data, FLASH_TEST_LEN);
				flash_erased_ok = 1;
				for (uint32_t i = 0; i < FLASH_TEST_LEN; i++)
				{
					if (flash_data[i] != 0xFF) flash_erased_ok = 0;
				}
				for (uint32_t i = 0; i < FLASH_TEST_LEN; i++)
				{
					flash_frame[i] = (uint8_t)(0x50 + i);
				}
				flash_write(&hspi1, FLASH_TEST_ADDR, flash_frame, FLASH_TEST_LEN);
				flash_read(&hspi1, FLASH_TEST_ADDR, flash_data, FLASH_TEST_LEN);
				if (flash_check_id(&hspi1) == HAL_OK && flash_erased_ok
					&& memcmp(flash_data, flash_frame, FLASH_TEST_LEN) == 0)
				{
					flag_flash = 1;
					debug_send("Flash bringup success\r\n");
				}
				else
				{
					debug_send("Flash bringup FAIL\r\n");
				}
				break;
			}

			/* ---- 0x02: FRAM test ---- */
			case 0x02:
			{
				fram_read_id(&hspi2, fram_id);
				fram_read_status(&hspi2, &fram_sr);
				for (uint32_t i = 0; i < FRAM_TEST_LEN; i++)
				{
					fram_frame[i] = (uint8_t)(0xA0 + i);
				}
				fram_write(&hspi2, FRAM_TEST_ADDR, fram_frame, FRAM_TEST_LEN);
				fram_read(&hspi2, FRAM_TEST_ADDR, fram_data, FRAM_TEST_LEN);
				if (fram_check_id(&hspi2) == HAL_OK
					&& memcmp(fram_data, fram_frame, FRAM_TEST_LEN) == 0)
				{
					flag_fram = 1;
					debug_send("FRAM bringup success\r\n");
				}
				else
				{
					debug_send("FRAM bringup FAIL\r\n");
				}
				break;
			}

			/* ---- 0x03: CAN1 test (TX only, no RX hardware) ---- */
			case 0x03:
			{
				/* Enable peripheral power & wake transceiver */
				HAL_GPIO_WritePin(PB1_PERI_PORT, PB1_PERI_PIN, GPIO_PIN_SET);
				osDelay(10);
				HAL_GPIO_WritePin(CAN1_STB_PORT, CAN1_STB_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(CAN1_SHDN_PORT, CAN1_SHDN_PIN, GPIO_PIN_RESET);

				/* Filter config (still needed before Start) */
				FDCAN_FilterTypeDef sFilter = {0};
				sFilter.IdType       = FDCAN_STANDARD_ID;
				sFilter.FilterIndex  = 0;
				sFilter.FilterType   = FDCAN_FILTER_MASK;
				sFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
				sFilter.FilterID1    = 0x000;
				sFilter.FilterID2    = 0x000;
				HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilter);
				HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
					FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
					FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

				if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
				{
					debug_send("CAN1 bringup FAIL (Start)\r\n");
					break;
				}

				/* Transmit a test frame */
				FDCAN_TxHeaderTypeDef TxHeader = {0};
				TxHeader.Identifier          = CAN_TX_ID;
				TxHeader.IdType              = FDCAN_STANDARD_ID;
				TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
				TxHeader.DataLength          = FDCAN_DLC_BYTES_8;
				TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
				TxHeader.BitRateSwitch       = FDCAN_BRS_OFF;
				TxHeader.FDFormat            = FDCAN_CLASSIC_CAN;
				TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
				TxHeader.MessageMarker       = 0;

				uint8_t tx_test[8] = {0xCA, 0x01, 0x55, 0xAA, 0x01, 0x02, 0x03, 0x04};
				if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, tx_test) == HAL_OK)
				{
					flag_can1 = 1;
					debug_send("CAN1 bringup success\r\n");
				}
				else
				{
					debug_send("CAN1 bringup FAIL (TX)\r\n");
				}
				break;
			}

			/* ---- 0x04: CAN2 test (TX only, no RX hardware) ---- */
			case 0x04:
			{
				/* Wake transceiver (PB1 already set if CAN1 ran first, but safe to repeat) */
				HAL_GPIO_WritePin(PB1_PERI_PORT, PB1_PERI_PIN, GPIO_PIN_SET);
				osDelay(10);
				HAL_GPIO_WritePin(CAN2_STB_PORT, CAN2_STB_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(CAN2_SHDN_PORT, CAN2_SHDN_PIN, GPIO_PIN_RESET);

				FDCAN_FilterTypeDef sFilter = {0};
				sFilter.IdType       = FDCAN_STANDARD_ID;
				sFilter.FilterIndex  = 0;
				sFilter.FilterType   = FDCAN_FILTER_MASK;
				sFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
				sFilter.FilterID1    = 0x000;
				sFilter.FilterID2    = 0x000;
				HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilter);
				HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,
					FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
					FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

				if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK)
				{
					debug_send("CAN2 bringup FAIL (Start)\r\n");
					break;
				}

				/* Transmit a test frame */
				FDCAN_TxHeaderTypeDef TxHeader = {0};
				TxHeader.Identifier          = CAN_TX_ID;
				TxHeader.IdType              = FDCAN_STANDARD_ID;
				TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
				TxHeader.DataLength          = FDCAN_DLC_BYTES_8;
				TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
				TxHeader.BitRateSwitch       = FDCAN_BRS_OFF;
				TxHeader.FDFormat            = FDCAN_CLASSIC_CAN;
				TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
				TxHeader.MessageMarker       = 0;

				uint8_t tx_test[8] = {0xCA, 0x02, 0x55, 0xAA, 0x05, 0x06, 0x07, 0x08};
				if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, tx_test) == HAL_OK)
				{
					flag_can2 = 1;
					debug_send("CAN2 bringup success\r\n");
				}
				else
				{
					debug_send("CAN2 bringup FAIL (TX)\r\n");
				}
				break;
			}

			/* ---- 0x05: RS485 test ---- */
			/* ---- 0x05: RS485 test (TX Only) ---- */
            case 0x05:
            {
                // Chuẩn bị dữ liệu gửi (0x30 đến 0x3F)
                for (uint32_t i = 0; i < RS485_TEST_LEN; i++)
                {
                    rs485_tx[i] = (uint8_t)(0x30 + i);
                }

                // Bật chế độ Gửi trên IC thu phát RS485 (tuỳ thiết kế phần cứng: 
                // nếu mạch của bạn dùng mức HIGH để truyền thì để GPIO_PIN_SET, ngược lại dùng RESET)
                HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_SET);

                // Gửi nguyên mảng dữ liệu qua UART2 trong 1 lần
                if (HAL_UART_Transmit(&huart2, rs485_tx, RS485_TEST_LEN, 100) == HAL_OK)
                {
                    flag_rs485 = 1;
                    debug_send("RS485 TX bringup success\r\n");
                }
                else
                {
                    debug_send("RS485 TX bringup FAIL\r\n");
                }

                // Đưa chân RE/DE về lại mức nhận sau khi gửi xong
                HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_RESET);
                break;
            }
			/* ---- 0x06: Half-bridge (DRV8908) test ---- */
			case 0x06:
			{
				HAL_GPIO_WritePin(ENLS_PORT, ENLS_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(EN12_PORT, EN12_PIN, GPIO_PIN_SET);
				drv_st_init = drv8908_init(&hspi3);
				for (uint8_t c = 1; c <= 3; c++)
				{
					drv8908_coil_freq(&hspi3, c, DRV_PWM_FREQ_2000HZ);
				}
				if (drv_st_init == HAL_OK)
				{
					flag_drv = 1;
					debug_send("Half-bridge bringup success\r\n");
				}
				else
				{
					debug_send("Half-bridge bringup FAIL\r\n");
				}
				break;
			}

			default:
			{
				debug_send("Unknown bringup command\r\n");
				break;
			}
		}
	}
}

/* ========== ComTask (COMMENTED OUT - not used in bringup mode) ========== */
//uint32_t rs485_cnt = 0;
//static void ComTask(void *argument)
//{
//	FDCAN_FilterTypeDef   sFilter;
//	FDCAN_TxHeaderTypeDef TxHeader;
//	FDCAN_RxHeaderTypeDef RxHeader;
//	FDCAN_ProtocolStatusTypeDef psr;
//
//	// CAN1
//	HAL_GPIO_WritePin(PB1_PERI_PORT, PB1_PERI_PIN, GPIO_PIN_SET);
//	osDelay(10);
//	HAL_GPIO_WritePin(CAN1_STB_PORT, CAN1_STB_PIN, GPIO_PIN_RESET);
//	HAL_GPIO_WritePin(CAN1_SHDN_PORT, CAN1_SHDN_PIN, GPIO_PIN_RESET);
//
//	sFilter.IdType       = FDCAN_STANDARD_ID;
//	sFilter.FilterIndex  = 0;
//	sFilter.FilterType   = FDCAN_FILTER_MASK;
//	sFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
//	sFilter.FilterID1    = 0x000;
//	sFilter.FilterID2    = 0x000;
//	HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilter);
//	HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
//	if (HAL_FDCAN_Start(&hfdcan1) == HAL_OK) flag_can1 = 1;
//
//	// CAN2
//	HAL_GPIO_WritePin(CAN2_STB_PORT, CAN2_STB_PIN, GPIO_PIN_RESET);
//	HAL_GPIO_WritePin(CAN2_SHDN_PORT, CAN2_SHDN_PIN, GPIO_PIN_RESET);
//	HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilter);
//	HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
//	can2_st_start = HAL_FDCAN_Start(&hfdcan2);
//	if (can2_st_start == HAL_OK) flag_can2 = 1;
//
//	TxHeader.Identifier          = CAN_TX_ID;
//	TxHeader.IdType              = FDCAN_STANDARD_ID;
//	TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
//	TxHeader.DataLength          = FDCAN_DLC_BYTES_8;
//	TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
//	TxHeader.BitRateSwitch       = FDCAN_BRS_OFF;
//	TxHeader.FDFormat            = FDCAN_CLASSIC_CAN;
//	TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
//	TxHeader.MessageMarker       = 0;
//
//	// RS485
//	HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_RESET);
//	for (uint32_t i = 0; i < RS485_TEST_LEN; i++)
//	{
//		rs485_tx[i] = (uint8_t)(0x30 + i);
//	}
//
//	for (;;)
//	{
//		// CAN1
//		can_tx_data[0] = (uint8_t)can_tx_count;
//		can_tx_data[1] = 0xAA;
//		can_tx_data[2] = 0x55;
//		can_tx_data[3] = 0x01;
//		can_tx_data[4] = 0x02;
//		can_tx_data[5] = 0x03;
//		can_tx_data[6] = 0x04;
//		can_tx_data[7] = 0x05;
//
//		HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, can_tx_data);
//		can_tx_count++;
//
//		while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0U)
//		{
//			if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &RxHeader, can_rx_data) == HAL_OK)
//			{
//				can_rx_id  = RxHeader.Identifier;
//				can_rx_dlc = RxHeader.DataLength;
//				can_rx_count++;
//			}
//		}
//
//		HAL_FDCAN_GetProtocolStatus(&hfdcan1, &psr);
//		can_last_err = psr.LastErrorCode;
//		can_bus_off  = psr.BusOff;
//
//		// CAN2 -> Thruster
//		if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2) > 0U &&
//		    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, can_tx_data) == HAL_OK)
//		{
//			can2_tx_count++;
//		}
//
//		while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan2, FDCAN_RX_FIFO0) > 0U)
//		{
//			if (HAL_FDCAN_GetRxMessage(&hfdcan2, FDCAN_RX_FIFO0, &RxHeader, can2_rx_data) == HAL_OK)
//			{
//				can2_rx_id  = RxHeader.Identifier;
//				can2_rx_dlc = RxHeader.DataLength;
//				can2_rx_count++;
//			}
//		}
//
//		HAL_FDCAN_GetProtocolStatus(&hfdcan2, &psr);
//		can2_last_err = psr.LastErrorCode;
//		can2_bus_off  = psr.BusOff;
//
//		//RS485
//		rs485_ok_count  = 0;
//		rs485_err_count = 0;
//		for (uint32_t i = 0; i < RS485_TEST_LEN; i++)
//		{
//			rs485_rx[i] = 0x00;
//			HAL_UART_Transmit(&huart2, &rs485_tx[i], 1, 100);
//			HAL_UART_Receive(&huart2, &rs485_rx[i], 1, 10);
//			if (rs485_rx[i] == rs485_tx[i]) rs485_ok_count++;
//			else rs485_err_count++;
//		}
//		rs485_match = (rs485_ok_count == RS485_TEST_LEN) ? 1 : 0;
//		if (rs485_match)
//		{
//			rs485_cnt++;
//			flag_rs485 = 1;
//		}
//		osDelay(1000);
//	}
//}

/* ========== SensorTask (COMMENTED OUT - not used in bringup mode) ========== */
//volatile uint8_t i2c3_found[8];
//volatile uint8_t i2c3_count;
//
//static void SensorTask(void *argument)
//{
//	// ----- Sensor Hub -----
//	HAL_GPIO_WritePin(PB2_HUB_PORT, PB2_HUB_PIN, GPIO_PIN_SET);
//	osDelay(200);
//	// Sun sensor
//	tca_select(&hi2c2, TCA_MUX_SUN, 0);
//	if (veml_init(&hi2c2, VEML_IT_100MS, VEML_GAIN_X2, VEML_PD_FULL) == HAL_OK) flag_sun = 1;
//	tca_release(&hi2c2, TCA_MUX_SUN);
//	osDelay(200);
//
//	// MTQ
//	HAL_GPIO_WritePin(ENRM_PORT, ENRM_PIN, GPIO_PIN_SET);
//	osDelay(10);
//	HAL_GPIO_WritePin(EN_I2C3_PORT, EN_I2C3_PIN, GPIO_PIN_SET);
//	osDelay(200);
//	mag_st_chk = m393_check(&hi2c3, M393_ADDR_00);
//	if (mag_st_chk == HAL_OK)
//	{
//		m393_init(&hi2c3, M393_ADDR_00, 7, 0);
//		flag_mag = 1;
//	}
//
//	// Earth sensor
//	flag_earth = 1;
//
//	for (;;)
//	{
//		// Earth sensor
//		tca_select(&hi2c2, TCA_MUX_CES, 0);
//		mlx_read_object(&hi2c2, (float *)&ces_tobj_c);
//		mlx_read_ambient(&hi2c2, (float *)&ces_ta_c);
//		mlx_read_raw(&hi2c2, MLX_RAM_TOBJ1, (uint16_t *)&ces_raw);
//		tca_release(&hi2c2, TCA_MUX_CES);
//		osDelay(200);
//		tca_select(&hi2c2, TCA_MUX_FES, 0);
//		mlx_read_object(&hi2c2, (float *)&fes_tobj_c);
//		mlx_read_ambient(&hi2c2, (float *)&fes_ta_c);
//		mlx_read_raw(&hi2c2, MLX_RAM_TOBJ1, (uint16_t *)&fes_raw);
//		tca_release(&hi2c2, TCA_MUX_FES);
//		osDelay(200);
//		// Sun sensor
//		tca_select(&hi2c2, TCA_MUX_SUN, 0);
//		veml_read_als(&hi2c2, (uint16_t *)&sun_raw);
//		veml_read_lux(&hi2c2, (float *)&sun_lux);
//		tca_release(&hi2c2, TCA_MUX_SUN);
//		osDelay(200);
//		// MTQ
//		i2c3_count = 0;
//		for (uint8_t a = 0x08; a <= 0x77; a++)
//		{
//			if (HAL_I2C_IsDeviceReady(&hi2c3, a << 1, 2, 50) == HAL_OK)
//			{
//				if (i2c3_count < 8) i2c3_found[i2c3_count] = a;
//				i2c3_count++;
//			}
//		}
//		mag_st_read = m393_read_ut(&hi2c3, M393_ADDR_00, (float *)&mag_x, (float *)&mag_y, (float *)&mag_z);
//		mag_norm = m393_norm_ut(mag_x, mag_y, mag_z);
//		osDelay(200);
//	}
//}

/* ==================================================================
 * CmdTask: receives a single-byte command over UART8 from laptop
 * and signals BringupTask via semaphore.
 * ================================================================== */
 static void CmdTask(void *argument)

{

(void)argument;

uint8_t cmd;

debug_send("\r\n======================================\r\n");

debug_send(" ADCS BRINGUP - WAITING FOR COMMAND \r\n");

debug_send(" 0x01=Flash 0x02=FRAM 0x03=CAN1 \r\n");

debug_send(" 0x04=CAN2 0x05=RS485 0x06=HBridge \r\n");

debug_send("======================================\r\n");

for (;;)

{


if (HAL_UART_Receive(&huart8, &cmd, 1, 100) == HAL_OK)

{

bringup_req = cmd;

osSemaphoreRelease(bringup_sem);

}

osDelay(10);

}

} 

/* ==================================================================
 * app_tasks_init: create semaphore + spawn 3 tasks
 *   1) WdgTask   - watchdog
 *   2) CmdTask   - receive commands from laptop via UART8
 *   3) BringupTask - execute bringup tests
 * ================================================================== */
void app_tasks_init(void)
{
	/* Create binary semaphore for command synchronization */
	bringup_sem = osSemaphoreNew(1, 0, NULL);

	const osThreadAttr_t wdg_attr     = { .name = "wdg",     .stack_size = 512,  .priority = osPriorityHigh };
	const osThreadAttr_t cmd_attr     = { .name = "cmd",     .stack_size = 512,  .priority = osPriorityNormal };
	const osThreadAttr_t bringup_attr = { .name = "bringup", .stack_size = 2048, .priority = osPriorityLow };

	osThreadNew(WdgTask,     NULL, &wdg_attr);
	osThreadNew(CmdTask,     NULL, &cmd_attr);
	osThreadNew(BringupTask, NULL, &bringup_attr);

	/* Disabled tasks (comment, not delete) */
	// const osThreadAttr_t dbg_attr    = { .name = "dbg",    .stack_size = 512,  .priority = osPriorityLow };
	// const osThreadAttr_t com_attr    = { .name = "com",    .stack_size = 1024, .priority = osPriorityLow };
	// const osThreadAttr_t sensor_attr = { .name = "sensor", .stack_size = 1024, .priority = osPriorityLow };
	// osThreadNew(DebugTask,  NULL, &dbg_attr);
	// osThreadNew(ComTask,    NULL, &com_attr);
	// osThreadNew(SensorTask, NULL, &sensor_attr);
}
//sua code
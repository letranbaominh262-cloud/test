#include "flash.h"
#include <string.h>

static HAL_StatusTypeDef flash_xfer(SPI_HandleTypeDef *hspi, uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef st;

    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET);

    if (rx != NULL)
    {
        st = HAL_SPI_TransmitReceive(hspi, tx, rx, len, FLASH_SPI_TIMEOUT_MS);
    }
    else
    {
        st = HAL_SPI_Transmit(hspi, tx, len, FLASH_SPI_TIMEOUT_MS);
    }

    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET);
    return st;
}

static HAL_StatusTypeDef flash_cmd(SPI_HandleTypeDef *hspi, uint8_t op)
{
    return flash_xfer(hspi, &op, NULL, 1U);
}

static HAL_StatusTypeDef flash_wait_ready(SPI_HandleTypeDef *hspi, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    uint8_t  sr1;
    for (;;)
    {
        if (flash_read_status(hspi, &sr1) != HAL_OK) return HAL_ERROR;

        if ((sr1 & FLASH_SR1_WIP) == 0U)
        {
            if (sr1 & (FLASH_SR1_P_ERR | FLASH_SR1_E_ERR))
            {
                (void)flash_cmd(hspi, FLASH_OP_CLSR);
                return HAL_ERROR;
            }
            return HAL_OK;
        }

        if ((HAL_GetTick() - t0) > timeout_ms) return HAL_TIMEOUT;
    }
}

static HAL_StatusTypeDef flash_write_enable(SPI_HandleTypeDef *hspi)
{
    uint8_t sr1;

    if (flash_cmd(hspi, FLASH_OP_WREN) != HAL_OK) return HAL_ERROR;
    if (flash_read_status(hspi, &sr1) != HAL_OK) return HAL_ERROR;

    return (sr1 & FLASH_SR1_WEL) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef flash_erase_cmd(SPI_HandleTypeDef *hspi, uint8_t op, uint32_t addr, uint32_t timeout_ms)
{
    uint8_t tx[5];

    if (hspi == NULL || addr >= FLASH_CHIP_SIZE) return HAL_ERROR;
    if (flash_wait_ready(hspi, timeout_ms) != HAL_OK) return HAL_ERROR;
    if (flash_write_enable(hspi) != HAL_OK) return HAL_ERROR;

    tx[0] = op;
    tx[1] = (uint8_t)(addr >> 24);
    tx[2] = (uint8_t)(addr >> 16);
    tx[3] = (uint8_t)(addr >> 8);
    tx[4] = (uint8_t)addr;
    if (flash_xfer(hspi, tx, NULL, 5U) != HAL_OK) return HAL_ERROR;

    return flash_wait_ready(hspi, timeout_ms);
}

HAL_StatusTypeDef flash_read_id(SPI_HandleTypeDef *hspi, uint8_t *id)
{
    uint8_t tx[1U + FLASH_ID_LEN] = { 0 };
    uint8_t rx[1U + FLASH_ID_LEN] = { 0 };

    if (hspi == NULL || id == NULL) return HAL_ERROR;

    tx[0] = FLASH_OP_RDID;
    if (flash_xfer(hspi, tx, rx, 1U + FLASH_ID_LEN) != HAL_OK) return HAL_ERROR;

    memcpy(id, &rx[1], FLASH_ID_LEN);
    return HAL_OK;
}

HAL_StatusTypeDef flash_check_id(SPI_HandleTypeDef *hspi)
{
    uint8_t id[FLASH_ID_LEN];

    if (flash_read_id(hspi, id) != HAL_OK) return HAL_ERROR;

    return ((id[0] == FLASH_MANUF_ID) &&
            (id[1] == FLASH_DEV_ID_MSB) &&
            (id[2] == FLASH_DEV_ID_LSB)) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef flash_read_status(SPI_HandleTypeDef *hspi, uint8_t *sr1)
{
    uint8_t tx[2] = { FLASH_OP_RDSR1, 0x00U };
    uint8_t rx[2] = { 0, 0 };

    if (hspi == NULL || sr1 == NULL) return HAL_ERROR;
    if (flash_xfer(hspi, tx, rx, 2U) != HAL_OK) return HAL_ERROR;

    *sr1 = rx[1];
    return HAL_OK;
}

HAL_StatusTypeDef flash_read(SPI_HandleTypeDef *hspi, uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t  tx[5U + FLASH_CHUNK];
    uint8_t  rx[5U + FLASH_CHUNK];
    uint32_t done = 0U;

    if (hspi == NULL || data == NULL || len == 0U) return HAL_ERROR;
    if (addr >= FLASH_CHIP_SIZE || len > FLASH_CHIP_SIZE - addr) return HAL_ERROR;
    if (flash_wait_ready(hspi, FLASH_TIMEOUT_SE_MS) != HAL_OK) return HAL_ERROR;

    memset(tx, 0, sizeof(tx));
    while (done < len)
    {
        uint32_t n = len - done;
        uint32_t a = addr + done;

        if (n > FLASH_CHUNK) n = FLASH_CHUNK;

        tx[0] = FLASH_OP_4READ;
        tx[1] = (uint8_t)(a >> 24);
        tx[2] = (uint8_t)(a >> 16);
        tx[3] = (uint8_t)(a >> 8);
        tx[4] = (uint8_t)a;
        if (flash_xfer(hspi, tx, rx, (uint16_t)(5U + n)) != HAL_OK) return HAL_ERROR;

        memcpy(&data[done], &rx[5], n);
        done += n;
    }
    return HAL_OK;
}

HAL_StatusTypeDef flash_write(SPI_HandleTypeDef *hspi, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint8_t  tx[5U + FLASH_CHUNK];
    uint32_t done = 0U;

    if (hspi == NULL || data == NULL || len == 0U) return HAL_ERROR;
    if (addr >= FLASH_CHIP_SIZE || len > FLASH_CHIP_SIZE - addr) return HAL_ERROR;

    while (done < len)
    {
        uint32_t a         = addr + done;
        uint32_t page_left = FLASH_PG_SIZE - (a % FLASH_PG_SIZE);
        uint32_t n         = len - done;

        if (n > FLASH_CHUNK)   n = FLASH_CHUNK;
        if (n > page_left)     n = page_left;

        if (flash_wait_ready(hspi, FLASH_TIMEOUT_PP_MS) != HAL_OK) return HAL_ERROR;
        if (flash_write_enable(hspi) != HAL_OK) return HAL_ERROR;

        tx[0] = FLASH_OP_4PP;
        tx[1] = (uint8_t)(a >> 24);
        tx[2] = (uint8_t)(a >> 16);
        tx[3] = (uint8_t)(a >> 8);
        tx[4] = (uint8_t)a;
        memcpy(&tx[5], &data[done], n);
        if (flash_xfer(hspi, tx, NULL, (uint16_t)(5U + n)) != HAL_OK) return HAL_ERROR;

        if (flash_wait_ready(hspi, FLASH_TIMEOUT_PP_MS) != HAL_OK) return HAL_ERROR;
        done += n;
    }
    return HAL_OK;
}

HAL_StatusTypeDef flash_erase_sector(SPI_HandleTypeDef *hspi, uint32_t addr)
{
    return flash_erase_cmd(hspi, FLASH_OP_4SE, addr, FLASH_TIMEOUT_SE_MS);
}

HAL_StatusTypeDef flash_erase_block(SPI_HandleTypeDef *hspi, uint32_t addr)
{
    return flash_erase_cmd(hspi, FLASH_OP_4BE, addr, FLASH_TIMEOUT_BE_MS);
}

HAL_StatusTypeDef flash_erase_chip(SPI_HandleTypeDef *hspi)
{
    if (hspi == NULL) return HAL_ERROR;
    if (flash_wait_ready(hspi, FLASH_TIMEOUT_BE_MS) != HAL_OK) return HAL_ERROR;
    if (flash_write_enable(hspi) != HAL_OK) return HAL_ERROR;
    if (flash_cmd(hspi, FLASH_OP_CE) != HAL_OK) return HAL_ERROR;

    return flash_wait_ready(hspi, FLASH_TIMEOUT_CE_MS);
}
/************IMU***************/
/*
#ifndef IMU_3DM_CV7_H
#define IMU_3DM_CV7_H

#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdbool.h>

// Cổng UART giao tiếp với IMU (theo yêu cầu của bạn là USART1)
extern UART_HandleTypeDef huart1;

// Định nghĩa chân nguồn PE3
#define IMU_PWR_PORT    GPIOE
#define IMU_PWR_PIN     GPIO_PIN_3

// Các hàm Bring-up chính
void IMU_Power_On(void);
bool IMU_Bringup_Sequence(void);
bool IMU_Parse_Euler_Data(uint8_t *rx_data, float *roll, float *pitch, float *yaw);

#endif // IMU_3DM_CV7_H
#include "imu_3dm_cv7.h"

// -----------------------------------------------------------------------------
// HÀM BỔ TRỢ: TÍNH CHECKSUM FLETCHER-16 VÀ GỬI GÓI TIN MIP
// -----------------------------------------------------------------------------
static void MIP_Send_Packet(uint8_t desc_set, uint8_t payload_len, uint8_t *payload) {
    uint8_t packet[128];
    
    packet[0] = 0x75; // Sync 1
    packet[1] = 0x65; // Sync 2
    packet[2] = desc_set;
    packet[3] = payload_len;
    
    memcpy(&packet[4], payload, payload_len);
    
    // Tính Checksum Fletcher-16 trên toàn bộ payload
    uint8_t sum1 = 0, sum2 = 0;
    for (uint16_t i = 2; i < (4 + payload_len); i++) {
        sum1 += packet[i];
        sum2 += sum1;
    }
    
    packet[4 + payload_len] = sum1; // Checksum MSB
    packet[5 + payload_len] = sum2; // Checksum LSB
    
    // Gửi mảng qua USART1
    HAL_UART_Transmit(&huart1, packet, 6 + payload_len, 100);
}

// -----------------------------------------------------------------------------
// BƯỚC 1: BẬT NGUỒN PE3 & CHỜ NGUỒN ỔN ĐỊNH
// -----------------------------------------------------------------------------
void IMU_Power_On(void) {
    HAL_GPIO_WritePin(IMU_PWR_PORT, IMU_PWR_PIN, GPIO_PIN_SET); // PE3 = HIGH
    HAL_Delay(100); // Chờ 100ms cho IC Load Switch mở và IMU boot nguồn nội
}

// -----------------------------------------------------------------------------
// BƯỚC 2: CHUYỂN IMU VỀ TRẠNG THÁI IDLE (TẮT STREAM TẠM THỜI)
// -----------------------------------------------------------------------------
static bool IMU_Set_Idle(void) {
    uint8_t payload[] = {0x02, 0x02}; // Field Length = 2, Field Desc = 0x02 (Set Idle)
    uint8_t rx_buf[10] = {0};

    MIP_Send_Packet(0x01, sizeof(payload), payload);
    
    // Chờ nhận ACK phản hồi
    if (HAL_UART_Receive(&huart1, rx_buf, 10, 100) == HAL_OK) {
        if (rx_buf[0] == 0x75 && rx_buf[1] == 0x65 && rx_buf[7] == 0x00) {
            return true; // ACK OK
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// BƯỚC 3: ĐỌC VÀ CHECK ID TÊN MODEL
// -----------------------------------------------------------------------------
static bool IMU_Check_ID(void) {
    uint8_t payload[] = {0x02, 0x03}; // Field Length = 2, Field Desc = 0x03 (Get Device Info)
    uint8_t rx_buf[90] = {0};

    MIP_Send_Packet(0x01, sizeof(payload), payload);

    if (HAL_UART_Receive(&huart1, rx_buf, 86, 200) == HAL_OK) {
        if (rx_buf[0] == 0x75 && rx_buf[1] == 0x65 && rx_buf[5] == 0x81) {
            // Ép kiểu kiểm tra xem trong phản hồi có chuỗi "CV7" hoặc "3DM" không
            if (strstr((char*)&rx_buf[6], "CV7") != NULL || strstr((char*)&rx_buf[6], "3DM") != NULL) {
                return true; // ID chính xác!
            }
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// BƯỚC 4: CÀI ĐẶT TẦN SỐ BẮN DỮ LIỆU GÓC EULER LÀ 50HZ
// -----------------------------------------------------------------------------
static bool IMU_Set_Rate_50Hz(void) {
    // Tần số gốc AHRS EKF = 500Hz -> Cần hệ số chia Decimation = 500/50 = 10 (0x000A trong Hex)
    uint8_t payload[] = {
        0x07,       // Field Length (7 bytes)
        0x08,       // Field Desc: Set Message Format (0x08)
        0x01,       // Function Selector: Apply New Settings (0x01)
        0x82,       // Descriptor Set: AHRS/EKF Data Set (0x82)
        0x01,       // Number of Fields (1 field)
        0x05,       // Field 1 Desc: Euler Angles (0x05)
        0x00, 0x0A  // Decimation = 10 (cho ra tần số 50Hz)
    };
    uint8_t rx_buf[10] = {0};

    MIP_Send_Packet(0x0C, sizeof(payload), payload);

    if (HAL_UART_Receive(&huart1, rx_buf, 10, 100) == HAL_OK) {
        if (rx_buf[0] == 0x75 && rx_buf[1] == 0x65 && rx_buf[7] == 0x00) {
            return true; // Set 50Hz THÀNH CÔNG
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// BƯỚC 5: KÍCH HOẠT CHO IMU BẮT ĐẦU BẮN DỮ LIỆU LIÊN TỤC (RESUME STREAM)
// -----------------------------------------------------------------------------
static bool IMU_Enable_Stream(void) {
    uint8_t payload[] = {0x02, 0x06}; // Field Desc = 0x06 (Resume Stream)
    uint8_t rx_buf[10] = {0};

    MIP_Send_Packet(0x01, sizeof(payload), payload);

    if (HAL_UART_Receive(&huart1, rx_buf, 10, 100) == HAL_OK) {
        if (rx_buf[0] == 0x75 && rx_buf[1] == 0x65 && rx_buf[7] == 0x00) {
            return true; // Bắt đầu Stream!
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// TOÀN BỘ QUY TRÌNH BRING-UP HOÀN CHỈNH (GỌI HÀM NÀY TRONG MAIN)
// -----------------------------------------------------------------------------
bool IMU_Bringup_Sequence(void) {
    // 1. Kích chân PE3 mở nguồn
    IMU_Power_On();

    // 2. Chuyển về Idle
    if (!IMU_Set_Idle()) {
        return false; // Lỗi không đưa được về Idle
    }

    // 3. Check ID
    if (!IMU_Check_ID()) {
        return false; // Lỗi sai ID hoặc không phản hồi
    }

    // 4. Cài tần số 50Hz
    if (!IMU_Set_Rate_50Hz()) {
        return false; // Lỗi cài đặt tần số
    }

    // 5. Bật Stream liên tục
    if (!IMU_Enable_Stream()) {
        return false; // Lỗi không mở được Stream
    }

    return true; // BRING-UP THÀNH CÔNG 100%!
}

3. Sử dụng trong main.c

#include "main.h"
#include "imu_3dm_cv7.h"

int main(void) {
    // Khởi tạo HAL, Clock, GPIO, USART1...
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    // Thực thi Bring-up IMU
    if (IMU_Bringup_Sequence()) {
        // BRING-UP THÀNH CÔNG! 
        // Lúc này IMU đang bắn dữ liệu 50Hz liên tục về USART1.
        // Bạn có thể bật UART DMA Circular Mode để hứng dữ liệu ở đây.
    } else {
        // BRING-UP THẤT BẠI! Xử lý đèn báo lỗi hoặc Reset lại.
    }

    while (1) {
        // Vòng lặp chính xử lý công việc khác...
    }
}
*/
/*
**************GNSS**********************
#ifndef GNSS_H
#define GNSS_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

 ==================================================================
 * 1. ĐỊNH NGHĨA FRAME TRUYỀN (SKYTRAQ BINARY)
 * ================================================================== 
#define GNSS_SYNC1          0xA0
#define GNSS_SYNC2          0xA1
#define GNSS_END1           0x0D // Ký tự '\r'
#define GNSS_END2           0x0A // Ký tự '\n'

==================================================================
 * 2. ĐỊNH NGHĨA MESSAGE ID (LỆNH CẤU HÌNH & TRẠNG THÁI)
 * Lưu ý: Chọn bừa một số chuẩn của hệ SkyTraq. 
 * SAU NÀY BẠN THAY BẰNG SỐ THẬT THEO TÀI LIỆU CỦA ORION B17 NHÉ!
 * ================================================================== 
#define GNSS_CMD_PING       0x02  // data ack ping
#define GNSS_ACK_PING       0x83  // command báo là lệnh ack ping
#define GNSS_CMD_CFG_RATE   0x0E  // ID Lệnh cài đặt Update Rate
#define GNSS_ACK_ACK        0x83  // ID Phản hồi báo cấu hình thành công (ACK chung)

 ==================================================================
 * 3. HÀM GIAO TIẾP (API)
 * ================================================================== 
void GNSS_Power_On(void);
bool GNSS_Ping(void);
bool GNSS_Set_Update_Rate(uint8_t rate_hz);
void GNSS_Bringup_Sequence(void);

#endif 
#endif 

//gnss.c
#include "gnss.h"
#include <string.h>

// Handle UART giao tiếp với GNSS (ORION_TXD_B / RXD_B)
extern UART_HandleTypeDef huart3; 

/* --- KHAI BÁO CHÂN PHẦN CỨNG THỰC TẾ CỦA BẠN --- 
#define GNSS_PWR_PORT   GPIOC             // Load Switch cấp nguồn
#define GNSS_PWR_PIN    GPIO_PIN_13

#define GNSS_RESET_PORT GPIOD             // Chân Reset (Active Low)
#define GNSS_RESET_PIN  GPIO_PIN_6

#define GNSS_1PPS_PORT  GPIOD             // Chân 1PPS đồng bộ thời gian (EXTI)
#define GNSS_1PPS_PIN   GPIO_PIN_7

/* ==================================================================
 * Hàm tính Checksum XOR 8-bit trên Payload
 * ================================================================== 
static uint8_t GNSS_Calc_Checksum(uint8_t *payload, uint16_t len) 
{
    uint8_t cs = 0;
    for (uint16_t i = 0; i < len; i++) {
        cs ^= payload[i]; // Phép XOR liên tục các byte
    }
    return cs;
}

/* ==================================================================
 * Hàm đóng gói và GỬI lệnh Binary (Master -> GNSS)
 * ================================================================== 
static void GNSS_Send_Command(uint8_t msg_id, uint8_t *data, uint16_t data_len) 
{
    uint8_t tx_buf[128];
    uint16_t payload_len = data_len + 1; // +1 byte vì Message ID nằm trong Payload
    
    // Header
    tx_buf[0] = GNSS_SYNC1;
    tx_buf[1] = GNSS_SYNC2;
    tx_buf[2] = (payload_len >> 8) & 0xFF; // Length MSB (Big Endian)
    tx_buf[3] = payload_len & 0xFF;        // Length LSB
    
    // Payload (Message ID + Data)
    tx_buf[4] = msg_id;
    if (data_len > 0 && data != NULL) {
        memcpy(&tx_buf[5], data, data_len);
    }
    
    // Checksum tính từ byte msg_id
    tx_buf[4 + payload_len] = GNSS_Calc_Checksum(&tx_buf[4], payload_len);
    
    // Khóa đuôi (End sequence)
    tx_buf[5 + payload_len] = GNSS_END1;
    tx_buf[6 + payload_len] = GNSS_END2;
    
    // Bắn nguyên mảng qua UART
    HAL_UART_Transmit(&huart3, tx_buf, 7 + payload_len, 100);
}

/* ==================================================================
 * BƯỚC 1: BẬT NGUỒN VÀ KHỞI ĐỘNG SẠCH GNSS
 * ================================================================== 
void GNSS_Power_On(void) 
{
    // 1. Mở nguồn Load Switch
    HAL_GPIO_WritePin(GNSS_PWR_PORT, GNSS_PWR_PIN, GPIO_PIN_SET);
    HAL_Delay(50); 

    // 2. Kéo chân Reset xuống LOW để ép GNSS khởi động lại sạch sẽ
    HAL_GPIO_WritePin(GNSS_RESET_PORT, GNSS_RESET_PIN, GPIO_PIN_RESET); 
    HAL_Delay(100); 
    
    // 3. Nhả chân Reset lên HIGH để module chính thức boot hệ điều hành
    HAL_GPIO_WritePin(GNSS_RESET_PORT, GNSS_RESET_PIN, GPIO_PIN_SET);   
    
    // 4. Chờ hệ thống GNSS khởi động xong 
    HAL_Delay(500); 
}

/* ==================================================================
 * BƯỚC 2: PING KIỂM TRA GIAO TIẾP
 * ================================================================== 
bool GNSS_Ping(void) 
{
    uint8_t rx_buf[20] = {0};

    // Gửi lệnh Ping không kèm theo data
    GNSS_Send_Command(GNSS_CMD_PING, NULL, 0);

    // Chờ nhận phản hồi
    if (HAL_UART_Receive(&huart3, rx_buf, 15, 200) == HAL_OK) 
    {
        // Kiểm tra đúng Sync và ID phản hồi
        if (rx_buf[0] == GNSS_SYNC1 && rx_buf[1] == GNSS_SYNC2 && rx_buf[4] == GNSS_ACK_PING) 
        {
            return true; 
        }
    }
    return false;
}

/* ==================================================================
 * BƯỚC 3: CẤU HÌNH TỐC ĐỘ CẬP NHẬT TỌA ĐỘ
 * ================================================================== 
bool GNSS_Set_Update_Rate(uint8_t rate_hz) 
{
    uint8_t rx_buf[10] = {0};
    uint8_t payload_data[1] = {rate_hz}; // Data đính kèm (Ví dụ: 5Hz)

    // Gửi lệnh
    GNSS_Send_Command(GNSS_CMD_CFG_RATE, payload_data, 1);

    // Chờ phản hồi ACK
    if (HAL_UART_Receive(&huart3, rx_buf, 9, 200) == HAL_OK) 
    {
        if (rx_buf[0] == GNSS_SYNC1 && rx_buf[1] == GNSS_SYNC2 && rx_buf[4] == GNSS_ACK_ACK) 
        {
            return true;
        }
    }
    return false;
}
//main.c
#include "gnss.h"

// Gọi hàm in log debug UART của bạn
extern void debug_send(const char *str);

/* ==================================================================
 * LUỒNG TEST BRING-UP TUẦN TỰ CHO ORION B17 GN
 * ================================================================== 
void GNSS_Bringup_Sequence(void) 
{
    debug_send("\r\n--- STARTING GNSS ORION B17 BRING-UP ---\r\n");

    // 1. Cấp nguồn và thực hiện chu trình Reset chân PD6
    GNSS_Power_On();
    debug_send("[1] GNSS Power ON & Hard Reset... OK\r\n");

    // Clear cờ rác UART để tránh lỗi Overrun sau khi khởi động
    __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_OREF);
    
    // 2. Ping thiết bị để kiểm tra dây TX/RX và Baudrate
    if (GNSS_Ping()) 
    {
        debug_send("[2] GNSS Ping... SUCCESS (Device is Alive!)\r\n");
    } 
    else 
    {
        debug_send("[2] GNSS Ping... FAIL (Check UART connection/Baudrate)\r\n");
        return; // Dừng lại nếu phần cứng không giao tiếp được
    }

    // 3. Cấu hình tốc độ (Ví dụ set 5Hz)
    if (GNSS_Set_Update_Rate(5)) 
    {
        debug_send("[3] GNSS Set 5Hz Rate... SUCCESS\r\n");
    } 
    else 
    {
        debug_send("[3] GNSS Set 5Hz Rate... FAIL\r\n");
    }

    debug_send("--- GNSS BRING-UP COMPLETE ---\r\n");
}

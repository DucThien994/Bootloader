#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define FLASH_INTERFACE_BASE_ADDR   0x40023C00
#define USART1_BASE_ADDR            0x40011000
#define DMA2_BASE_ADDR              0x40026400
#define RCC_BASE_ADDR               0x40023800
#define GPIOB_BASE_ADDR             0x40020400
#define GPIOD_BASE_ADDR             0x40020C00

#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))

/* GPIOB Registers (PB6 TX, PB7 RX) */
#define GPIOB_MODER                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OSPEEDR               (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_AFRL                  (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x20))

/* GPIOD Registers (LED PD12..PD15) */
#define GPIOD_MODER                 (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x00))
#define GPIOD_ODR                   (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x14))

/* USART1 Registers */
#define USART1_SR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x00))
#define USART1_DR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x04))
#define USART1_BRR                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x08))
#define USART1_CR1                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x0C))
#define USART1_CR3                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x14))

/* DMA2 Stream 2 Registers (USART1 RX: Channel 4) */
#define DMA2_LISR                   (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x00))
#define DMA2_LIFCR                  (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x08))
#define DMA2_S2CR                   (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x10 + 0x18 * 2))
#define DMA2_S2NDTR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x14 + 0x18 * 2))
#define DMA2_S2PAR                  (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x18 + 0x18 * 2))
#define DMA2_S2M0AR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x1C + 0x18 * 2))

/* Flash Interface Registers */
#define FLASH_KEYR                  (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x04))
#define FLASH_SR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C))
#define FLASH_CR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10))

/* NVIC & SCB Registers */
#define NVIC_ISER1                  (*(volatile uint32_t*)0xE000E104)
#define NVIC_ICER1                  (*(volatile uint32_t*)0xE000E184)
#define SCB_AIRCR                   (*(volatile uint32_t*)0xE000ED0C)

/* ============================================================================
 * CẤU HÌNH ĐỊA CHỈ VÀ BỘ ĐỆM
 * ============================================================================
 * Firmware mới được ghi đè trực tiếp từ Sector 0 (0x08000000), sau đó chip tự động
 * System Reset để khởi động thẳng vào firmware mới, thay thế vĩnh viễn FW cũ/Bootloader.
 */
#define APP_ADDRESS                 0x08000000
#define ram_in_func                 __attribute__((section(".Function_in_Ram")))

/* Đệm nhận Firmware 48 KB */
#define RX_BUFFER_SIZE              (48 * 1024)
uint8_t rx_buf[RX_BUFFER_SIZE];

/* Đệm nhận lệnh điều khiển ("led on", "led off", "update") */
#define CMD_BUF_SIZE                64
char cmd_buf[CMD_BUF_SIZE];
volatile uint8_t cmd_idx = 0;
volatile uint8_t cmd_ready = 0;

/* Khai báo nguyên mẫu hàm */
void usart1_send(char data);
void my_printf(const char* str, ...);
void USART1_Config(void);
void LED_Init(void);
void USART1_IRQHandler(void);
void trim_str(char* str);
void process_update(void);

ram_in_func void flash_unlock(void);
ram_in_func void flash_erase_sector(int sec_num);
ram_in_func void flash_erase_for_firmware(uint32_t size);
ram_in_func void flash_program_byte(uint32_t addr, uint8_t val);
ram_in_func void flash_and_reset(uint32_t fw_size);

/* ============================================================================
 * TRUYỀN DỮ LIỆU USART1 & HÀM PRINTF TỰ ĐỊNH NGHĨA
 * ============================================================================
 */
void usart1_send(char data)
{
    while (((USART1_SR >> 7) & 1) == 0); // Chờ cờ TXE = 1
    USART1_DR = (data & 0xFF);
}

void my_printf(const char* str, ...)
{
    va_list list;
    va_start(list, str);
    char print_buf[128] = {0};
    vsnprintf(print_buf, sizeof(print_buf), str, list);
    int len = strlen(print_buf);
    for (int i = 0; i < len; i++) {
        usart1_send(print_buf[i]);
    }
    va_end(list);
}

/* ============================================================================
 * CẤU HÌNH USART1 (PB6 - TX, PB7 - RX) 115200 BAUD, BẬT NGẮT RXNE
 * ============================================================================
 */
void USART1_Config(void)
{
    // Bật clock GPIOB (Bit 1 AHB1ENR) và USART1 (Bit 4 APB2ENR)
    RCC_AHB1ENR |= (1 << 1);
    RCC_APB2ENR |= (1 << 4);

    // Cấu hình PB6 (TX), PB7 (RX) sang Alternate Function AF7
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |=  ((0x2 << 12) | (0x2 << 14));
    GPIOB_AFRL  &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL  |=  ((0x7 << 24) | (0x7 << 28));
    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR |=  ((0x1 << 12) | (0x1 << 14));

    // Baudrate 115200 @ 16MHz HSI: Mantissa = 8, Fraction = 11 (0x8B)
    USART1_BRR = 0x8B;

    // Bật USART1, Transmitter Enable (TE), Receiver Enable (RE), RXNEIE (Bit 5)
    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2) | (1 << 5);

    // Bật NVIC IRQ 37 (USART1)
    NVIC_ISER1 |= (1 << (37 - 32));
}

/* ============================================================================
 * CẤU HÌNH 4 LED TRÊN BOARD STM32F4 DISCOVERY (PD12, PD13, PD14, PD15)
 * ============================================================================
 */
void LED_Init(void)
{
    // Bật clock GPIOD (Bit 3 AHB1ENR)
    RCC_AHB1ENR |= (1 << 3);

    // Cấu hình PD12..PD15 là General Purpose Output (01b)
    GPIOD_MODER &= ~(0xFF << 24);
    GPIOD_MODER |=  (0x55 << 24);

    // Mặc định tắt 4 LED
    GPIOD_ODR &= ~(0xF << 12);
}

/* ============================================================================
 * HÀM XỬ LÝ CHUỖI: LOẠI BỎ KÝ TỰ KHOẢNG TRẮNG ĐẦU VÀ CUỐI
 * ============================================================================
 */
void trim_str(char* str)
{
    char* p = str;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (p != str) {
        memmove(str, p, strlen(p) + 1);
    }
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

/* ============================================================================
 * TRÌNH PHỤC VỤ NGẮT USART1: NHẬN LỆNH ĐIỀU KHIỂN
 * ============================================================================
 */
void USART1_IRQHandler(void)
{
    // Kiểm tra cờ nhận RXNE (Bit 5 trong USART1_SR)
    if ((USART1_SR & (1 << 5)) != 0)
    {
        char c = (char)(USART1_DR & 0xFF);

        if (c == '\r' || c == '\n')
        {
            if (cmd_idx > 0 && cmd_ready == 0)
            {
                cmd_buf[cmd_idx] = '\0';
                cmd_ready = 1;
            }
        }
        else
        {
            if (cmd_ready == 0 && cmd_idx < CMD_BUF_SIZE - 1)
            {
                cmd_buf[cmd_idx++] = c;
            }
        }
    }
}

/* ============================================================================
 * CÁC HÀM THAO TÁC FLASH VÀ RESET HỆ THỐNG (CHẠY TRÊN RAM)
 * Vì Sector 0 bị xóa và ghi đè nên các hàm này BẮT BUỘC phải nằm trên RAM (.Function_in_Ram)
 * ============================================================================
 */
ram_in_func void flash_unlock(void)
{
    if (((FLASH_CR >> 31) & 1) == 1) {
        FLASH_KEYR = 0x45670123;
        FLASH_KEYR = 0xCDEF89AB;
    }
}

ram_in_func void flash_erase_sector(int sec_num)
{
    flash_unlock();
    if (sec_num > 7) return;
    while (((FLASH_SR >> 16) & 1) == 1); // Chờ BSY = 0
    FLASH_SR = 0xF3;                      // Xóa các cờ lỗi cũ
    FLASH_CR &= ~(0x3 << 8);              // PSIZE = 00 (x8)
    FLASH_CR &= ~(0x1F << 3);             // Xóa số sector cũ (SNB)
    FLASH_CR |= ((sec_num & 0x1F) << 3);  // Chọn số sector cần xóa
    FLASH_CR |= (1 << 1);                 // SER = 1 (Sector Erase)
    FLASH_CR |= (1 << 16);                // STRT = 1 (Start Erase)
    while (((FLASH_SR >> 16) & 1) == 1); // Chờ xóa hoàn tất (BSY = 0)
    FLASH_CR &= ~(1 << 1);                // Xóa cờ SER
    FLASH_CR &= ~(0x1F << 3);             // Xóa cờ SNB
}

ram_in_func void flash_erase_for_firmware(uint32_t size)
{
    // Sector 0: 0x08000000 - 0x08003FFF (16KB)
    flash_erase_sector(0);

    // Sector 1: 0x08004000 - 0x08007FFF (16KB, tổng 32KB)
    if (size > 16384) {
        flash_erase_sector(1);
    }

    // Sector 2: 0x08008000 - 0x0800BFFF (16KB, tổng 48KB)
    if (size > 32768) {
        flash_erase_sector(2);
    }

    // Sector 3: 0x0800C000 - 0x0800FFFF (16KB, tổng 64KB)
    if (size > 49152) {
        flash_erase_sector(3);
    }
}

ram_in_func void flash_program_byte(uint32_t addr, uint8_t val)
{
    flash_unlock();
    while (((FLASH_SR >> 16) & 1) == 1); // Chờ BSY = 0
    FLASH_SR = 0xF3;                      // Xóa các cờ lỗi
    FLASH_CR &= ~(0x3 << 8);              // PSIZE = 00 (8-bit)
    FLASH_CR |= (1 << 0);                 // PG = 1 (Programming mode)
    *(volatile uint8_t*)addr = val;
    while (((FLASH_SR >> 16) & 1) == 1); // Chờ ghi xong byte (BSY = 0)
    FLASH_CR &= ~(1 << 0);                // PG = 0
}

ram_in_func void flash_and_reset(uint32_t fw_size)
{
    // 1. Xóa các sector tương ứng bắt đầu từ Sector 0
    flash_erase_for_firmware(fw_size);

    // 2. Ghi đè toàn bộ dữ liệu firmware mới từ địa chỉ 0x08000000
    for (uint32_t i = 0; i < fw_size; i++) {
        flash_program_byte(APP_ADDRESS + i, rx_buf[i]);
    }

    // 3. Khóa Flash lại (Lock bit 31)
    FLASH_CR |= (1 << 31);

    // 4. Delay nhỏ trong RAM
    for (volatile uint32_t d = 0; d < 500000; d++);

    // 5. Kích hoạt System Reset để MCU khởi động thẳng vào Firmware mới
    SCB_AIRCR = (0x5FA << 16) | (1 << 2);
    __asm volatile ("dsb\n isb\n");

    while (1);
}

/* ============================================================================
 * QUY TRÌNH NẠP FIRMWARE MỚI KHI NHẬN LỆNH "update"
 * ============================================================================
 */
void process_update(void)
{
    // 1. Tắt toàn bộ ngắt để chuẩn bị update FW
    __asm volatile ("cpsid i");
    NVIC_ICER1 = (1 << (37 - 32));         // Tắt ngắt USART1 trong NVIC
    USART1_CR1 &= ~((1 << 5) | (1 << 4));  // Tắt RXNEIE, IDLEIE

    // 2. In thông báo yêu cầu nhập dung lượng cho Firmware mới
    my_printf("\r\nNhap dung luong cho firmware moi (bytes): ");
    while (((USART1_SR >> 6) & 1) == 0);   // Chờ gửi xong chuỗi (TC = 1)

    // 3. Đọc dung lượng từ UART (Polling cờ RXNE vì ngắt đã tắt)
    char size_buf[32] = {0};
    uint8_t s_idx = 0;
    while (1)
    {
        while (((USART1_SR >> 5) & 1) == 0); // Chờ RXNE = 1
        char c = (char)(USART1_DR & 0xFF);
        usart1_send(c); // Echo ký tự để người dùng thấy trên Terminal

        if (c == '\r' || c == '\n')
        {
            if (s_idx > 0)
            {
                size_buf[s_idx] = '\0';
                break;
            }
        }
        else if (c == '\b' || c == 0x7F) // Hỗ trợ xóa khi bấm Backspace
        {
            if (s_idx > 0)
            {
                s_idx--;
                usart1_send(' ');
                usart1_send('\b');
            }
        }
        else if (c >= '0' && c <= '9')
        {
            if (s_idx < sizeof(size_buf) - 1)
            {
                size_buf[s_idx++] = c;
            }
        }
    }

    // Chuyển chuỗi số sang giá trị nguyên
    uint32_t fw_size = 0;
    for (int i = 0; size_buf[i] != '\0'; i++)
    {
        fw_size = fw_size * 10 + (size_buf[i] - '0');
    }

    // Xả sạch ký tự thừa (như '\n' đi kèm '\r' nếu terminal gửi CRLF)
    for (volatile uint32_t d = 0; d < 200000; d++);
    while ((USART1_SR & (1 << 5)) != 0)
    {
        volatile uint32_t flush = USART1_DR;
        (void)flush;
    }

    // Kiểm tra dung lượng hợp lệ
    if (fw_size == 0 || fw_size > RX_BUFFER_SIZE)
    {
        my_printf("\r\nLoi: Dung luong khong hop le (1..%d bytes)!\r\n", RX_BUFFER_SIZE);
        // Bật lại ngắt và quay về chế độ thường
        USART1_CR1 |= (1 << 5);
        NVIC_ISER1 |= (1 << (37 - 32));
        __asm volatile ("cpsie i");
        return;
    }

    my_printf("\r\nDung luong nhan duoc: %lu bytes.\r\n", (unsigned long)fw_size);
    my_printf("San sang nhan file qua UART (gui file binary ngay bay gio)...\r\n");
    while (((USART1_SR >> 6) & 1) == 0);

    // 4. Nhận file firmware mới bằng DMA2 Stream 2 Channel 4
    RCC_AHB1ENR |= (1 << 22); // Cấp clock cho DMA2

    // Tắt Stream 2 trước khi cấu hình
    DMA2_S2CR &= ~(1 << 0);
    while ((DMA2_S2CR & (1 << 0)) != 0);

    // Xóa cờ ngắt Stream 2
    DMA2_LIFCR = (0x3D << 16);

    // Xóa cờ lỗi và dữ liệu cũ của USART1
    volatile uint32_t temp_sr = USART1_SR;
    volatile uint32_t temp_dr = USART1_DR;
    (void)temp_sr;
    (void)temp_dr;

    // Cấu hình DMA2 Stream 2 Channel 4 nhận đúng fw_size bytes vào rx_buf
    DMA2_S2PAR  = (uint32_t)(USART1_BASE_ADDR + 0x04);
    DMA2_S2M0AR = (uint32_t)rx_buf;
    DMA2_S2NDTR = fw_size;

    // Bật cờ DMAR trong USART1_CR3
    USART1_CR3 |= (1 << 6);

    // Bật DMA2 Stream 2: Channel 4, MINC (Bit 10), Priority High (Bits 17:16 = 10b), EN (Bit 0)
    DMA2_S2CR = (4 << 25) | (2 << 16) | (1 << 10) | (1 << 0);

    // Polling kiểm tra cờ hoàn tất nhận dữ liệu DMA (TCIF2: Bit 21 trong DMA2_LISR)
    while (((DMA2_LISR >> 21) & 1) == 0);

    // Nhận xong, tắt DMA và DMAR
    DMA2_S2CR &= ~(1 << 0);
    while ((DMA2_S2CR & (1 << 0)) != 0);
    USART1_CR3 &= ~(1 << 6);
    DMA2_LIFCR = (0x3D << 16);

    // 5. Kiểm tra tính hợp lệ của Vector Table trước khi ghi đè Flash
    uint32_t initial_sp = *(uint32_t*)&rx_buf[0];
    uint32_t reset_handler = *(uint32_t*)&rx_buf[4];

    // MSP phải nằm trong vùng RAM STM32F411 (0x20000000 - 0x20020000)
    // Reset_Handler phải nằm trong Flash (0x08000000 đến 0x08000000 + fw_size) và có bit Thumb (bit 0 = 1)
    if (initial_sp < 0x20000000 || initial_sp > 0x20020000 ||
        reset_handler < 0x08000000 || reset_handler > (0x08000000 + fw_size) ||
        (reset_handler & 1) == 0)
    {
        my_printf("\r\nLoi: Du lieu binary khong hop le (SP=0x%08lX, Reset=0x%08lX)! Huy nap de tranh brick board.\r\n",
                  (unsigned long)initial_sp, (unsigned long)reset_handler);
        USART1_CR1 |= (1 << 5);
        NVIC_ISER1 |= (1 << (37 - 32));
        __asm volatile ("cpsie i");
        return;
    }

    // 6. Thông báo trước khi thực hiện ghi đè Flash và Reset
    my_printf("\r\nDa nhan du %lu bytes thanh cong!\r\n", (unsigned long)fw_size);
    my_printf("Dang xoa Flash Sector 0 va ghi de truc tiep tu 0x08000000...\r\n");
    my_printf("He thong se tu dong Reset vao Firmware moi, khong the quay lai fw cu!\r\n");
    while (((USART1_SR >> 6) & 1) == 0); // Chờ UART TX hoàn tất 100%

    // 7. Thực thi hàm ghi đè Flash và Reset trên RAM
    flash_and_reset(fw_size);
}

/* ============================================================================
 * HÀM MAIN CHÍNH
 * ============================================================================
 */
int main(void)
{
    // Khởi tạo các ngoại vi bằng thanh ghi
    LED_Init();
    USART1_Config();

    my_printf("\r\n==========================================\r\n");
    my_printf(" STM32F411 Bootloader Ready!\r\n");
    my_printf(" Lenh: 'led on', 'led off', 'update'\r\n");
    my_printf("==========================================\r\n");

    while (1)
    {
        if (cmd_ready == 1)
        {
            trim_str(cmd_buf);

            if (strcmp(cmd_buf, "led on") == 0)
            {
                GPIOD_ODR |= (0xF << 12); // Bật 4 LED PD12..PD15
                my_printf("leds are on\r\n");
            }
            else if (strcmp(cmd_buf, "led off") == 0)
            {
                GPIOD_ODR &= ~(0xF << 12); // Tắt 4 LED PD12..PD15
                my_printf("leds are off\r\n");
            }
            else if (strcmp(cmd_buf, "update") == 0)
            {
                process_update();
            }
            else 
            {
                my_printf("ERROR!!! TRY AGAIN \r\n");
            }

            // Xóa buffer lệnh để sẵn sàng nhận lệnh mới
            cmd_idx = 0;
            cmd_buf[0] = '\0';
            cmd_ready = 0;
        }
    }

    return 0;
}

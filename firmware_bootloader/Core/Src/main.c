//#include "main.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <string.h>
//#include <stdarg.h>
//
//#define FLASH_INTERFACE_BASE_ADDR   0x40023C00
//#define USART1_BASE_ADDR            0x40011000
//#define DMA2_BASE_ADDR              0x40026400
//
//#define RCC_BASE_ADDR               0x40023800
//#define GPIOA_BASE_ADDR             0x40020000
//#define GPIOB_BASE_ADDR             0x40020400
//
//#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
//#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))
//
//#define GPIOB_MODER                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x00))
//#define GPIOB_OSPEEDR               (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x08))
//#define GPIOB_PUPDR                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x0C))
//#define GPIOB_AFRL                  (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x20))
//
//#define USART1_SR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x00))
//#define USART1_DR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x04))
//#define USART1_BRR                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x08))
//#define USART1_CR1                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x0C))
//#define USART1_CR3                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x14))
//
//#define DMA2_S2PAR                  (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x18 + 0x18 * 2))
//#define DMA2_S2M0AR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x1C + 0x18 * 2))
//#define DMA2_S2NDTR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x14 + 0x18 * 2))
//#define DMA2_S2CR                   (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x10 + 0x18 * 2))
//
//#define FLASH_CR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10))
//#define FLASH_SR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C))
//#define FLASH_KEYR                  (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x04))
//
//uint8_t rx_buf[1760];
//volatile uint32_t received_fw_size = 0;
//volatile uint8_t receive_new_fw = 0;
//
//void usart1_send(char data)
//{
//    // Chờ cho đến khi Data Register rỗng (TXE = bit 7)
//    while (((USART1_SR >> 7) & 1) == 0);
//    USART1_DR = (data & 0xFF);
//}
//
//void my_printf(char* str, ...)
//{
//    va_list list;
//    va_start(list, str);
//
//    char print_buf[128] = {0};
//    vsnprintf(print_buf, sizeof(print_buf), str, list);
//
//    int len = strlen(print_buf);
//    for (int i = 0; i < len; i++)
//    {
//        usart1_send(print_buf[i]);
//    }
//
//    va_end(list);
//}
//
//void USART1_Config(void)
//{
//	// enable clock
//    RCC_AHB1ENR |= (1 << 1);
//    RCC_APB2ENR |= (1 << 4);
//
//    //PB6 tx PB7 rx
//    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
//    GPIOB_MODER |=  ((0x2 << 12) | (0x2 << 14));
//    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
//    GPIOB_AFRL |=  ((0x7 << 24) | (0x7 << 28));  // AF7
//    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
//
//    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
//    GPIOB_PUPDR |=  ((0x1 << 12) | (0x1 << 14));
//    USART1_BRR = 0x683;
//
//    // Bật UART, TE, RE, và ngắt IDLE (bit 4 IDLEIE)
//    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2) | (1 << 4);
//
//    // 7. Bật DMA Receiver trong USART1 (DMAR = bit 6)
//    USART1_CR3 = (1 << 6);
//
//    // 8. Bật ngắt USART1 trong NVIC (IRQn = 37 -> ISER1 bit 5)
//    uint32_t* ISER1 = (uint32_t*)0xE000E104;
//    *ISER1 |= (1 << (37 - 32));
//}
//
//void DMA_Init(void)
//{
//    RCC_AHB1ENR |= (1 << 22);
//    DMA2_S2CR &= ~(1 << 0);  // disable stream
//    while ((DMA2_S2CR & (1 << 0)) != 0);
//
//    // Xóa cờ ngắt Stream 2 trong LIFCR
//    uint32_t* DMA_LIFCR = (uint32_t*)(DMA2_BASE_ADDR + 0x08);
//    *DMA_LIFCR |= (0x3D << 16);
//
//    DMA2_S2PAR  = (uint32_t)(USART1_BASE_ADDR + 0x04); // USART1->DR
//    DMA2_S2M0AR = (uint32_t)rx_buf;                    // Buffer RAM
//    DMA2_S2NDTR = sizeof(rx_buf);                      // 8192 bytes
//
//    // Channel 4 (bits 27:25 = 4), Memory Increment (bit 10 = 1)
//    DMA2_S2CR = (4 << 25) | (1 << 10);
//
//    // Bật DMA2 Stream 2
//    DMA2_S2CR |= (1 << 0);
//}
//
//// Ngắt USART1: Bắt sự kiện truyền xong file (IDLE Line)
//// void USART1_IRQHandler(void)
//// {
////     // Kiểm tra cờ IDLE (bit 4 trong USART_SR)
////     if ((USART1_SR & (1 << 4)) != 0)
////     {
////         // Xóa cờ IDLE bằng cách đọc SR rồi đọc DR
////         volatile uint32_t temp = USART1_SR;
////         temp = USART1_DR;
////         (void)temp;
//
////         // Tính số lượng byte thực tế đã nhận được từ DMA NDTR
////         uint32_t remaining = DMA2_S2NDTR;
////         uint32_t received = sizeof(rx_buf) - remaining;
//
////         if (received > 0)
////         {
////             // Tắt DMA Stream
////             DMA2_S2CR &= ~(1 << 0);
//
////             received_fw_size = received;
////             receive_new_fw = 1;
////         }
////     }
//// }
//
//// // =================== Flash Operation in RAM ===================
//
//// __attribute__((section(".Function_in_Ram"))) void flash_erase_sector(int sec_num)
//// {
////     // Mở khóa Flash nếu đang bị khóa
////     if (((FLASH_CR >> 31) & 1) == 1)
////     {
////         FLASH_KEYR = 0x45670123;
////         FLASH_KEYR = 0xCDEF89AB;
////     }
//
////     if (sec_num > 7) return;
//
////     // Chờ Flash rảnh (BSY = 0)
////     while (((FLASH_SR >> 16) & 1) == 1);
//
////     // Chọn Sector và bật chế độ Sector Erase (SER)
////     FLASH_CR &= ~(0xF << 3);
////     FLASH_CR |= (sec_num << 3);        // SNB[3:0]
////     FLASH_CR |= (1 << 1);              // SER = 1
//
////     // Bắt đầu xóa
////     FLASH_CR |= (1 << 16);             // STRT = 1
//
////     // Chờ hoàn thành (BSY = 0)
////     while (((FLASH_SR >> 16) & 1) == 1);
//
////     // Tắt cờ SER và SNB sau khi xóa xong
////     FLASH_CR &= ~(1 << 1);
////     FLASH_CR &= ~(0xF << 3);
//// }
//
//// __attribute__((section(".Function_in_Ram"))) void flash_program(uint8_t* addr, uint8_t val)
//// {
////     // Mở khóa Flash nếu đang bị khóa
////     if (((FLASH_CR >> 31) & 1) == 1)
////     {
////         FLASH_KEYR = 0x45670123;
////         FLASH_KEYR = 0xCDEF89AB;
////     }
//
////     // Chờ Flash rảnh
////     while (((FLASH_SR >> 16) & 1) == 1);
//
////     // Bật bit Program (PG)
////     FLASH_CR |= (1 << 0);
//
////     // Ghi byte vào địa chỉ Flash
////     *addr = val;
//
////     // Chờ ghi xong
////     while (((FLASH_SR >> 16) & 1) == 1);
//
////     // Tắt bit PG
////     FLASH_CR &= ~(1 << 0);
//// }
//
//// 1. Trong USART1_IRQHandler: Tắt ngắt sau khi nhận xong dữ liệu
//void USART1_IRQHandler(void)
//{
//    if ((USART1_SR & (1 << 4)) != 0) // Cờ IDLE
//    {
//        volatile uint32_t temp = USART1_SR;
//        temp = USART1_DR;
//        (void)temp;
//
//        uint32_t remaining = DMA2_S2NDTR;
//        uint32_t received = sizeof(rx_buf) - remaining;
//
//        if (received > 0)
//        {
//            DMA2_S2CR &= ~(1 << 0); // disable stream 2
//            USART1_CR1 &= ~(1 << 4); // Tắt IDLEIE
//            USART1_CR3 &= ~(1 << 6); // Tắt DMAR
//            // Tắt ngắt USART1 trong NVIC
//            uint32_t* ICER1 = (uint32_t*)0xE000E184;
//            *ICER1 = (1 << (37 - 32));
//
//            received_fw_size = received;
//            receive_new_fw = 1;
//        }
//    }
//}
//
//__attribute__((section(".Function_in_Ram"))) void flash_erase_sector(int sec_num)
//{
//    if (((FLASH_CR >> 31) & 1) == 1)
//    {
//        FLASH_KEYR = 0x45670123;
//        FLASH_KEYR = 0xCDEF89AB;
//    }
//
//    if (sec_num > 7) return;
//
//    while (((FLASH_SR >> 16) & 1) == 1); // Chờ BSY = 0
//
//    FLASH_SR = 0xF3; // QUAN TRỌNG: Xóa sạch các cờ lỗi cũ
//
//    FLASH_CR &= ~(0xF << 3);
//    FLASH_CR |= (sec_num << 3);
//    FLASH_CR |= (1 << 1);              // SER = 1
//    FLASH_CR |= (1 << 16);             // STRT = 1
//
//    while (((FLASH_SR >> 16) & 1) == 1); // Chờ xóa xong
//
//    FLASH_CR &= ~(1 << 1);
//    FLASH_CR &= ~(0xF << 3);
//}
//
//__attribute__((section(".Function_in_Ram"))) void flash_program(uint8_t* addr, uint8_t val)
//{
//    if (((FLASH_CR >> 31) & 1) == 1)
//    {
//        FLASH_KEYR = 0x45670123;
//        FLASH_KEYR = 0xCDEF89AB;
//    }
//
//    while (((FLASH_SR >> 16) & 1) == 1);
//
//    FLASH_SR = 0xF3;
//    FLASH_CR &= ~(0x3 << 8); // PSIZE = 00 (Program 8-bit byte)
//    FLASH_CR |= (1 << 0);    // PG = 1
//    *addr = val;
//    while (((FLASH_SR >> 16) & 1) == 1);
//    FLASH_CR &= ~(1 << 0);
//}
//
//
//__attribute__((section(".Function_in_Ram"))) void update(void)
//{
//    __asm("CPSID i");
//
//    if (receive_new_fw == 1 && received_fw_size > 0)
//    {
//        // 1. Xóa Sector 0 (16KB: 0x08000000 - 0x08003FFF)
//        flash_erase_sector(0);
//
//        // 2. Nếu firmware > 16KB, xóa tiếp Sector 1
//        if (received_fw_size > 16384)
//        {
//            flash_erase_sector(1);
//        }
//
//        // 3. Ghi dữ liệu Firmware mới vào Flash từ 0x08000000
//        for (uint32_t i = 0; i < received_fw_size; i++)
//        {
//            flash_program((uint8_t*)(0x08000000 + i), rx_buf[i]);
//        }
//
//        // 4. Khóa lại Flash
//        FLASH_CR |= (1 << 31);
//
//        // 5. System Reset để MCU khởi động chạy Firmware mới
//        uint32_t* AIRCR = (uint32_t*)(0xE000ED0C);
//        *AIRCR = (0x5FA << 16) | (1 << 2);
//
//        while (1);
//    }
//}
//
//int main(void)
//{
//    USART1_Config();
//    DMA_Init();
//
//    my_printf("Bootloader started!\r\n");
//    my_printf("waiting for new firmware...\r\n");
//
//    while (1)
//    {
//        if (receive_new_fw == 1)
//        {
//            my_printf("Received %d bytes. Updating firmware...\r\n", received_fw_size);
//
//            // Chờ truyền xong chuỗi thông báo qua UART trước khi xóa Flash
//            while (((USART1_SR >> 6) & 1) == 0); // Chờ TC = 1
//
//            update();
//        }
//    }
//
//    return 0;
//}
//


// khong gui uart===============================================================================
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
#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))
#define GPIOB_MODER                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OSPEEDR               (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_AFRL                  (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x20))
#define USART1_SR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x00))
#define USART1_DR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x04))
#define USART1_BRR                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x08))
#define USART1_CR1                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x0C))
#define USART1_CR3                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x14))
#define DMA2_S2PAR                  (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x18 + 0x18 * 2))
#define DMA2_S2M0AR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x1C + 0x18 * 2))
#define DMA2_S2NDTR                 (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x14 + 0x18 * 2))
#define DMA2_S2CR                   (*(volatile uint32_t*)(DMA2_BASE_ADDR + 0x10 + 0x18 * 2))
#define FLASH_CR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10))
#define FLASH_SR                    (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C))
#define FLASH_KEYR                  (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x04))
// Đệm nhận 32 KB
#define RX_BUFFER_SIZE  (32 * 1024)
uint8_t rx_buf[RX_BUFFER_SIZE];
volatile uint32_t received_fw_size = 0;
volatile uint8_t receive_new_fw = 0;
void usart1_send(char data)
{
    while (((USART1_SR >> 7) & 1) == 0); // Chờ TXE = 1
    USART1_DR = (data & 0xFF);
}

void my_printf(char* str, ...)
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
void USART1_Config(void)
{
    // Bật clock GPIOB và USART1
    RCC_AHB1ENR |= (1 << 1);
    RCC_APB2ENR |= (1 << 4);
    // Cấu hình PB6 (TX), PB7 (RX) AF7
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |=  ((0x2 << 12) | (0x2 << 14));
    GPIOB_AFRL  &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL  |=  ((0x7 << 24) | (0x7 << 28));
    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR |=  ((0x1 << 12) | (0x1 << 14));
    // Baudrate 115200 @ 16MHz HSI: USARTDIV = 16000000 / (16 * 115200) = 8.6875 -> Mantissa = 8, Fraction = 11 (0x8B)
    // Nếu dùng 9600 baud @ 16MHz: 0x683
    USART1_BRR = 0x8B;
    // Bật USART, TE, RE, IDLEIE
    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2) | (1 << 4);
    USART1_CR3 = (1 << 6); // DMAR
    // Bật NVIC IRQ 37 (USART1)
    uint32_t* ISER1 = (uint32_t*)0xE000E104;
    *ISER1 |= (1 << (37 - 32));
}
void DMA_Init(void)
{
    RCC_AHB1ENR |= (1 << 22);
    DMA2_S2CR &= ~(1 << 0);
    while ((DMA2_S2CR & (1 << 0)) != 0);
    uint32_t* DMA_LIFCR = (uint32_t*)(DMA2_BASE_ADDR + 0x08);
    *DMA_LIFCR |= (0x3D << 16);
    DMA2_S2PAR  = (uint32_t)(USART1_BASE_ADDR + 0x04);
    DMA2_S2M0AR = (uint32_t)rx_buf;
    DMA2_S2NDTR = sizeof(rx_buf);
    DMA2_S2CR = (4 << 25) | (1 << 10) | (1 << 0); // Channel 4, MINC, EN
}

void USART1_IRQHandler(void)
{
    if ((USART1_SR & (1 << 4)) != 0) // IDLE flag
    {
        volatile uint32_t temp = USART1_SR;
        temp = USART1_DR;
        (void)temp;
        uint32_t remaining = DMA2_S2NDTR;
        uint32_t received = sizeof(rx_buf) - remaining;
        // Chỉ chấp nhận khi nhận được tối thiểu Vector Table (ít nhất 512 bytes)
        if (received >= 512)
        {
            DMA2_S2CR &= ~(1 << 0);
            USART1_CR1 &= ~(1 << 4);
            USART1_CR3 &= ~(1 << 6);
            uint32_t* ICER1 = (uint32_t*)0xE000E184;
            *ICER1 = (1 << (37 - 32));
            received_fw_size = received;
            receive_new_fw = 1;
        }
    }
}
__attribute__((section(".Function_in_Ram"))) void flash_unlock(void)
{
    if (((FLASH_CR >> 31) & 1) == 1) {
        FLASH_KEYR = 0x45670123;
        FLASH_KEYR = 0xCDEF89AB;
    }
}
__attribute__((section(".Function_in_Ram"))) void flash_erase_sector(int sec_num)
{
    flash_unlock();
    if (sec_num > 7) return;
    while (((FLASH_SR >> 16) & 1) == 1);
    FLASH_SR = 0xF3;
    FLASH_CR &= ~(0x3 << 8); // PSIZE = 00 (x8)
    FLASH_CR &= ~(0xF << 3);
    FLASH_CR |= (sec_num << 3);
    FLASH_CR |= (1 << 1);  // SER
    FLASH_CR |= (1 << 16); // STRT
    while (((FLASH_SR >> 16) & 1) == 1);
    FLASH_CR &= ~(1 << 1);
    FLASH_CR &= ~(0xF << 3);
}
__attribute__((section(".Function_in_Ram"))) void flash_program_byte(uint32_t addr, uint8_t val)
{
    flash_unlock();
    while (((FLASH_SR >> 16) & 1) == 1);
    FLASH_SR = 0xF3;
    FLASH_CR &= ~(0x3 << 8); // PSIZE = 00 (8-bit)
    FLASH_CR |= (1 << 0);    // PG = 1
    *(volatile uint8_t*)addr = val;
    while (((FLASH_SR >> 16) & 1) == 1);
    FLASH_CR &= ~(1 << 0);
}
__attribute__((section(".Function_in_Ram"))) void update(void)
{
    __asm volatile ("cpsid i");
    if (receive_new_fw == 1 && received_fw_size > 0)
    {
        // 1. Xóa Sector 0 (16KB)
        flash_erase_sector(0);
        // Nếu firmware lớn hơn 16KB, xóa thêm Sector 1
        if (received_fw_size > 16384) {
            flash_erase_sector(1);
        }
        // 2. Ghi từng byte vào Flash
        for (uint32_t i = 0; i < received_fw_size; i++) {
            flash_program_byte(0x08000000 + i, rx_buf[i]);
        }
        // 3. Khóa Flash
        FLASH_CR |= (1 << 31);
        // 4. System Reset khởi động lại
        uint32_t* AIRCR = (uint32_t*)(0xE000ED0C);
        *AIRCR = (0x5FA << 16) | (1 << 2);
        while (1);
    }
}
int main(void)
{
    USART1_Config();
    DMA_Init();
    my_printf("Bootloader started! Baudrate: 115200\r\n");
    my_printf("Waiting for Firmware_01.bin...\r\n");
    while (1)
    {
        if (receive_new_fw == 1)
        {
            my_printf("Received %d bytes. Flashing...\r\n", received_fw_size);
            while (((USART1_SR >> 6) & 1) == 0); // Chờ UART TX hoàn tất
            update();
        }
    }
    return 0;
}

// =======================end ==========================================

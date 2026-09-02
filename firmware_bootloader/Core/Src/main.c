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
#define ram_in_func                 __attribute__((section(".Function_in_Ram"))) // định nghĩa dán nhãn để hàm chuyển lên phân vùng RAM 


// Đệm nhận 32 KB
#define RX_BUFFER_SIZE  (32 * 1024)
uint8_t rx_buf[RX_BUFFER_SIZE];
volatile uint32_t received_fw_size = 0;
volatile uint8_t receive_new_fw = 0;

void usart1_send(char data);
void my_printf(char* str, ...);
void USART1_Config(void);
void DMA_Init(void);
void USART1_IRQHandler(void);
ram_in_func void flash_unlock(void);
ram_in_func void flash_erase_sector(int sec_num);
ram_in_func void flash_program_byte(uint32_t addr, uint8_t val);
ram_in_func void update(void);

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

ram_in_func void flash_program_byte(uint32_t addr, uint8_t val)
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

ram_in_func void update(void)
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
            // Delay ~300ms để Minicom kịp đóng popup truyền file và vẽ lại màn hình
            for (volatile uint32_t i = 0; i < 1000000; i++);

            my_printf("\r\nReceived %d bytes. Flashing into Flash...\r\n", received_fw_size);
            while (((USART1_SR >> 6) & 1) == 0); // Chờ UART TX hoàn tất (TC = 1)
            
            update();
        }
    }
    return 0;
}

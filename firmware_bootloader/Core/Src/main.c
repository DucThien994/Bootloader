#include "main.h"
#include <stdio.h>
#include <stdint.h>

#define FLASH_INTERFACE_BASE_ADDR   0x40023C00
#define USART1_BASE_ADDR            0x40011000
#define DMA2_BASE_ADDR              0x40026400

#define RCC_BASE_ADDR               0x40023800
#define GPIOA_BASE_ADDR             0x40020000
#define USART1_BASE_ADDR            0x40011000

#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))

#define GPIOA_MODER                 (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x00))
#define GPIOA_AFRH                  (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x24))

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

int rx_index = 0;
char rx_buf[8192];

// USART1 CONFIG 
void USART1_Config(){
    // enable clock pa9 pa10 for usart1 
    RCC_AHB1ENR |= (1 << 0);
    RCC_APB2ENR |= (1 << 4);

    // alternate function 
    GPIOA_MODER &= ~(0x3 << 18);  // reset pa9 moder
    GPIOA_MODER |= (0x2 << 18);   // set pa9 moder to alternate function 
    
    GPIOA_MODER &= ~(0x3 << 20);  // reset pa10 moder
    GPIOA_MODER |= (0x2 << 20);   // set pa10 moder to alternate function 

    GPIOA_AFRH &= ~(0xF << 4);   // reset pa9 alternate function
    GPIOA_AFRH |= (0x7 << 4);    // set pa9 alternate function to usart1

    GPIOA_AFRH &= ~(0xF << 8);   // reset pa10 alternate function
    GPIOA_AFRH |= (0x7 << 8);    // set pa10 alternate function to usart1

    // set baud rate 9600
    USART1_BRR = 0x683;

    // enable usart1 
    USART1_CR1 |= (1 << 13);    
    USART1_CR1 |= (1 << 3);     
    USART1_CR1 |= (1 << 2);     
    USART1_CR3 |= (1 << 6);   

}

// ===================update firmware ==============================================
//#if 0

__attribute__((section(".Function_in_RAM"))) void flash_erase_sector(int sec_num)
{

    // Nếu Flash đang bị khóa thì mở khóa
    if (((FLASH_CR) >> 31) == 1)
    {
        // Unlock sequence
        FLASH_KEYR = 0x45670123;
        FLASH_KEYR = 0xCDEF89AB;
    }

    // Nếu sector vượt quá 7 thì hông hợp lệ (trên STM32F411 chỉ có 8 sector)
    if (sec_num > 7)
        return;

    // 1. Chờ không còn thao tác Flash nào đang diễn ra
    while (((FLASH_SR >> 16) & 1) == 1);

    // 2. Thiết lập bit SER (Sector Erase) và chọn sector cần xóa
    FLASH_CR |= (1 << 1);              // SER = 1
    FLASH_CR &= ~(0xF << 3); // reset before set
    FLASH_CR |= (sec_num << 3);        // SNB[3:0] = sector number

    // 3. Thiết lập bit STRT để bắt đầu xóa sector
    FLASH_CR |= (1 << 16);             // STRT = 1

    // 4. Chờ cho đến khi bit BSY = 0 (hoàn thành)
    while (((FLASH_SR >> 16) & 1) == 1);
}

//===============flash program =================
__attribute__((section(".Function_in_RAM"))) void flash_program(uint8_t* addr, uint8_t val)
{
    // Nếu Flash đang bị khóa thì mở khóa
    if (((FLASH_CR) >> 31) == 1)
    {
        // Unlock Flash bằng chuỗi khóa
        FLASH_KEYR = 0x45670123;   // KEY1
        FLASH_KEYR = 0xCDEF89AB;   // KEY2
    }
    
    // 1. Đợi cho đến khi không còn hoạt động ghi nào đang diễn ra (BSY = 0)
    while (((FLASH_SR >> 16) & 1) == 1);

    // 2. Set bit PG (program) trong thanh ghi FLASH_CR
    FLASH_CR |= (1 << 0);

    // 3. Ghi dữ liệu vào địa chỉ flash
    *addr = val;

    // 4. Chờ đến khi việc ghi hoàn tất (BSY = 0)
    while (((FLASH_SR >> 16) & 1) == 1);
    FLASH_CR &= ~(1 << 0);
}

void usart1_send(char data)
{
    // Chờ cho đến khi việc truyền hoàn tất (bit 6 TXE trong SR = 1)
    while (((USART1_SR >> 6) & 1) == 0);
    USART1_DR = data;  // Gửi dữ liệu
}

void DMA_Init()
{
    /*
        chọn DMA2 stream 2 channel 4 cho UART1 Rx
        - set địa chỉ người gửi
        - set địa chỉ người nhận
        - set số lượng data
    */

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    DMA2_S2PAR  = 0x40011004;              // Địa chỉ thanh ghi UART1->DR (Data Register)
    DMA2_S2M0AR = (uint32_t)rx_buf;        // Bộ đệm nhận dữ liệu
    DMA2_S2NDTR = sizeof(rx_buf);          // Số lượng byte cần nhận
    DMA2_S2CR |= (4 << 25);                // Channel 4
    DMA2_S2CR |= (1 << 8);                 // Circular mode
    DMA2_S2CR |= (1 << 10);                // Memory increment mode

    /** dma send an interrupt signal*/
    DMA2_S2CR  |= 1 << 4;
    uint32_t* ISER1 = (uint32_t*)0xE000E104;
    *ISER1 |= 1 << (58 - 32);
    DMA2_S2CR |= 1 << 0;

}

char receive_new_fw = 0;
void DMA2_Stream2_IRQHandler(){
    __asm("NOP");
	uint32_t* DMA_LIFCR = (uint32_t*)(DMA2_BASE_ADDR + 0x08);
	*DMA_LIFCR |= 1 << 21;
	receive_new_fw = 1;
}

#include <string.h>
#include <stdarg.h>

void my_printf(char* str, ...)
{
    va_list list;
    va_start(list, str);

    char print_buf[128] = {0};                // Bộ đệm lưu chuỗi sau khi format
    vsprintf(print_buf, str, list);           // Format chuỗi với tham số biến

    int len = strlen(print_buf);
    for (int i = 0; i < len; i++)
    {
        usart1_send(print_buf[i]);              // Gửi từng ký tự qua UART
    }

    va_end(list);
}

__attribute__((section(".Function_in_RAM"))) void update()
{
	// disable interrupt
	__asm("CPSID i");
	if (receive_new_fw == 1){
	    flash_erase_sector(0);
	    for (int i = 0; i < sizeof(rx_buf); i++)
	    {
	    	flash_program((uint8_t*)(0x08000000 + i), rx_buf[i]); // sector 0
	    }
        // APPLICATION INTERRUPT CONTROL ADDRESS IN ARM M4
	    uint32_t* AIRCR = (uint32_t*)(0xE000ED0C);
	    // WRITE KEY 5FA [VECTOR KEY START]
        // system request reset bit 2
        *AIRCR = (0x5FA << 16) | (1 << 2);
    }

    FLASH_CR |= (1 << 31); // lock after update
}

//#endif
// ======================== ket thuc update firmware =====================

int main (){

    DMA_Init();
    USART1_Config();
    my_printf("Bootloader started!\r\n");
    my_printf("waiting for new firmware\r\n");

	while (1){
        if (receive_new_fw == 1){
            my_printf("update new firmware\r\n");
            update();
        }
	}

	return 0;
}

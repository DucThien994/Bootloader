#include "main.h"
#include <stdio.h>
#include <stdint.h>

#define FLASH_INTERFACE_BASE_ADDR  0x40023C00

// ===================update firmware ==============================================
//#if 0

__attribute__((section(".Function_in_RAM"))) void flash_erase_sector(int sec_num)
{
    uint32_t* FLASH_SR = (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C));
    uint32_t* FLASH_CR = (*(volatile uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10));

    // Nếu Flash đang bị khóa thì mở khóa
    if (((*FLASH_CR) >> 31) == 1)
    {
        // Unlock sequence
        uint32_t* FLASH_KEYR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x04);
        *FLASH_KEYR = 0x45670123;
        *FLASH_KEYR = 0xCDEF89AB;
    }

    // Nếu sector vượt quá 7 thì không hợp lệ (trên STM32F411 chỉ có 8 sector)
    if (sec_num > 7)
        return;

    // 1. Chờ không còn thao tác Flash nào đang diễn ra
    while (((*FLASH_SR >> 16) & 1) == 1);

    // 2. Thiết lập bit SER (Sector Erase) và chọn sector cần xóa
    *FLASH_CR |= (1 << 1);              // SER = 1
    *FLASH_CR |= (sec_num << 3);        // SNB[3:0] = sector number

    // 3. Thiết lập bit STRT để bắt đầu xóa sector
    *FLASH_CR |= (1 << 16);             // STRT = 1

    // 4. Chờ cho đến khi bit BSY = 0 (hoàn thành)
    while (((*FLASH_SR >> 16) & 1) == 1);
}

//===============flash program =================
__attribute__((section(".Function_in_RAM"))) void flash_program(uint8_t* addr, uint8_t val)
{
    uint32_t* FLASH_SR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C);
    uint32_t* FLASH_CR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10);

    // Nếu Flash đang bị khóa thì mở khóa
    if (((*FLASH_CR) >> 31) == 1)
    {
        // Unlock Flash bằng chuỗi khóa
        uint32_t* FLASH_KEYR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x04);
        *FLASH_KEYR = 0x45670123;   // KEY1
        *FLASH_KEYR = 0xCDEF89AB;   // KEY2
    }

    // 1. Đợi cho đến khi không còn hoạt động ghi nào đang diễn ra (BSY = 0)
    while (((*FLASH_SR >> 16) & 1) == 1);

    // 2. Set bit PG (program) trong thanh ghi FLASH_CR
    *FLASH_CR |= (1 << 0);

    // 3. Ghi dữ liệu vào địa chỉ flash
    *addr = val;

    // 4. Chờ đến khi việc ghi hoàn tất (BSY = 0)
    while (((*FLASH_SR >> 16) & 1) == 1);
}

#define UART1_BASE_ADDR 0x40011000
void uart_send(char data)
{
    uint32_t* UART_DR = (uint32_t*)(UART1_BASE_ADDR + 0x04);  // Thanh ghi dữ liệu
    *UART_DR = data;  // Gửi dữ liệu

    // Chờ cho đến khi việc truyền hoàn tất (bit 6 TXE trong SR = 1)
    uint32_t* UART_SR = (uint32_t*)(UART1_BASE_ADDR + 0x00);  // Thanh ghi trạng thái
    while (((*UART_SR >> 6) & 1) == 0);
}

char rx_buf[8192];
int rx_index = 0;
#define DMA2_BASE_ADDR  0x40026400

void DMA_Init()
{
    /*
        chọn DMA2 stream 2 channel 4 cho UART1 Rx
        - set địa chỉ người gửi
        - set địa chỉ người nhận
        - set số lượng data
    */

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    uint32_t* DMA_S2PAR  = (uint32_t*)(DMA2_BASE_ADDR + 0x18 + 0x18 * 2);
    uint32_t* DMA_S2M0AR = (uint32_t*)(DMA2_BASE_ADDR + 0x1C + 0x18 * 2);
    uint32_t* DMA_S2NDTR = (uint32_t*)(DMA2_BASE_ADDR + 0x14 + 0x18 * 2);
    uint32_t* DMA_S2CR   = (uint32_t*)(DMA2_BASE_ADDR + 0x10 + 0x18 * 2);

    *DMA_S2PAR  = 0x40011004;              // Địa chỉ thanh ghi UART1->DR (Data Register)
    *DMA_S2M0AR = (uint32_t)rx_buf;        // Bộ đệm nhận dữ liệu
    *DMA_S2NDTR = sizeof(rx_buf);          // Số lượng byte cần nhận

    *DMA_S2CR |= (4 << 25);                // Channel 4
    *DMA_S2CR |= (1 << 8);                 // Circular mode
    *DMA_S2CR |= (1 << 10);                // Memory increment mode

    /** dma send an interrupt signal*/
    *DMA_S2CR  |= 1 << 4;
    uint32_t* ISER1 = (uint32_t*)0xE000E104;
    *ISER1 |= 1 << (58 - 32);
    *DMA_S2CR |= 1 << 0;

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
        uart_send(print_buf[i]);              // Gửi từng ký tự qua UART
    }

    va_end(list);
}

__attribute__((section(".Function_in_RAM"))) void update()
{
	// disable interrupt
	__asm("CPSID i");
	if (receive_new_fw == 1)
	    	{
	    		flash_erase_sector(0);
	    		for (int i = 0; i < sizeof(rx_buf); i++)
	    		{
	    			flash_program(0x08000000 + i, rx_buf[i]);
	    		}
	    		uint32_t* AIRCR = (uint32_t*)(0xE000ED0C);
	    		*AIRCR = (0x5FA << 16) | (1 << 2);

	    	}
}

//#endif
// ======================== ket thuc update firmware =====================

int main (){

	while (1){

	}

	return 0;
}

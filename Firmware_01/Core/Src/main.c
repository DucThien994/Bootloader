#include <stdio.h>
#include "main.h"
#include "stm32f4xx.h"

#define GPIOD_BASE_ADDR			0x40020C00
#define RCC_BASE_ADDR	 		0x40023800
#define GPIOD_ODR 				(*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x14))
#define GPIOD_MODER 			(*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x00))
#define RCC_AHB1ENR 			(*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))

void GPIO_Init(void){
	RCC_AHB1ENR |= 1 << 3;
	GPIOD_MODER &= ~(0XFF << 24);
	GPIOD_MODER |= (0x55 << 24);
}

int main (){

	HAL_Init();
	GPIO_Init();
	while(1){
		GPIOD_ODR &= ~(0b1111 << 12);
		HAL_Delay(80);
		GPIOD_ODR |= 0b1111 << 12;
		HAL_Delay(80);

	}

	return 0;
}

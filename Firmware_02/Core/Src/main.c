#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 * ĐỊNH NGHĨA THANH GHI (DIRECT REGISTER DEFINITIONS)
 * ============================================================================
 */
#define RCC_BASE_ADDR               0x40023800
#define GPIOB_BASE_ADDR             0x40020400
#define GPIOD_BASE_ADDR             0x40020C00
#define USART1_BASE_ADDR            0x40011000
#define ADC1_BASE_ADDR              0x40012000
#define SYSTICK_BASE_ADDR           0xE000E010

/* RCC Registers */
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

/* ADC1 Registers & ADC Common Control Register */
#define ADC1_SR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x00))
#define ADC1_CR1                    (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x04))
#define ADC1_CR2                    (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x08))
#define ADC1_SMPR1                  (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x0C))
#define ADC1_SQR1                   (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x2C))
#define ADC1_SQR3                   (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x34))
#define ADC1_DR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x4C))
#define ADC_CCR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x304))

/* SysTick Registers */
#define SYST_CSR                    (*(volatile uint32_t*)(SYSTICK_BASE_ADDR + 0x00))
#define SYST_RVR                    (*(volatile uint32_t*)(SYSTICK_BASE_ADDR + 0x04))
#define SYST_CVR                    (*(volatile uint32_t*)(SYSTICK_BASE_ADDR + 0x08))

/* ============================================================================
 * KHỞI TẠO VÀ ĐỊNH THỜI SYSTICK (16MHz HSI)
 * ============================================================================
 */
void SysTick_Init(void)
{
    // STM32F411 chạy HSI = 16MHz -> 1ms = 16000 clock cycles
    SYST_RVR = 16000 - 1;
    SYST_CVR = 0;
    // Bật SysTick: CLKSOURCE = Processor clock (16MHz), ENABLE = 1
    SYST_CSR = (1 << 2) | (1 << 0);
}

void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        SYST_CVR = 0;
        // Chờ cờ COUNTFLAG (bit 16) bật lên khi đếm hết 1ms
        while ((SYST_CSR & (1 << 16)) == 0);
    }
}

/* ============================================================================
 * CẤU HÌNH USART1 (PB6 - TX, PB7 - RX) 115200 BAUD
 * ============================================================================
 */
void USART1_Config(void)
{
    // 1. Cấp clock cho GPIOB (Bit 1 AHB1ENR) và USART1 (Bit 4 APB2ENR)
    RCC_AHB1ENR |= (1 << 1);
    RCC_APB2ENR |= (1 << 4);

    // 2. Cấu hình PB6 (TX), PB7 (RX) sang Alternate Function AF7
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |=  ((0x2 << 12) | (0x2 << 14));

    GPIOB_AFRL  &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL  |=  ((0x7 << 24) | (0x7 << 28));

    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR |=  ((0x1 << 12) | (0x1 << 14));

    // 3. Cấu hình Baudrate 115200 @ 16MHz HSI: Mantissa = 8, Fraction = 11 (0x8B)
    USART1_BRR = 0x8B;

    // 4. Bật USART1, Transmitter Enable (TE), Receiver Enable (RE)
    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2);
}

void usart1_send(char data)
{
    while (((USART1_SR >> 7) & 1) == 0); // Chờ TXE = 1 (Transmit Data Register Empty)
    USART1_DR = (data & 0xFF);
}

void my_printf(const char *str, ...)
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
 * CẤU HÌNH ADC1 ĐỌC CẢM BIẾN NHIỆT ĐỘ NỘI (INTERNAL TEMPERATURE SENSOR)
 * ============================================================================
 * - Kênh cảm biến nhiệt độ nội STM32F411: ADC1 Channel 18 (IN18).
 * - Bật TSVREFE (bit 23 trong ADC_CCR).
 * - Thời gian lấy mẫu: SMP18 = 480 cycles (vượt ngưỡng >= 10us).
 * ============================================================================
 */
void ADC1_Temperature_Init(void)
{
    // 1. Cấp clock cho ADC1 (Bit 8 APB2ENR)
    RCC_APB2ENR |= (1 << 8);

    // 2. Bật kênh Temperature Sensor & VREFINT (Bit 23 trong ADC_CCR: TSVREFE = 1)
    ADC_CCR |= (1 << 23);

    // 3. Cấu hình Prescaler ADC = PCLK2 / 4 (ADCPRE = 01b, bits 17:16 trong ADC_CCR)
    ADC_CCR &= ~(0x3 << 16);
    ADC_CCR |=  (0x1 << 16);

    // 4. Cấu hình thời gian lấy mẫu cho Channel 18: SMP18 = 111b (480 cycles)
    ADC1_SMPR1 |= (7 << 24);

    // 5. Cấu hình độ dài chuỗi chuyển đổi: L[3:0] = 0000 (1 conversion)
    ADC1_SQR1 &= ~(0xF << 20);

    // 6. Gán kênh chuyển đổi thứ 1 là Channel 18 (SQ1 = 18)
    ADC1_SQR3 &= ~(0x1F << 0);
    ADC1_SQR3 |= (18 << 0);

    // 7. Bật nguồn ADC1 (Bit 0 trong ADC_CR2: ADON = 1)
    ADC1_CR2 |= (1 << 0);

    // Chờ cảm biến nhiệt độ và ADC ổn định nguồn (tSTART >= 10us)
    delay_ms(15);
}

/*
 * Hàm đọc ADC có lọc trung bình (Average Filter) 16 lần
 * để loại bỏ nhiễu và đảm bảo cập nhật giá trị mới nhất.
 */
uint16_t ADC1_Read_Temperature_Average(void)
{
    uint32_t adc_sum = 0;
    const int SAMPLES = 16;

    for (int i = 0; i < SAMPLES; i++)
    {
        // Xóa cờ EOC (Bit 1 trong ADC_SR) trước khi kích hoạt
        ADC1_SR &= ~(1 << 1);

        // Kích hoạt chuyển đổi bằng phần mềm: SWSTART = 1 (Bit 30 trong ADC_CR2)
        ADC1_CR2 |= (1 << 30);

        // Chờ cờ EOC = 1 báo chuyển đổi hoàn tất
        while (((ADC1_SR >> 1) & 1) == 0);

        // Đọc giá trị 12-bit từ ADC1_DR
        adc_sum += (ADC1_DR & 0x0FFF);

        // Delay 1ms giữa các lần lấy mẫu
        delay_ms(1);
    }

    return (uint16_t)(adc_sum / SAMPLES);
}

/*
 * Tính toán nhiệt độ (°C) chuẩn theo Datasheet STM32F411 (RM0383 Section 11.10):
 * 
 * 1. Vsense (mV) = (ADC_VAL * Vdda_mV) / 4095.0
 * 2. Temp (°C)   = ((Vsense - V25) / Avg_Slope) + 25.0
 * 
 * Thông số chuẩn nhà sản xuất STMicroelectronics:
 * - Vdda = 3.3V = 3300 mV
 * - V25 = 760 mV (Điện áp cảm biến tại 25 °C)
 * - Avg_Slope = 2.5 mV/°C (Độ dốc trung bình)
 */
float Calculate_Temperature(uint16_t raw_adc, float *vsense_out)
{
    // Tính điện áp cảm biến (mV)
    float vsense_mv = ((float)raw_adc * 3300.0f) / 4095.0f;
    
    if (vsense_out != NULL) {
        *vsense_out = vsense_mv;
    }

    // Tính nhiệt độ ra °C
    float temperature = ((vsense_mv - 760.0f) / 2.5f) + 25.0f;

    return temperature;
}

/* ============================================================================
 * CẤU HÌNH LED BÁO TRẠNG THÁI (PD12)
 * ============================================================================
 */
void LED_Init(void)
{
    // Cấp clock GPIOD (Bit 3 AHB1ENR)
    RCC_AHB1ENR |= (1 << 3);

    // Cấu hình PD12 là Output (01b)
    GPIOD_MODER &= ~(0x3 << 24);
    GPIOD_MODER |=  (0x1 << 24);
}

void LED_Toggle(void)
{
    GPIOD_ODR ^= (1 << 12);
}

/* ============================================================================
 * HÀM MAIN CHÍNH
 * ============================================================================
 */
int main(void)
{
    // Khởi tạo các ngoại vi bằng thanh ghi
    SysTick_Init();
    USART1_Config();
    ADC1_Temperature_Init();
    LED_Init();

    my_printf("\r\n========================================================\r\n");
    my_printf(" STM32F411 Firmware 02 - Internal Temperature Monitor\r\n");
    my_printf(" Register-Level Driver | Baudrate: 115200 bps\r\n");
    my_printf(" V25 = 760mV | Slope = 2.5mV/*C | Sampling Rate: 1s\r\n");
    my_printf("========================================================\r\n\r\n");

    uint32_t sample_index = 0;

    while (1)
    {
        sample_index++;

        // 1. Đọc giá trị ADC cảm biến nhiệt độ nội (lọc trung bình 16 mẫu)
        uint16_t adc_val = ADC1_Read_Temperature_Average();

        // 2. Tính toán điện áp Vsense và nhiệt độ °C
        float vsense_mv = 0.0f;
        float temp_c = Calculate_Temperature(adc_val, &vsense_mv);

        // Tách phần nguyên và phần thập phân để in ra chuỗi
        int temp_int = (int)temp_c;
        int temp_dec = (int)((temp_c - (float)temp_int) * 100.0f);
        if (temp_dec < 0) temp_dec = -temp_dec;

        int vsense_int = (int)vsense_mv;
        int vsense_dec = (int)((vsense_mv - (float)vsense_int) * 10.0f);
        if (vsense_dec < 0) vsense_dec = -vsense_dec;

        // 3. Đảo trạng thái LED PD12 mỗi lần gửi
        LED_Toggle();

        // 4. In thông tin chi tiết: [Mẫu] Nhiệt độ | Điện áp Vsense | Giá trị ADC Raw
        my_printf("[%lu] Temp: %d.%02d *C | Vsense: %d.%d mV | ADC: %u\r\n", 
                  sample_index, temp_int, temp_dec, vsense_int, vsense_dec, adc_val);

        // 5. Chu kỳ gửi 1 giây (1000 ms)
        delay_ms(1000);
    }

    return 0;
}
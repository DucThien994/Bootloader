#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 * ĐỊNH NGHĨA ĐỊA CHỈ CƠ SỞ CỦA CÁC NGOẠI VI (PERIPHERAL BASE ADDRESSES)
 * ============================================================================
 */
#define RCC_BASE_ADDR               0x40023800UL
#define GPIOB_BASE_ADDR             0x40020400UL
#define GPIOD_BASE_ADDR             0x40020C00UL
#define USART1_BASE_ADDR            0x40011000UL
#define ADC1_BASE_ADDR              0x40012000UL
#define TIM2_BASE_ADDR              0x40000000UL

/* ============================================================================
 * ĐỊNH NGHĨA CÁC THANH GHI BẰNG CON TRỎ TRỰC TIẾP (DIRECT REGISTER DEFINITIONS)
 * ============================================================================
 */

/* --- RCC Registers --- */
#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
#define RCC_APB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x40))
#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))

/* --- GPIOB Registers (PB6 TX, PB7 RX - USART1) --- */
#define GPIOB_MODER                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OSPEEDR               (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_AFRL                  (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x20))

/* --- GPIOD Registers (LED PD12..PD15) --- */
#define GPIOD_MODER                 (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x00))
#define GPIOD_ODR                   (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x14))

/* --- USART1 Registers --- */
#define USART1_SR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x00))
#define USART1_DR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x04))
#define USART1_BRR                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x08))
#define USART1_CR1                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x0C))
#define USART1_CR2                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x10))
#define USART1_CR3                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x14))

/* --- ADC1 Registers & ADC Common Control Register --- */
#define ADC1_SR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x00))
#define ADC1_CR1                    (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x04))
#define ADC1_CR2                    (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x08))
#define ADC1_SMPR1                  (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x0C))
#define ADC1_SQR1                   (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x2C))
#define ADC1_SQR3                   (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x34))
#define ADC1_DR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x4C))
#define ADC_CCR                     (*(volatile uint32_t*)(ADC1_BASE_ADDR + 0x304))

/* --- TIM2 Registers (Timer 0.5s) --- */
#define TIM2_CR1                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x00))
#define TIM2_DIER                   (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x0C))
#define TIM2_SR                     (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x10))
#define TIM2_EGR                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x14))
#define TIM2_CNT                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x24))
#define TIM2_PSC                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x28))
#define TIM2_ARR                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x2C))

/* --- Cortex-M4 Core Registers --- */
#define NVIC_ISER0                  (*(volatile uint32_t*)0xE000E100UL)
#define SCB_VTOR                    (*(volatile uint32_t*)0xE000ED08UL)

/* ============================================================================
 * BIẾN TOÀN CỤC & NGUYÊN MẪU HÀM
 * ============================================================================
 */
volatile uint8_t tim2_temp_flag = 0;

void delay_ms(uint32_t ms);
void LED_Init(void);
void USART1_Config(void);
void usart1_send(char data);
void my_printf(const char *str, ...);
void ADC1_Init(void);
float Read_temperature(void);
void TIM2_Config(void);
void TIM2_IRQHandler(void);

/* ============================================================================
 * HÀM DELAY ĐƠN GIẢN DÙNG VÒNG LẶP NOP
 * ============================================================================
 */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        for (volatile uint32_t j = 0; j < 3200; j++)
        {
            __asm volatile ("nop");
        }
    }
}

/* ============================================================================
 * CẤU HÌNH LED BÁO TRẠNG THÁI TRÊN BOARD DISCOVERY (PD12..PD15)
 * ============================================================================
 */
void LED_Init(void)
{
    // 1. Cấp clock cho GPIOD (Bit 3 trong AHB1ENR)
    RCC_AHB1ENR |= (1 << 3);

    // 2. Cấu hình PD12..PD15 là General Purpose Output (01b)
    GPIOD_MODER &= ~(0xFF << 24);
    GPIOD_MODER |=  (0x55 << 24);

    // 3. Mặc định tắt toàn bộ 4 LED
    GPIOD_ODR &= ~(0xF << 12);
}

/* ============================================================================
 * CẤU HÌNH USART1 (PB6 - TX, PB7 - RX) 115200 BAUD @ 16MHz HSI
 * ============================================================================
 */
void USART1_Config(void)
{
    // 1. Cấp clock cho GPIOB (Bit 1 AHB1ENR) và USART1 (Bit 4 APB2ENR)
    RCC_AHB1ENR |= (1 << 1);
    RCC_APB2ENR |= (1 << 4);

    // 2. Cấu hình PB6 (TX), PB7 (RX) sang Alternate Function AF7 (02b)
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |=  ((0x2 << 12) | (0x2 << 14));

    // Gán chức năng AF7 (USART1) cho PB6 và PB7 trong AFRL
    GPIOB_AFRL  &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL  |=  ((0x7 << 24) | (0x7 << 28));

    // Cấu hình tốc độ cao và kích hoạt điện trở kéo lên Pull-up
    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR   &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR   |=  ((0x1 << 12) | (0x1 << 14));

    // 3. Cấu hình Baudrate 115200 @ 16MHz HSI:
    // USARTDIV = 16000000 / (16 * 115200) = 8.6875
    // Mantissa = 8, Fraction = 0.6875 * 16 = 11 (0x0B) => 0x8B
    USART1_BRR = 0x8B;

    // 4. Bật USART1, Transmitter Enable (TE bit 3), Receiver Enable (RE bit 2)
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
    for (int i = 0; i < len; i++)
    {
        usart1_send(print_buf[i]);
    }
    va_end(list);
}

/* ============================================================================
 * CẤU HÌNH ADC1 (LẤY CHÍNH XÁC THEO CODE MẪU IMIC_EMBEDDEDC_FINAL)
 * ============================================================================
 * Code mẫu imic_embeddedC_final:
 *   void ADC1_Init(void){
 *       RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
 *       ADC->CCR |= ADC_CCR_TSVREFE;       // Enable temp sensor (Bit 23)
 *       ADC1->SQR3 = 16;                   // Channel 16 = Temp sensor
 *       ADC1->SMPR1 |= (0b111 << 18);      // Sample time 480 cycles (Channel 16)
 *       ADC1->CR2 |= ADC_CR2_ADON;         // Enable ADC1 (Bit 0)
 *   }
 * ============================================================================
 */
void ADC1_Init(void)
{
    // 1. Cấp clock cho ADC1 (Bit 8 APB2ENR)
    RCC_APB2ENR |= (1 << 8);

    // 2. Bật cảm biến nhiệt độ: ADC->CCR |= ADC_CCR_TSVREFE (Bit 23)
    ADC_CCR |= (1 << 23);

    // 3. Chọn kênh 16: ADC1->SQR3 = 16
    ADC1_SQR3 = 16;

    // 4. Cấu hình thời gian lấy mẫu 480 cycles cho kênh 16: ADC1->SMPR1 |= (0b111 << 18)
    ADC1_SMPR1 |= (0b111 << 18);

    // 5. Bật nguồn bộ ADC1: ADC1->CR2 |= ADC_CR2_ADON (Bit 0)
    ADC1_CR2 |= (1 << 0);

    // Chờ cảm biến nhiệt độ và ADC ổn định nguồn
    delay_ms(15);
}

/* ============================================================================
 * HÀM ĐỌC VÀ TÍNH TOÁN NHIỆT ĐỘ (LẤY CHÍNH XÁC THEO CODE MẪU IMIC)
 * ============================================================================
 * Code mẫu imic_embeddedC_final:
 *   float Read_temperature(void){
 *       ADC1->CR2 |= ADC_CR2_SWSTART;
 *       while (!(ADC1->SR & ADC_SR_EOC));
 *       uint16_t data_temp = ADC1->DR;
 *
 *       float V_sense = (float)(data_temp * 3) / 4095;
 *       float temperature = (float)(((V_sense - 0.76) / 0.0025) + 25);
 *       return temperature;
 *   }
 * ============================================================================
 */
float Read_temperature(void)
{
    // Kích hoạt chuyển đổi bằng phần mềm: SWSTART = 1 (Bit 30 trong ADC1_CR2)
    ADC1_CR2 |= (1 << 30);

    // Chờ cờ EOC = 1 báo chuyển đổi hoàn tất (Bit 1 trong ADC1_SR)
    while (((ADC1_SR >> 1) & 1) == 0);

    // Đọc giá trị 12-bit từ ADC1_DR
    uint16_t data_temp = (uint16_t)(ADC1_DR & 0x0FFF);

    // Chính xác phương trình biến đổi của code tham khảo:
    float V_sense = (float)(data_temp * 3) / 4095.0f;
    float temperature = (float)(((V_sense - 0.76f) / 0.0025f) + 25.0f);

    return temperature;
}

/* ============================================================================
 * CẤU HÌNH TIMER 2 (TIM2) - ĐỊNH THỜI CHU KỲ GỬI UART 0.5s (500ms)
 * ============================================================================
 * TIM2 nằm trên APB1 bus (Clock = 16 MHz).
 * - Prescaler = 16000 - 1 = 15999 => Tần số đếm = 16MHz / 16000 = 1000 Hz (1ms/tick)
 * - Auto-Reload = 500 - 1 = 499    => Chu kỳ ngắt = 500 ms (0.5 giây)
 * - Vector ngắt: TIM2_IRQn (IRQ 28)
 */
void TIM2_Config(void)
{
    // 1. Cấp clock cho TIM2 (Bit 0 trong APB1ENR)
    RCC_APB1ENR |= (1 << 0);

    // 2. Cài đặt Prescaler và Auto-reload register
    TIM2_PSC = 15999;
    TIM2_ARR = 499;

    // 3. Kích hoạt Update Generation để nạp giá trị vào shadow registers ngay lập tức
    TIM2_EGR |= (1 << 0);
    TIM2_SR  &= ~(1 << 0); // Xóa cờ ngắt sinh ra bởi bit UG

    // 4. Bật ngắt cập nhật (UIE - Update Interrupt Enable bit 0 trong DIER)
    TIM2_DIER |= (1 << 0);

    // 5. Cho phép ngắt NVIC IRQ 28 (TIM2_IRQn)
    NVIC_ISER0 |= (1 << 28);

    // 6. Bật bộ đếm Counter Enable (CEN bit 0 trong CR1)
    TIM2_CR1 |= (1 << 0);
}

/*
 * Hàm phục vụ ngắt Timer 2 (Chu kỳ 0.5s)
 */
void TIM2_IRQHandler(void)
{
    // Kiểm tra cờ ngắt cập nhật UIF (Bit 0 trong TIM2_SR)
    if ((TIM2_SR & (1 << 0)) != 0)
    {
        TIM2_SR &= ~(1 << 0); // Xóa cờ ngắt

        // Bật cờ báo hiệu cho vòng lặp main đọc cảm biến và gửi UART
        tim2_temp_flag = 1;

        // Đảo trạng thái LED PD12 (Green) để quan sát nhịp 0.5s trên board
        GPIOD_ODR ^= (1 << 12);
    }
}

/* ============================================================================
 * HÀM MAIN CHÍNH
 * ============================================================================
 */
int main(void)
{
    // 1. Đảm bảo bảng Vector Table trỏ đúng vào địa chỉ bắt đầu của Firmware (0x08000000)
    SCB_VTOR = 0x08000000UL;

    // 2. Khởi tạo ngoại vi hoàn toàn bằng thanh ghi
    LED_Init();
    USART1_Config();
    ADC1_Init();

    // 3. In thông báo khởi động Firmware 02
    my_printf("\r\n======================================================================\r\n");
    my_printf("This is firmware 02\r\n");
    my_printf("STM32 Temperature Sensor Initialized (Channel 16)\r\n");
    my_printf("Sampling period: 0.5s via TIM2 | Baudrate: 115200 bps\r\n");
    my_printf("======================================================================\r\n\r\n");

    // 4. Khởi động Timer 2 để bắt đầu chu kỳ 0.5s
    TIM2_Config();

    // 5. Vòng lặp chính
    while (1)
    {
        // Khi Timer 2 đếm đủ 0.5s (500ms), cờ tim2_temp_flag sẽ được kích hoạt
        if (tim2_temp_flag == 1)
        {
            tim2_temp_flag = 0; // Xóa cờ

            // Đọc nhiệt độ theo chính xác hàm & phương trình của code tham khảo
            float temperature = Read_temperature();

            // Tách phần nguyên và 2 chữ số thập phân để in an toàn và chính xác
            int temp_int = (int)temperature;
            int temp_dec = (int)((temperature - (float)temp_int) * 100.0f);
            if (temp_dec < 0) temp_dec = -temp_dec;

            // Gửi dữ liệu nhiệt độ lên Terminal:
            // "Temperature of STM32: .... C"
            my_printf("Temperature of STM32: %d.%02d C\r\n", temp_int, temp_dec);
        }
    }

    return 0;
}

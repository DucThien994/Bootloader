#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#define RCC_BASE_ADDR               0x40023800UL
#define GPIOA_BASE_ADDR             0x40020000UL
#define GPIOB_BASE_ADDR             0x40020400UL
#define GPIOD_BASE_ADDR             0x40020C00UL
#define GPIOE_BASE_ADDR             0x40021000UL
#define USART1_BASE_ADDR            0x40011000UL
#define SPI1_BASE_ADDR              0x40013000UL
#define TIM1_BASE_ADDR              0x40010000UL
#define TIM2_BASE_ADDR              0x40000000UL
#define RCC_AHB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x30))
#define RCC_APB1ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x40))
#define RCC_APB2ENR                 (*(volatile uint32_t*)(RCC_BASE_ADDR + 0x44))
#define GPIOA_MODER                 (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x00))
#define GPIOA_OSPEEDR               (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x08))
#define GPIOA_PUPDR                 (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x0C))
#define GPIOA_AFRL                  (*(volatile uint32_t*)(GPIOA_BASE_ADDR + 0x20))
#define GPIOB_MODER                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OSPEEDR               (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR                 (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_AFRL                  (*(volatile uint32_t*)(GPIOB_BASE_ADDR + 0x20))
#define GPIOD_MODER                 (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x00))
#define GPIOD_ODR                   (*(volatile uint32_t*)(GPIOD_BASE_ADDR + 0x14))
#define GPIOE_MODER                 (*(volatile uint32_t*)(GPIOE_BASE_ADDR + 0x00))
#define GPIOE_OSPEEDR               (*(volatile uint32_t*)(GPIOE_BASE_ADDR + 0x08))
#define GPIOE_PUPDR                 (*(volatile uint32_t*)(GPIOE_BASE_ADDR + 0x0C))
#define GPIOE_ODR                   (*(volatile uint32_t*)(GPIOE_BASE_ADDR + 0x14))
#define USART1_SR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x00))
#define USART1_DR                   (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x04))
#define USART1_BRR                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x08))
#define USART1_CR1                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x0C))
#define USART1_CR2                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x10))
#define USART1_CR3                  (*(volatile uint32_t*)(USART1_BASE_ADDR + 0x14))
#define SPI1_CR1                    (*(volatile uint32_t*)(SPI1_BASE_ADDR + 0x00))
#define SPI1_CR2                    (*(volatile uint32_t*)(SPI1_BASE_ADDR + 0x04))
#define SPI1_SR                     (*(volatile uint32_t*)(SPI1_BASE_ADDR + 0x08))
#define SPI1_DR                     (*(volatile uint32_t*)(SPI1_BASE_ADDR + 0x0C))
#define TIM1_CR1                    (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x00))
#define TIM1_DIER                   (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x0C))
#define TIM1_SR                     (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x10))
#define TIM1_EGR                    (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x14))
#define TIM1_CNT                    (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x24))
#define TIM1_PSC                    (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x28))
#define TIM1_ARR                    (*(volatile uint32_t*)(TIM1_BASE_ADDR + 0x2C))
#define TIM2_CR1                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x00))
#define TIM2_DIER                   (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x0C))
#define TIM2_SR                     (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x10))
#define TIM2_EGR                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x14))
#define TIM2_CNT                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x24))
#define TIM2_PSC                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x28))
#define TIM2_ARR                    (*(volatile uint32_t*)(TIM2_BASE_ADDR + 0x2C))
#define NVIC_ISER0                  (*(volatile uint32_t*)0xE000E100UL)

#define I3G4250D_WHO_AM_I_ADDR      0x0F
#define I3G4250D_CTRL_REG1_ADDR     0x20
#define I3G4250D_CTRL_REG4_ADDR     0x23
#define I3G4250D_STATUS_REG_ADDR    0x27
#define I3G4250D_OUT_X_L_ADDR       0x28

volatile uint8_t tim1_gyro_flag = 0;

void delay_ms(uint32_t ms);
void LED_Init(void);
void USART1_Config(void);
void usart1_send(char data);
void my_printf(const char *str, ...);
void SPI1_Config(void);
uint8_t SPI1_Transfer(uint8_t data);
void CS_LOW(void);
void CS_HIGH(void);
void I3G4250D_WriteReg(uint8_t reg, uint8_t val);
uint8_t I3G4250D_ReadReg(uint8_t reg);
uint8_t I3G4250D_Init(void);
void I3G4250D_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
void TIM1_Config(void);
void TIM2_Config(void);

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
 * CẤU HÌNH 4 LED TRÊN KIT STM32F411 DISCOVERY (PD12, PD13, PD14, PD15)
 * ============================================================================
 */
void LED_Init(void)
{
    // 1. Cấp clock cho Port D (Bit 3 trong AHB1ENR)
    RCC_AHB1ENR |= (1 << 3);

    // 2. Cấu hình PD12, PD13, PD14, PD15 sang General Purpose Output (01b)
    GPIOD_MODER &= ~(0xFF << 24);
    GPIOD_MODER |=  (0x55 << 24);

    // 3. Khởi tạo ban đầu: Tắt cả 4 LED
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

    // Tốc độ cao và kích hoạt điện trở kéo lên Pull-up
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
    while (((USART1_SR >> 7) & 1) == 0); // Chờ TXE = 1 (Transmit data register empty)
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
 * CẤU HÌNH SPI1 VÀ CHÂN ĐIỀU KHIỂN CẢM BIẾN I3G4250D
 * ============================================================================
 * Kit STM32F411E-DISCO kết nối Gyroscope qua:
 * - CS   : PE3 (GPIO Output)
 * - SCK  : PA5 (SPI1 Alternate Function AF5)
 * - MISO : PA6 (SPI1 Alternate Function AF5)
 * - MOSI : PA7 (SPI1 Alternate Function AF5)
 */
void CS_LOW(void)
{
    GPIOE_ODR &= ~(1 << 3); // Kéo PE3 xuống mức 0 để chọn Chip
}

void CS_HIGH(void)
{
    // Chờ cờ BSY (Bit 7) = 0 để đảm bảo SPI đã truyền nhận xong byte cuối cùng
    while (((SPI1_SR >> 7) & 1) != 0);
    GPIOE_ODR |= (1 << 3);  // Kéo PE3 lên mức 1 để nhả Chip
}

void SPI1_Config(void)
{
    // 1. Cấp clock cho GPIOA (Bit 0), GPIOE (Bit 4) và SPI1 (Bit 12 APB2ENR)
    RCC_AHB1ENR |= (1 << 0) | (1 << 4);
    RCC_APB2ENR |= (1 << 12);

    // 2. Cấu hình chân Chip Select PE3 làm General Purpose Output
    GPIOE_MODER   &= ~(0x3 << 6);
    GPIOE_MODER   |=  (0x1 << 6);  // Output mode (01b)
    GPIOE_OSPEEDR |=  (0x3 << 6);  // Very high speed
    GPIOE_PUPDR   &= ~(0x3 << 6);
    GPIOE_PUPDR   |=  (0x1 << 6);  // Pull-up
    GPIOE_ODR     |=  (1 << 3);    // Mặc định thả CS = HIGH (không chọn chip)

    // 3. Cấu hình PA5 (SCK), PA6 (MISO), PA7 (MOSI) sang AF5 (SPI1)
    GPIOA_MODER &= ~((0x3 << 10) | (0x3 << 12) | (0x3 << 14));
    GPIOA_MODER |=  ((0x2 << 10) | (0x2 << 12) | (0x2 << 14)); // Alternate Function (10b)

    // Gán AF5 (0101b) cho PA5, PA6, PA7 trong AFRL
    GPIOA_AFRL &= ~((0xF << 20) | (0xF << 24) | (0xF << 28));
    GPIOA_AFRL |=  ((0x5 << 20) | (0x5 << 24) | (0x5 << 28));

    // Cấu hình tốc độ cao cho các chân SPI
    GPIOA_OSPEEDR |= ((0x3 << 10) | (0x3 << 12) | (0x3 << 14));
    GPIOA_PUPDR   &= ~((0x3 << 10) | (0x3 << 12) | (0x3 << 14));

    // 4. Cấu hình ngoại vi SPI1:
    // - Mode 3: CPOL = 1 (bit 1), CPHA = 1 (bit 0)
    // - Master mode: MSTR = 1 (bit 2)
    // - Baudrate: BR[2:0] = 010b => fPCLK2 / 8 = 16MHz / 8 = 2 MHz (rất ổn định)
    // - 8-bit frame: DFF = 0 (bit 11)
    // - MSB first: LSBFIRST = 0 (bit 7)
    // - Software Slave Management: SSM = 1 (bit 9), SSI = 1 (bit 8)
    // - Kích hoạt SPI: SPE = 1 (bit 6)
    SPI1_CR1 = (1 << 9) | (1 << 8) | (1 << 6) | (2 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
}

uint8_t SPI1_Transfer(uint8_t data)
{
    // Chờ cờ TXE = 1 (Transmit buffer empty)
    while (((SPI1_SR >> 1) & 1) == 0);
    SPI1_DR = data;

    // Chờ cờ RXNE = 1 (Receive buffer not empty)
    while (((SPI1_SR >> 0) & 1) == 0);
    return (uint8_t)(SPI1_DR & 0xFF);
}

void I3G4250D_WriteReg(uint8_t reg, uint8_t val)
{
    CS_LOW();
    // Bit 7 = 0 (Write), Bit 6 = 0 (Single access)
    SPI1_Transfer(reg & 0x3F);
    SPI1_Transfer(val);
    CS_HIGH();
}

uint8_t I3G4250D_ReadReg(uint8_t reg)
{
    CS_LOW();
    // Bit 7 = 1 (Read), Bit 6 = 0 (Single access)
    SPI1_Transfer(0x80 | (reg & 0x3F));
    uint8_t val = SPI1_Transfer(0x00); // Gửi dummy byte để nhận dữ liệu về
    CS_HIGH();
    return val;
}

uint8_t I3G4250D_Init(void)
{
    delay_ms(20); // Chờ cảm biến ổn định nguồn

    // 1. Kiểm tra mã định danh Chip ID (WHO_AM_I)
    // I3G4250D = 0xD3, L3GD20 = 0xD4, L3GD20H = 0xD7
    uint8_t who_am_i = I3G4250D_ReadReg(I3G4250D_WHO_AM_I_ADDR);

    // 2. Cấu hình CTRL_REG1:
    // DR=00 (100Hz ODR), BW=00 (12.5Hz Cutoff), PD=1 (Normal mode), Zen=1, Yen=1, Xen=1
    // 0x0F = 0000 1111b
    I3G4250D_WriteReg(I3G4250D_CTRL_REG1_ADDR, 0x0F);

    // 3. Cấu hình CTRL_REG4:
    // BDU=1 (Block Data Update: tránh đọc lệch byte L và H), FS=00 (Full scale 245/250 dps)
    // 0x80 = 1000 0000b
    I3G4250D_WriteReg(I3G4250D_CTRL_REG4_ADDR, 0x80);

    delay_ms(10);
    return who_am_i;
}

void I3G4250D_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    CS_LOW();
    // Bit 7 = 1 (Read), Bit 6 = 1 (Auto-increment address starting from OUT_X_L: 0x28)
    SPI1_Transfer(0xC0 | I3G4250D_OUT_X_L_ADDR);

    uint8_t xl = SPI1_Transfer(0x00);
    uint8_t xh = SPI1_Transfer(0x00);
    uint8_t yl = SPI1_Transfer(0x00);
    uint8_t yh = SPI1_Transfer(0x00);
    uint8_t zl = SPI1_Transfer(0x00);
    uint8_t zh = SPI1_Transfer(0x00);
    CS_HIGH();

    *x = (int16_t)((xh << 8) | xl);
    *y = (int16_t)((yh << 8) | yl);
    *z = (int16_t)((zh << 8) | zl);
}

/* ============================================================================
 * CẤU HÌNH TIMER 1 (TIM1) - ĐỊNH THỜI CẬP NHẬT DỮ LIỆU GYRO 0.5s (500ms)
 * ============================================================================
 * TIM1 nằm trên APB2 bus (Clock = 16 MHz).
 * - Prescaler = 16000 - 1 = 15999 => Tần số đếm = 16MHz / 16000 = 1000 Hz (1ms/tick)
 * - Auto-Reload = 500 - 1 = 499    => Chu kỳ ngắt = 500 ms (0.5 giây)
 * - Vector ngắt: TIM1_UP_TIM10_IRQn (IRQ 25)
 */
void TIM1_Config(void)
{
    // 1. Cấp clock cho TIM1 (Bit 0 trong APB2ENR)
    RCC_APB2ENR |= (1 << 0);

    // 2. Cài đặt Prescaler và Auto-reload register
    TIM1_PSC = 15999;
    TIM1_ARR = 499;

    // 3. Kích hoạt Update Generation để nạp giá trị vào thanh ghi đệm ngay lập tức
    TIM1_EGR |= (1 << 0);
    TIM1_SR  &= ~(1 << 0); // Xóa cờ ngắt sinh ra bởi bit UG

    // 4. Bật ngắt cập nhật (UIE - Update Interrupt Enable bit 0 trong DIER)
    TIM1_DIER |= (1 << 0);

    // 5. Cho phép ngắt NVIC IRQ 25 (TIM1_UP_TIM10_IRQn)
    NVIC_ISER0 |= (1 << 25);

    // 6. Bật bộ đếm Counter Enable (CEN bit 0 trong CR1)
    TIM1_CR1 |= (1 << 0);
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    // Kiểm tra cờ ngắt cập nhật UIF (Bit 0 trong TIM1_SR)
    if ((TIM1_SR & (1 << 0)) != 0)
    {
        TIM1_SR &= ~(1 << 0); // Xóa cờ ngắt

        // Bật cờ báo hiệu cho vòng lặp main đọc cảm biến và truyền UART
        tim1_gyro_flag = 1;
    }
}

/* ============================================================================
 * CẤU HÌNH TIMER 2 (TIM2) - ĐIỀU KHIỂN CHẠY LẦN LƯỢT 4 LED VỚI CHU KỲ 2 GIÂY
 * ============================================================================
 * TIM2 nằm trên APB1 bus (Clock = 16 MHz).
 * Chu kỳ tổng cộng 2 giây được chia đều cho 4 LED (PD12 -> PD13 -> PD14 -> PD15):
 * Mỗi LED sáng trong 2000 ms / 4 = 500 ms (0.5 giây).
 * - Prescaler = 16000 - 1 = 15999 => Tần số đếm = 1000 Hz (1ms/tick)
 * - Auto-Reload = 500 - 1 = 499    => Chu kỳ ngắt = 500 ms
 * - Vector ngắt: TIM2_IRQn (IRQ 28)
 */
void TIM2_Config(void)
{
    // 1. Cấp clock cho TIM2 (Bit 0 trong APB1ENR)
    RCC_APB1ENR |= (1 << 0);

    // 2. Cài đặt Prescaler và Auto-reload register
    TIM2_PSC = 15999;
    TIM2_ARR = 499;

    // 3. Nạp shadow registers
    TIM2_EGR |= (1 << 0);
    TIM2_SR  &= ~(1 << 0);

    // 4. Bật ngắt cập nhật (UIE bit 0)
    TIM2_DIER |= (1 << 0);

    // 5. Cho phép ngắt NVIC IRQ 28 (TIM2_IRQn)
    NVIC_ISER0 |= (1 << 28);

    // 6. Bật bộ đếm Counter Enable (CEN bit 0)
    TIM2_CR1 |= (1 << 0);
}

void TIM2_IRQHandler(void)
{
    // Kiểm tra cờ ngắt cập nhật UIF (Bit 0 trong TIM2_SR)
    if ((TIM2_SR & (1 << 0)) != 0)
    {
        TIM2_SR &= ~(1 << 0); // Xóa cờ ngắt

        // Hiệu ứng chạy lần lượt 4 LED (PD12 -> PD13 -> PD14 -> PD15)
        // Mỗi bước giữ 0.5s => 4 bước = 2.0s hoàn thành 1 chu kỳ trọn vẹn
        static uint8_t led_step = 0;

        // Tắt toàn bộ 4 LED trước khi bật LED kế tiếp
        GPIOD_ODR &= ~(0xF << 12);

        // Bật duy nhất 1 LED tại vị trí bước hiện tại
        GPIOD_ODR |= (1 << (12 + led_step));

        // Chuyển sang bước tiếp theo
        led_step = (led_step + 1) % 4;
    }
}

/* ============================================================================
 * HÀM MAIN CHÍNH
 * ============================================================================
 */
int main(void)
{
    // 1. Khởi tạo ngoại vi bằng thanh ghi
    LED_Init();
    USART1_Config();
    SPI1_Config();

    // 2. In chuỗi thông báo ngay khi nạp / khởi động firmware 01
    my_printf("This is firmware 01\r\n");

    // 3. Khởi tạo cảm biến Gyroscope I3G4250D qua SPI1
    uint8_t chip_id = I3G4250D_Init();
    my_printf("I3G4250D Gyroscope Initialized. Device ID: 0x%02X\r\n", chip_id);
    my_printf("======================================================================\r\n");

    // 4. Khởi động Timer 1 (cập nhật UART 0.5s) và Timer 2 (chu kỳ 2s chạy 4 LED)
    TIM1_Config();
    TIM2_Config();
    I3G4250D_WriteReg(I3G4250D_CTRL_REG4_ADDR, 0x02);

    // 5. Vòng lặp chính
    while (1)
    {
        // Khi Timer 1 đếm đủ 0.5s (500ms), cờ tim1_gyro_flag sẽ được kích hoạt
        if (tim1_gyro_flag == 1)
        {
            tim1_gyro_flag = 0; // Xóa cờ

            // Đọc dữ liệu 3 trục X, Y, Z (16-bit Raw ADC)
            int16_t x_raw = 0, y_raw = 0, z_raw = 0;
            I3G4250D_ReadXYZ(&x_raw, &y_raw, &z_raw);

            // Quy đổi sang vận tốc góc dps (Độ/giây):
            // Độ nhạy tại Full Scale 245/250 dps: 8.75 mdps/LSB = 0.00875 dps/LSB
            float x_dps = (float)x_raw * 0.00875f;
            float y_dps = (float)y_raw * 0.00875f;
            float z_dps = (float)z_raw * 0.00875f;

            // Tách phần nguyên và 2 số thập phân để in an toàn
            int x_int = (int)x_dps;
            int x_dec = (int)((x_dps - (float)x_int) * 100.0f);
            if (x_dec < 0) x_dec = -x_dec;

            int y_int = (int)y_dps;
            int y_dec = (int)((y_dps - (float)y_int) * 100.0f);
            if (y_dec < 0) y_dec = -y_dec;

            int z_int = (int)z_dps;
            int z_dec = (int)((z_dps - (float)z_int) * 100.0f);
            if (z_dec < 0) z_dec = -z_dec;

            // In đồng thời cả giá trị Raw và giá trị quy đổi DPS lên Terminal
            my_printf("[I3G4250D] RAW: X=%-6d Y=%-6d Z=%-6d | DPS: X=%d.%02d Y=%d.%02d Z=%d.%02d dps\r\n",
                      x_raw, y_raw, z_raw,
                      x_int, x_dec,
                      y_int, y_dec,
                      z_int, z_dec);
        }
    }

    return 0;
}


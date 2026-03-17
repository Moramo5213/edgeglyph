#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------
// ESP32-C3 Hardware Registers
// ---------------------------------------------------------
#define GPIO_BASE           0x60004000
#define GPIO_OUT            (GPIO_BASE + 0x0004)
#define GPIO_OUT_W1TS      (GPIO_BASE + 0x0008)
#define GPIO_OUT_W1TC      (GPIO_BASE + 0x000C)
#define GPIO_ENABLE         (GPIO_BASE + 0x0020)
#define GPIO_FUNC_OUT_BASE  (GPIO_BASE + 0x0554)

#define IO_MUX_BASE         0x60009000

#define UART0_BASE          0x60000000
#define UART0_FIFO          (UART0_BASE + 0x0000)
#define UART0_STATUS        (UART0_BASE + 0x001C)

#define USB_BASE            0x60043000
#define USB_FIFO            (USB_BASE + 0x0000)
#define USB_CONF            (USB_BASE + 0x0004)

// Board Pins from board.md
#define PIN_LCD_RST  1
#define PIN_LCD_DC   2
#define PIN_LCD_CS   10
#define PIN_LCD_CLK  6
#define PIN_LCD_MOSI 7
#define PIN_LCD_BL   3

// ---------------------------------------------------------
// GPIO Driver
// ---------------------------------------------------------
void delay_ms(volatile uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms; i++) {
        for (volatile int j = 0; j < 15000; j++) { __asm__("nop"); }
    }
}

void gpio_init(int pin) {
    // 1. IO MUX: Set to Function 1 (GPIO)
    // Register index = 4 + pin * 4
    *(volatile uint32_t*)(IO_MUX_BASE + 4 + pin * 4) = (1 << 12) | (1 << 9); // MCU_SEL=1, FUN_IE=1
    
    // 2. GPIO Matrix: Set to direct output (0x80)
    *(volatile uint32_t*)(GPIO_FUNC_OUT_BASE + pin * 4) = 0x80;
    
    // 3. Enable Output
    *(volatile uint32_t*)GPIO_ENABLE |= (1 << pin);
}

void gpio_set(int pin, int level) {
    if (level) *(volatile uint32_t*)GPIO_OUT_W1TS = (1 << pin);
    else *(volatile uint32_t*)GPIO_OUT_W1TC = (1 << pin);
}

// ---------------------------------------------------------
// Bitbang SPI
// ---------------------------------------------------------
void bb_spi_write(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_set(PIN_LCD_MOSI, (data & 0x80));
        gpio_set(PIN_LCD_CLK, 0);
        data <<= 1;
        // Small nop delay for signal setup
        for (volatile int j = 0; j < 5; j++); 
        gpio_set(PIN_LCD_CLK, 1);
        for (volatile int j = 0; j < 5; j++);
    }
}

void lcd_cmd(uint8_t cmd) {
    gpio_set(PIN_LCD_DC, 0);
    gpio_set(PIN_LCD_CS, 0);
    bb_spi_write(cmd);
    gpio_set(PIN_LCD_CS, 1);
}

void lcd_data(uint8_t data) {
    gpio_set(PIN_LCD_DC, 1);
    gpio_set(PIN_LCD_CS, 0);
    bb_spi_write(data);
    gpio_set(PIN_LCD_CS, 1);
}

// ---------------------------------------------------------
// LCD Initialization
// ---------------------------------------------------------
void lcd_init() {
    gpio_init(PIN_LCD_RST);
    gpio_init(PIN_LCD_DC);
    gpio_init(PIN_LCD_CS);
    gpio_init(PIN_LCD_CLK);
    gpio_init(PIN_LCD_MOSI);
    gpio_init(PIN_LCD_BL);

    // Hard Reset
    gpio_set(PIN_LCD_RST, 0);
    delay_ms(100);
    gpio_set(PIN_LCD_RST, 1);
    delay_ms(100);

    // Backlight steady ON
    gpio_set(PIN_LCD_BL, 1);

    lcd_cmd(0xEF);
    lcd_cmd(0xEB); lcd_data(0x14);
    lcd_cmd(0xFE);
    lcd_cmd(0xEF);
    lcd_cmd(0xEB); lcd_data(0x14);
    lcd_cmd(0x84); lcd_data(0x40);
    lcd_cmd(0x85); lcd_data(0xFF);
    lcd_cmd(0x86); lcd_data(0xFF);
    lcd_cmd(0x87); lcd_data(0xFF);
    lcd_cmd(0x88); lcd_data(0x0A);
    lcd_cmd(0x89); lcd_data(0x21);
    lcd_cmd(0x8A); lcd_data(0x00);
    lcd_cmd(0x8B); lcd_data(0x80);
    lcd_cmd(0x8C); lcd_data(0x01);
    lcd_cmd(0x8D); lcd_data(0x01);
    lcd_cmd(0x8E); lcd_data(0xFF);
    lcd_cmd(0x8F); lcd_data(0xFF);
    lcd_cmd(0xB6); lcd_data(0x00); lcd_data(0x00);
    lcd_cmd(0x36); lcd_data(0x08);
    lcd_cmd(0x3A); lcd_data(0x05);
    lcd_cmd(0x90); lcd_data(0x08); lcd_data(0x08); lcd_data(0x08); lcd_data(0x08);
    lcd_cmd(0xBD); lcd_data(0x06);
    lcd_cmd(0xBC); lcd_data(0x00);
    lcd_cmd(0xFF); lcd_data(0x60); lcd_data(0x01); lcd_data(0x04);
    lcd_cmd(0x11); // Sleep out
    delay_ms(120);
    lcd_cmd(0x29); // Display ON
}

void lcd_clear(uint16_t color) {
    lcd_cmd(0x2A); lcd_data(0); lcd_data(0); lcd_data(0); lcd_data(239);
    lcd_cmd(0x2B); lcd_data(0); lcd_data(0); lcd_data(0); lcd_data(239);
    lcd_cmd(0x2C);
    for (uint32_t i = 0; i < 240 * 240; i++) {
        lcd_data(color >> 8);
        lcd_data(color & 0xFF);
    }
}

// ---------------------------------------------------------
// Heartbeat UART
// ---------------------------------------------------------
void uart_putc(char c) {
    while (((*((volatile uint32_t*)UART0_STATUS) >> 16) & 0xFF) >= 128);
    *((volatile uint32_t*)UART0_FIFO) = (uint32_t)c;
    while (!(*((volatile uint32_t*)USB_CONF) & 0x1));
    *((volatile uint32_t*)USB_FIFO) = (uint32_t)c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

// ---------------------------------------------------------
// Entry Point
// ---------------------------------------------------------
void kernel_main() {
    uart_puts("\n\n*** TENSOR-CORE RECOVERY BOOT ***\n");
    uart_puts("Initializing GPIOs...\n");
    
    lcd_init();
    
    // Clear to RED if we hit initialization
    uart_puts("Clearing LCD to RED...\n");
    lcd_clear(0xF800); 

    while (1) {
        delay_ms(500);
        uart_putc('!'); // Heartbeat
    }
}

// Minimal stubs for linker
void* memset(void* s, int c, size_t n) { return s; }
void* memcpy(void* d, const void* s, size_t n) { return d; }

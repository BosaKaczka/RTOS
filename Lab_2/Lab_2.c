// #include <boards/pico2_w.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/vreg.h"

#include "hardware/uart.h"

#include "pico/cyw43_arch.h"


// UART defines
#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 4
#define UART_RX_PIN 5


void task_1();
bool task_2(bool done);
int get_number();
void task_3(int choice, bool done);
void task_4();

int main()
{
    bool done = false;


    stdio_init_all();
    cyw43_arch_init();

    done = task_2(done);

    
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
 
    
    while (true) {

        printf("\n---New Cycle---\n");

        task_1();
        // done = task_2(done);
        task_3(get_number(), done);
        task_4 ();
    
        printf("\n\n\n\n");
        sleep_ms(1000);

    }
}

void task_1(){
    // PLL
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    // PLL USB
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    // ROSC
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    // clk_sys
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    // clk_peri
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    // clk_usb
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    // clk_adc
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);

    printf("---------- Task 1 ----------\n");
        sleep_ms(10); 
        printf("pll_sys  = %dkHz\n", f_pll_sys);
        sleep_ms(10);
        printf("pll_usb  = %dkHz\n", f_pll_usb);
        sleep_ms(10);
        printf("rosc     = %dkHz\n", f_rosc);
        sleep_ms(10);
        printf("clk_sys  = %dkHz\n", f_clk_sys);
        sleep_ms(10);
        printf("clk_peri = %dkHz\n", f_clk_peri);
        sleep_ms(10);
        printf("clk_usb  = %dkHz\n", f_clk_usb);
        sleep_ms(10);
        printf("clk_adc  = %dkHz\n", f_clk_adc);
        sleep_ms(10);
        printf("-----------------------------\n");

};

bool task_2(bool done){

    if(!done){
        // Connect clk_ref and clk_peri to XOSC (12MHz)
        clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, 12 * MHZ, 12 * MHZ);

        clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 12 * MHZ, 12 * MHZ);

        stdio_init_all();

        printf("---------- Task 2 ----------\n  DONE  \n-----------------------------\n");

        return true;
    }else{
        printf("---------- Task 2 ----------\n  ALREADY DONE  \n-----------------------------\n");
    }


    return false;
};

int get_number() {
    printf("\n--- Waiting for Input (0-3) ---\n");

    while (true) {
        char c = uart_getc(UART_ID); 

        if (c >= '0' && c <= '3') {
            int val = c - '0';  // change to int
            printf("Received: %d\n", val);
            return val;
        }

        if (c != '\n' && c != '\r') {
            printf("Invalid input: %c. Use 0, 1, 2, or 3.\n", c);
        }
    }
}

void task_3(int choice, bool done) {
    uint32_t vco = 0, post1 = 0, post2 = 0;
    uint32_t target_freq = 0;

    printf("---------- Task 3 ----------\n");

    //switch clk_sys to clk_ref (XOSC)

    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0,          
                    12 * MHZ,
                    12 * MHZ);

    if (choice == 0) {
        pll_deinit(pll_sys);
        printf("Switched back to clk_ref.\n");
        return;
    }

    switch (choice) {
        case 1: vco = 1500 * MHZ; post1 = 6; post2 = 2; break; // 125 MHz
        case 2: vco = 1200 * MHZ; post1 = 4; post2 = 2; break; // 150 MHz
        case 3: vco = 1200 * MHZ; post1 = 3; post2 = 2; break; // 200 MHz
        default: return;
    }

    target_freq = vco / (post1 * post2);

    // Reconfigure the PLL
    pll_init(pll_sys, 1, vco, post1, post2);

    // Switch clk_sys to the PLL
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    target_freq,
                    target_freq);

    printf("Freq switched to %u MHz.\n", target_freq / MHZ);
    printf("-----------------------------\n");
}



void task_4(){
    // task 4
    printf("---------- Task 4 ----------\n");
    uint f_pll = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("Verification -> PLL_SYS: %d kHz, CLK_SYS: %d kHz\n", f_pll, f_sys);
    printf("-----------------------------\n");
}
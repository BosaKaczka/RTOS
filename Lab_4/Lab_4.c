#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define ADDRESS 0x19
#define CTRL_REG1 0x20
#define OUT_Z_L 0x2C
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define LED0_PIN 10

#define TAP_THRESHOLD 4000
#define DOUBLE_TAP_GAP_MS 500
#define DEBOUNCE_MS 150

void task_1(void *pvParameters);
void task_2(void *pvParameters);
void init_gpio();
void init_queue();

QueueHandle_t queue;

int main()
{
    stdio_init_all();
    init_gpio();
    init_queue();

    xTaskCreate(task_1, "Task 1", 2048, NULL, 1, NULL);
    xTaskCreate(task_2, "Task 2", 2048, NULL, 2, NULL);

    vTaskStartScheduler();
    while (true)
    {
    };
}

void init_gpio()
{
    gpio_init(LED0_PIN);
    gpio_set_dir(LED0_PIN, GPIO_OUT);
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_disable_pulls(I2C_SDA);
    gpio_disable_pulls(I2C_SCL);
}

void init_queue()
{
    queue = xQueueCreate(32, sizeof(int16_t));
}

// Task 1: Akwizycja danych
void task_1(void *pvParameters)
{
    uint8_t config[] = {CTRL_REG1, 0x77}; // 400Hz
    i2c_write_blocking(I2C_PORT, ADDRESS, config, 2, false);

    int16_t z_accel;
    uint8_t reg = OUT_Z_L | 0x80;
    uint8_t data[2];

    while (true)
    {
        i2c_write_blocking(I2C_PORT, ADDRESS, &reg, 1, true);
        i2c_read_blocking(I2C_PORT, ADDRESS, data, 2, false);
        z_accel = (int16_t)(data[0] | (data[1] << 8));

        xQueueSend(queue, &z_accel, 0);
        vTaskDelay(pdMS_TO_TICKS(2)); // ok. 500Hz
    }
}

// Task 2: Data Analysis
void task_2(void *pvParameters)
{
    int16_t received_z;
    int16_t last_z = 0;
    TickType_t last_tap_tick = 0;
    bool first_tap_detected = false;
    bool initialized = false;

    while (true)
    {
        if (xQueueReceive(queue, &received_z, portMAX_DELAY))
        {
            if (!initialized)
            {
                last_z = received_z;
                initialized = true;
                continue;
            }

            int32_t delta = (int32_t)received_z - (int32_t)last_z;
            int32_t abs_delta = (delta < 0) ? -delta : delta;
            last_z = received_z;

            TickType_t current_tick = xTaskGetTickCount();
            // Convert gap and debounce to ticks
            TickType_t debounce_ticks = pdMS_TO_TICKS(DEBOUNCE_MS);
            TickType_t gap_ticks = pdMS_TO_TICKS(DOUBLE_TAP_GAP_MS);

            if (abs_delta > TAP_THRESHOLD)
            {
                // Debounce check
                if ((current_tick - last_tap_tick) < debounce_ticks)
                {
                    continue;
                }

                if (!first_tap_detected)
                {
                    first_tap_detected = true;
                    last_tap_tick = current_tick;
                }
                else
                {
                    if ((current_tick - last_tap_tick) < gap_ticks)
                    {
                        printf("Double Tap Detected!\n");
                        gpio_put(LED0_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(100)); // Brief flash
                        gpio_put(LED0_PIN, 0);

                        first_tap_detected = false;
                    }
                    else
                    {
                        // Current tap is now the first tap of a potential new sequence
                        last_tap_tick = current_tick;
                    }
                }
            }

            // Reset if the second tap never comes
            if (first_tap_detected && (current_tick - last_tap_tick > gap_ticks))
            {
                first_tap_detected = false;
            }
        }
    }
}
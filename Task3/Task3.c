#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LED0_PIN 10
#define LED1_PIN 11
#define LED2_PIN 12
#define BUTTON_PIN 13

void task_1(void *pvParameters);
void task_2(void *pvParameters);
void task_3(void *pvParameters);

void init_gpio();
void init_queue();

QueueHandle_t queue;

int main()
{
    stdio_init_all();

    init_gpio();
    init_queue();

    printf("------ FreeRTOS START ------\n");

    xTaskCreate(task_1, "task_1", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "task_2", 256, NULL, 1, NULL);
    xTaskCreate(task_3, "task_3", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
    {
        sleep_ms(100);
    }
}

void init_gpio()
{
    gpio_init(LED0_PIN);
    gpio_set_dir(LED0_PIN, GPIO_OUT);

    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    printf("GPIO initialized\n");
}

void init_queue()
{
    queue = xQueueCreate(16, sizeof(uint8_t));

    if (queue != NULL)
    {
        printf("Queue created successfully\n");
    }
    else
    {
        printf("Queue creation failed\n");
    }
}

// TASK 1
void task_1(void *pvParameters)
{
    while (true)
    {
        gpio_put(LED2_PIN, true);
        vTaskDelay(pdMS_TO_TICKS(33));

        gpio_put(LED2_PIN, false);
        vTaskDelay(pdMS_TO_TICKS(67));
    }
}

// TASK 2
void task_2(void *pvParameters)
{
    uint8_t core_id;

    while (true)
    {

        if (!gpio_get(BUTTON_PIN))
        {

            vTaskDelay(pdMS_TO_TICKS(50)); // debounce

            while (!gpio_get(BUTTON_PIN))
            {
                vTaskDelay(pdMS_TO_TICKS(3));
            }

            core_id = (uint8_t)portGET_CORE_ID();

            printf("Button pressed Core ID: %d\n", core_id);

            if (xQueueSend(queue, &core_id, 0) != pdPASS)
            {
                printf("Queue full!\n");
            }

            // When queue overflows, new button press events are discarded because xQueueSend() is called with zero timeout.
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// TASK 3
void task_3(void *pvParameters)
{
    uint8_t core_id;

    while (true)
    {

        if (xQueueReceive(queue, &core_id, portMAX_DELAY) == pdPASS)
        {

            printf("Received from queue: Core %d\n", core_id);

            if (core_id == 0)
            {
                gpio_put(LED0_PIN, true);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_put(LED0_PIN, false);
            }
            else if (core_id == 1)
            {
                gpio_put(LED1_PIN, true);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_put(LED1_PIN, false);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// To run FreeRTOS in single-core mode:
// set in FreeRTOSConfig.h
// #define configNUMBER_OF_CORES 1
//
// In this mode scheduler runs tasks only on one core.
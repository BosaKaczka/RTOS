#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/version.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "ssd1306.h"
#include "lis3dh.h"

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
#define I2C_OLED_PORT i2c0
#define I2C_OLED_SDA 8
#define I2C_OLED_SCL 9

#define I2C_ACCEL_PORT i2c1
#define I2C_ACCEL_SDA 6
#define I2C_ACCEL_SCL 7

#define BTN_UP_PIN 16
#define BTN_DOWN_PIN 18
#define BTN_OK_PIN 17

#define LED_PIN 10

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
#define ACCEL_POLL_MS 50 // 20 Hz - well above the required 10 Hz minimum
#define DEBOUNCE_MS 50

#define NUM_BUTTONS 3

// ---------------------------------------------------------------------------
// Menu / screen state
// ---------------------------------------------------------------------------
typedef enum
{
    SCREEN_HOME = 0,
    SCREEN_ACCEL = 1,
    SCREEN_SYSINFO = 2,
    SCREEN_COUNT = 3
} screen_index_t;

typedef enum
{
    MODE_MENU,  // navigating the top-level list
    MODE_DETAIL // a screen has been selected and is showing detail
} menu_mode_t;

// ---------------------------------------------------------------------------
// Button event type
// ---------------------------------------------------------------------------
typedef enum
{
    BTN_EVENT_UP,
    BTN_EVENT_DOWN,
    BTN_EVENT_OK
} btn_event_t;

// ---------------------------------------------------------------------------
// Shared accelerometer data (written by Task 1, read by Task 3)
// ---------------------------------------------------------------------------
static volatile lis3dh_data_t g_accel_data;
static SemaphoreHandle_t g_accel_mutex;

// ---------------------------------------------------------------------------
// Button event queue (written by Task 2, read by Task 3)
// ---------------------------------------------------------------------------
static QueueHandle_t g_btn_queue;

// ---------------------------------------------------------------------------
// Button pin and event lookup tables (file-scope avoids C89 local issues)
// ---------------------------------------------------------------------------
static const unsigned int g_btn_pins[NUM_BUTTONS] = {BTN_UP_PIN, BTN_DOWN_PIN, BTN_OK_PIN};
static const btn_event_t g_btn_events[NUM_BUTTONS] = {BTN_EVENT_UP, BTN_EVENT_DOWN, BTN_EVENT_OK};

// ---------------------------------------------------------------------------
// Menu label table
// ---------------------------------------------------------------------------
static const char *g_menu_labels[SCREEN_COUNT] = {
    "RZUlF Mie gonji",
    "Accelerometer",
    "System Info"};

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void task_accel(void *pv);
static void task_buttons(void *pv);
static void task_display(void *pv);
static void init_hardware(void);

// ===========================================================================
// main
// ===========================================================================
int main(void)
{
    stdio_init_all();
    init_hardware();

    g_accel_mutex = xSemaphoreCreateMutex();
    g_btn_queue = xQueueCreate(16, sizeof(btn_event_t));

    xTaskCreate(task_accel, "Accel", 512, NULL, 2, NULL);
    xTaskCreate(task_buttons, "Buttons", 512, NULL, 3, NULL);
    xTaskCreate(task_display, "Display", 1024, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true)
    {
    }
}

// ===========================================================================
// Hardware initialisation
// ===========================================================================
static void init_hardware(void)
{
    int i;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        gpio_init(g_btn_pins[i]);
        gpio_set_dir(g_btn_pins[i], GPIO_IN);
        gpio_pull_up(g_btn_pins[i]);
    }

    // I2C0 - OLED display
    i2c_init(I2C_OLED_PORT, 400 * 1000);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);

    // I2C1 - Accelerometer
    i2c_init(I2C_ACCEL_PORT, 400 * 1000);
    gpio_set_function(I2C_ACCEL_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_ACCEL_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_ACCEL_SDA);
    gpio_pull_up(I2C_ACCEL_SCL);
}

// ===========================================================================
// Task 1 - Accelerometer polling
// Reads XYZ at ACCEL_POLL_MS interval and stores in g_accel_data.
// ===========================================================================
static void task_accel(void *pv)
{
    lis3dh_data_t sample;

    lis3dh_init(I2C_ACCEL_PORT);

    while (true)
    {
        lis3dh_read(I2C_ACCEL_PORT, &sample);

        xSemaphoreTake(g_accel_mutex, portMAX_DELAY);
        g_accel_data = sample;
        xSemaphoreGive(g_accel_mutex);

        vTaskDelay(pdMS_TO_TICKS(ACCEL_POLL_MS));
    }
}

// ===========================================================================
// Task 2 - Button polling with debounce
// Detects UP / DOWN / OK presses and pushes events into g_btn_queue.
// ===========================================================================
static void task_buttons(void *pv)
{
    bool last_stable[NUM_BUTTONS];
    uint32_t last_change[NUM_BUTTONS];
    uint32_t now;
    int i;
    bool current;

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        last_stable[i] = true; // pull-up: idle = high
        last_change[i] = 0;
    }

    while (true)
    {
        now = to_ms_since_boot(get_absolute_time());

        for (i = 0; i < NUM_BUTTONS; i++)
        {
            current = gpio_get(g_btn_pins[i]);

            if (current != last_stable[i])
            {
                if ((now - last_change[i]) >= DEBOUNCE_MS)
                {
                    last_stable[i] = current;
                    last_change[i] = now;

                    // Fire event only on the falling edge (button pressed)
                    if (!current)
                    {
                        xQueueSend(g_btn_queue, &g_btn_events[i], 0);
                    }
                }
            }
            else
            {
                last_change[i] = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Poll at 100 Hz
    }
}

// ===========================================================================
// Task 3 - Display / menu logic
// Owns the SSD1306 frame buffer and all drawing. Consumes button events.
// ===========================================================================
static void task_display(void *pv)
{
    ssd1306_t disp;
    screen_index_t selected_item;
    menu_mode_t mode;
    btn_event_t event;
    lis3dh_data_t snapshot;
    char line[32];
    int i;
    int y;

    selected_item = SCREEN_HOME;
    mode = MODE_MENU;

    ssd1306_init(&disp, I2C_OLED_PORT);
    ssd1306_clear(&disp);
    ssd1306_send_frame(&disp, I2C_OLED_PORT);

    while (true)
    {
        // Process all pending button events before redrawing
        while (xQueueReceive(g_btn_queue, &event, 0) == pdTRUE)
        {
            if (mode == MODE_MENU)
            {
                switch (event)
                {
                case BTN_EVENT_UP:
                    if (selected_item > 0)
                        selected_item--;
                    break;
                case BTN_EVENT_DOWN:
                    if (selected_item < SCREEN_COUNT - 1)
                        selected_item++;
                    break;
                case BTN_EVENT_OK:
                    // Home screen ignores OK
                    if (selected_item != SCREEN_HOME)
                        mode = MODE_DETAIL;
                    break;
                }
            }
            else
            {
                // Any OK press returns to the menu list
                if (event == BTN_EVENT_OK)
                    mode = MODE_MENU;
                // UP/DOWN are ignored while a detail screen is open
            }
        }

        // Render the current state into the frame buffer
        ssd1306_clear(&disp);

        if (mode == MODE_MENU)
        {
            // 128x32 fits 3 rows at y = 0, 10, 20 (7px glyph + 3px gap)
            for (i = 0; i < SCREEN_COUNT; i++)
            {
                y = i * 10;
                if (i == (int)selected_item)
                {
                    ssd1306_draw_char(&disp, 0, y, '>');
                    ssd1306_draw_string(&disp, 8, y, g_menu_labels[i]);
                }
                else
                {
                    ssd1306_draw_string(&disp, 8, y, g_menu_labels[i]);
                }
            }
        }
        else
        {
            switch (selected_item)
            {

            case SCREEN_ACCEL:
                xSemaphoreTake(g_accel_mutex, portMAX_DELAY);
                snapshot = g_accel_data;
                xSemaphoreGive(g_accel_mutex);

                ssd1306_draw_string(&disp, 0, 0, "Accelerometer");
                ssd1306_draw_line_h(&disp, 0, 9, SSD1306_WIDTH);

                sprintf(line, "X:%-6d Y:%-6d", snapshot.x, snapshot.y);
                ssd1306_draw_string(&disp, 0, 12, line);

                sprintf(line, "Z:%-6d", snapshot.z);
                ssd1306_draw_string(&disp, 0, 22, line);
                break;

            case SCREEN_SYSINFO:
                ssd1306_draw_string(&disp, 0, 0, "System Info");
                ssd1306_draw_line_h(&disp, 0, 9, SSD1306_WIDTH);

                sprintf(line, "SDK %d.%d.%d",
                        PICO_SDK_VERSION_MAJOR,
                        PICO_SDK_VERSION_MINOR,
                        PICO_SDK_VERSION_REVISION);
                ssd1306_draw_string(&disp, 0, 14, line);
                break;

            default:
                break;
            }
        }

        ssd1306_send_frame(&disp, I2C_OLED_PORT);

        // Refresh at ~20 Hz; fast enough for dynamic accelerometer data
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
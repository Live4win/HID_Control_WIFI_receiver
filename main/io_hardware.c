/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "io_hardware.h"
#include "ws2812.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

//TODO: Must optimize here, only the first byte should be enough
uint8_t io_hardware_notify_data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

spi_device_handle_t spi;

static volatile uint8_t io_hardware_input_digital[GPIO_INPUT_NUMBER][2] = {
    {GPIO_INPUT_IO_0, 0}, // {GPIO_NUMBER, STATE}
    {GPIO_INPUT_IO_1, 0},
    {GPIO_INPUT_IO_2, 0},
    {GPIO_INPUT_IO_3, 0},
    {GPIO_INPUT_IO_4, 0}
    //{GPIO_INPUT_IO_5, 0}
};

/* trying to use ws2812-like rgb leds.. 
static uint8_t led_states[7] = { // the first led is either on or off
    LED_STATE_OFF,
    LED_STATE_OFF,
    LED_STATE_OFF,
    LED_STATE_OFF,
    LED_STATE_OFF,
    LED_STATE_OFF,
    LED_STATE_OFF
};
*/

rgbVal *pixels; // 7 leds
uint32_t pixels_states[NUMBER_OF_LEDS] = {
    0x000000, // 0xrrggbb
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
};

TimerHandle_t led_blink_timer;
static xQueueHandle external_tasks_evt_queue = NULL;

static uint8_t notify_code_received[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
// testing a sort of semaphore.. (not using it now)
static volatile uint8_t ledsInUse = 0;

// prototypes
static void led_blink_timer_callback(TimerHandle_t pxTimer);
static void ws2812_setRGBValue(rgbVal *pixel_to_set, uint8_t r, uint8_t g, uint8_t b);

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    static uint8_t firstTime = 1;
    uint8_t led_blink_activated = 0;
    uint8_t index_detected;
    uint8_t level_detected;
    uint8_t i;

    for (;;)
    {

        if (xQueueReceive(gpio_evt_queue, &io_num, 0))
        {
            //printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num)&1);

            level_detected = gpio_get_level(io_num);
            printf("level detected: %d, GPIO_NUM: %d\n", level_detected, io_num);

            for (i = 0; i < GPIO_INPUT_NUMBER; i++)
            {
                if (io_num == (unsigned int)io_hardware_input_digital[i][0])
                {
                    index_detected = i;
                    //printf("pin detected, index value: %d\n", (int)index_detected);
                    i = GPIO_INPUT_NUMBER; // stop the 'for' loop
                }
            }
            io_hardware_input_digital[index_detected][1] = level_detected;

            // maybe will add some control for standby buttons here..
        }

        // testing..

        if (xQueueReceive(io_hardware_get_queue(), &notify_code_received,
                          0))
        {
            //printf("Color received! 0x%06x\n", rgb(notify_code_received[2], notify_code_received[3], notify_code_received[4]));

            if ((notify_code_received[0]) == IO_HARDWARE_NOTIFY_BLE_DISCONNECT)
            {
                led_blink_activated = xTimerStart(led_blink_timer, 0);
                printf("BLE led blink started!\n");
            }
            else if (((notify_code_received[0]) == IO_HARDWARE_NOTIFY_BLE_CONNECT) &&
                     led_blink_activated)
            {
                xTimerStop(led_blink_timer, 0);
                led_blink_activated = 0;
                set_led_state(notify_code_received[1],
                              rgb(notify_code_received[2], notify_code_received[3], notify_code_received[4]));
                printf("BLE led blink stopped!\n");
            }
            else
            {
                set_led_state(notify_code_received[1],
                              rgb(notify_code_received[2], notify_code_received[3], notify_code_received[4]));
                printf("BLE led set to connected!\n");
            }
        }

        vTaskDelay(10); // some delay to let other tasks run smoothly..
    }
}

void io_hardware_setup()
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    //io_conf.pull_down_en = 0; /* may need adjustment */
    //enable pull-up mode
    //io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE; // was "GPIO_PIN_INTR_HILEVEL"
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //enable pull-down mode
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    //gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    external_tasks_evt_queue = xQueueCreate(5, sizeof(uint8_t) * 5);
    if (external_tasks_evt_queue == NULL)
    {
        printf("io hardware queue not set!\n");
    }

    // Initialize the RGB leds
    ws2812_init(RGB_LEDS_DATA_PIN);
    printf("Leds initialized!");

    pixels = malloc(sizeof(rgbVal) * NUMBER_OF_LEDS);
    printf("Pixels array allocated!\n");

    printf("Testing leds..\n");

    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        set_led_state(i, LED_STATE_RED);
        vTaskDelay((100) / portTICK_RATE_MS);
    }
    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        set_led_state(i, LED_STATE_GREEN);
        vTaskDelay((100) / portTICK_RATE_MS);
    }
    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        set_led_state(i, LED_STATE_BLUE);
        vTaskDelay((100) / portTICK_RATE_MS);
    }
    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        set_led_state(i, LED_STATE_OFF);
        vTaskDelay((100) / portTICK_RATE_MS);
    }

    printf("Test complete!\n");

    led_blink_timer = xTimerCreate("led_blink_timer",
                                   ((LED_BLINK_PERIOD_MS) / portTICK_RATE_MS),
                                   pdTRUE,
                                   (void *)1,
                                   led_blink_timer_callback);

    /* ###################### ATTENTION ##################################
    Don't call vTaskStartScheduler(), this function is called before app_main starts. 
    In fact, app_main runs within a FreeRTOS task already!
    */

    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void *)GPIO_INPUT_IO_1);

    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void *)GPIO_INPUT_IO_2);
    gpio_isr_handler_add(GPIO_INPUT_IO_3, gpio_isr_handler, (void *)GPIO_INPUT_IO_3);
    gpio_isr_handler_add(GPIO_INPUT_IO_4, gpio_isr_handler, (void *)GPIO_INPUT_IO_4);
    //gpio_isr_handler_add(GPIO_INPUT_IO_5, gpio_isr_handler, (void*) GPIO_INPUT_IO_5);

    // Notify initial BLE disconnection of the device..
    io_hardware_notify_data[0] = IO_HARDWARE_NOTIFY_BLE_DISCONNECT;
    io_hardware_notify_data[1] = IO_HARDWARE_BLE_LED; // use the last led
    io_hardware_notify_data[2] = LED_STATE_BLUE >> 16,
    io_hardware_notify_data[3] = (LED_STATE_BLUE >> 8) & 0xFF,
    io_hardware_notify_data[4] = LED_STATE_BLUE & 0xFF;

    xQueueSend(io_hardware_get_queue(), &io_hardware_notify_data, 0);

    /*gpio_set_direction(GPIO_OUTPUT_IO_1, GPIO_MODE_OUTPUT); // trying something new..
    gpio_set_direction(GPIO_OUTPUT_IO_0, GPIO_MODE_OUTPUT);*/

    /*
    esp_err_t ret;
    
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=2 // 2 bytes of data 
    };
    spi_device_interface_config_t devcfg={
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
#else
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
#endif
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=3,                          //We want to be able to queue 3 transactions at a time
        .pre_cb=NULL,                           //Specify pre-transfer callback 
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    
    ESP_ERROR_CHECK(ret);
    //Attach the led controllers (74HC595N shift registers) to the SPI bus
    //the registers will be in daisy chain 
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    */
    // commented for trying new things.. (ws2812-like leds)

    /*//remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);*/
}

xQueueHandle io_hardware_get_queue(void)
{
    return external_tasks_evt_queue;
}

uint8_t **io_hardware_get_digital_inputs(void)
{

    static uint8_t **return_matrix;
    static uint8_t firstTime = 1;
    uint8_t i;

    if (firstTime)
    {
        return_matrix = (uint8_t **)malloc(sizeof(uint8_t **) *
                                           GPIO_INPUT_NUMBER);
        for (i = 0; i < GPIO_INPUT_NUMBER; i++)
        {

            uint8_t *matrix_element = (uint8_t *)malloc(sizeof(uint8_t *) * 2);
            return_matrix[i] = matrix_element;
        }
        firstTime = 0;
    }

    for (i = 0; i < GPIO_INPUT_NUMBER; i++)
    {

        return_matrix[i][1] = io_hardware_input_digital[i][1];
    }

    return return_matrix;
}

static void led_blink_timer_callback(TimerHandle_t pxTimer)
{

    static uint8_t led_current_state = 0;

    //printf("led blink running.. 0x%02x%02x%02x%02x%02x \n", notify_code_received[0], notify_code_received[1],
    //                            notify_code_received[2], notify_code_received[3], notify_code_received[4]);

    if (!led_current_state)
    {
        set_led_state(notify_code_received[1],
                      rgb(notify_code_received[2], notify_code_received[3], notify_code_received[4]));

        led_current_state = 1;
    }
    else
    {
        set_led_state(notify_code_received[1],
                      LED_STATE_OFF);

        led_current_state = 0;
    }
}

void set_led_state(uint8_t led_number, uint32_t led_state_code)
{

    /* (this could work)
   if(ledsInUse){
    while(ledsInUse){
        // wait..
        printf("leds in use!\n");
    }
    ledsInUse = 1;
   }
   else {
       ledsInUse = 1;
   }
    */

    pixels_states[led_number] = led_state_code;

    for (int i = 0; i < NUMBER_OF_LEDS; i++)
    {
        ws2812_setRGBValue((rgbVal *)pixels + i, pixels_states[i] >> 16,
                           ((pixels_states[i] >> 8) & 0xFF),
                           pixels_states[i] & 0xFF);
    }

    ws2812_setColors(NUMBER_OF_LEDS, pixels);
}

static void ws2812_setRGBValue(rgbVal *pixel_to_set, uint8_t r, uint8_t g, uint8_t b)
{

    pixel_to_set->b = b;
    (*pixel_to_set).r = g; // yeah.. but it works like this for some reason
    (*pixel_to_set).g = r; // yeah.. but it works like this for some reason
}

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{

    uint32_t result;

    result = (uint32_t)((r << 16) | (g << 8) | (b));
    //printf("rgb value: %d\n", result);

    return result;
}

uint32_t rbg_with_brightness(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{

    float newRed, newGreen, newBlue;

    newRed = r * ((float)brightness / 100);
    newGreen = g * ((float)brightness / 100);
    newBlue = b * ((float)brightness / 100);

    // for debugging
    //printf("%d | %d | %d\n", (int)newRed, (int)newGreen, (int)newBlue);

    return rgb((uint8_t)newRed, (uint8_t)newGreen, (uint8_t)newBlue);
}
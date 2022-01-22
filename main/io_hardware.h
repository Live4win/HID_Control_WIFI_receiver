#ifndef IO_HARDWARE_H
#define IO_HARDWARE_H

#ifdef __cplusplus
extern "C"
{
#endif

#define GPIO_INPUT_NUMBER 5  // number of digital inputs
#define GPIO_OUTPUT_NUMBER 2 // number of digital outputs

#define GPIO_OUTPUT_IO_0 21
#define GPIO_OUTPUT_IO_1 22
#define GPIO_OUTPUT_IO_2 17
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1) | \
                             (1ULL << GPIO_OUTPUT_IO_2))

#define GPIO_INPUT_IO_0 12
#define GPIO_INPUT_IO_1 13 // was 13
#define GPIO_INPUT_IO_2 14
#define GPIO_INPUT_IO_3 25
#define GPIO_INPUT_IO_4 26
#define GPIO_INPUT_IO_5 27

#define GPIO_INPUT_PIN_SEL          \
    (                               \
        (1ULL << GPIO_INPUT_IO_0) | \
        (1ULL << GPIO_INPUT_IO_1) | \
        (1ULL << GPIO_INPUT_IO_2) | \
        (1ULL << GPIO_INPUT_IO_3) | \
        (1ULL << GPIO_INPUT_IO_4) | \
        (1ULL << GPIO_INPUT_IO_5))

// for ws2812-like leds
#define RGB_LEDS_DATA_PIN 16

#define NUMBER_OF_LEDS 7

// All with 15% brightness for now (otherwise they're too bright)
#define LED_STATE_OFF rbg_with_brightness(0, 0, 0, 15)
#define LED_STATE_RED rbg_with_brightness(255, 0, 0, 15)
#define LED_STATE_GREEN rbg_with_brightness(0, 255, 0, 15)
#define LED_STATE_BLUE rbg_with_brightness(0, 0, 255, 15)
#define LED_STATE_YELLOW rbg_with_brightness(255, 255, 0, 15)
#define LED_STATE_CYAN rbg_with_brightness(0, 255, 255, 15)
#define LED_STATE_GREY rbg_with_brightness(128, 128, 128, 15)
#define LED_STATE_WHITE rbg_with_brightness(255, 255, 255, 15)
#define LED_SPECIAL_STATE_ON rbg_with_brightness(255, 255, 255, 15)
#define LED_SPECIAL_STATE_OFF rbg_with_brightness(0, 0, 0, 100)

// Set blinking period for blinking activties
#define LED_BLINK_PERIOD_MS 500

#define IO_HARDWARE_WIFI_LED 5
#define IO_HARDWARE_BLE_LED 6

// codes for notifying ble disconnection to the
// io hardware related task so it can start
// blinking the related led
#define IO_HARDWARE_NOTIFY_BLE_DISCONNECT 0x00
#define IO_HARDWARE_NOTIFY_BLE_CONNECT 0x01

    // this queue must be accessible from all other tasks
    //extern xQueueHandle external_tasks_evt_queue; // disabled here for now

    // the notify data to send to the io hardware related task
    // ble state, led number, led color
    extern uint8_t io_hardware_notify_data[5];

    void io_hardware_setup();
    xQueueHandle io_hardware_get_queue(void);
    uint8_t **io_hardware_get_digital_inputs(void);
    void set_led_state(uint8_t led_number, uint32_t led_state_code);
    uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
    uint32_t rbg_with_brightness(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* IO_HARDWARE_H */
/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "hid_dev.h"

#include "ble_hid_app.h"
#include "hid_app_control.h"
#include "io_hardware.h"

/**
 * Brief:
 * This example Implemented BLE HID device profile related functions, in which the HID device
 * has 4 Reports (1 is mouse, 2 is keyboard and LED, 3 is Consumer Devices, 4 is Vendor devices).
 * Users can choose different reports according to their own application scenarios.
 * BLE HID profile inheritance and USB HID class.
 */

/**
 * Note:
 * 1. Win10 does not support vendor report , So SUPPORT_REPORT_VENDOR is always set to FALSE, it defines in hidd_le_prf_int.h
 * 2. Update connection parameters are not allowed during iPhone HID encryption, slave turns
 * off the ability to automatically update connection parameters during encryption.
 * 3. After our HID device is connected, the iPhones write 1 to the Report Characteristic Configuration Descriptor,
 * even if the HID encryption is not completed. This should actually be written 1 after the HID encryption is completed.
 * we modify the permissions of the Report Characteristic Configuration Descriptor to `ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED`.
 * if you got `GATT_INSUF_ENCRYPTION` error, please ignore.
 */

#define HID_DEMO_TAG "HID_TASK"

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
static bool send_volum_up = false;
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define HIDD_DEVICE_NAME "Scatola Comandi"
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0x12,
    0x18,
    0x00,
    0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x00F0, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,   //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x30,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// for notifying the i/o harware related task
//static uint8_t *notify_result;

// Global variables for the app control implementation
app_control_struct_t *zoom_control_mobile;
app_control_struct_t *zoom_control_pc;
app_control_struct_t *skype_control_mobile;
app_control_struct_t *skype_control_pc;
app_control_struct_t *app_control_registered[CONTROL_SCRIPTS_SETS];

// Global variable that relations the app_control implementation
// with the I/O hardare management
uint8_t app_control_io_hardware_scripts[CONTROL_SCRIPTS_SETS][GPIO_INPUT_NUMBER][2] =
    {
        {
            // Zoom control mobile
            {GPIO_INPUT_IO_0, ZOOM_CONTROL_MOBILE_TOGGLE_MIC},
            {GPIO_INPUT_IO_1, ZOOM_CONTROL_MOBILE_TOGGLE_VID},
            {GPIO_INPUT_IO_2, ZOOM_CONTROL_MOBILE_JOIN1},
            {GPIO_INPUT_IO_3, ZOOM_CONTROL_MOBILE_OPEN_APP},
            {GPIO_INPUT_IO_4, ZOOM_CONTROL_MOBILE_STILL_DECIDING},
            //{GPIO_INPUT_IO_5, ZOOM_CONTROL_MOBILE_STILL_DECIDING}
        },

        {
            // Zoom control pc
            {GPIO_INPUT_IO_0, ZOOM_CONTROL_PC_TOGGLE_MIC},
            {GPIO_INPUT_IO_1, ZOOM_CONTROL_PC_TOGGLE_VID},
            {GPIO_INPUT_IO_2, ZOOM_CONTROL_PC_IMPROV_VID},
            {GPIO_INPUT_IO_3, ZOOM_CONTROL_PC_OPEN_APP},
            {GPIO_INPUT_IO_4, ZOOM_CONTROL_PC_STILL_DECIDING},
            //{GPIO_INPUT_IO_5, ZOOM_CONTROL_PC_STILL_DECIDING}
        },

        {
            // Skype control mobile
            {GPIO_INPUT_IO_0, SKYPE_CONTROL_MOBILE_TOGGLE_MIC},
            {GPIO_INPUT_IO_1, SKYPE_CONTROL_MOBILE_TOGGLE_VID},
            {GPIO_INPUT_IO_2, SKYPE_CONTROL_MOBILE_IMPROV_VID},
            {GPIO_INPUT_IO_3, SKYPE_CONTROL_MOBILE_OPEN_APP},
            {GPIO_INPUT_IO_4, SKYPE_CONTROL_MOBILE_STILL_DECIDING},
            //{GPIO_INPUT_IO_5, SKYPE_CONTROL_MOBILE_STILL_DECIDING}
        },

        {
            // Skype control pc
            {GPIO_INPUT_IO_0, SKYPE_CONTROL_PC_TOGGLE_MIC},
            {GPIO_INPUT_IO_1, SKYPE_CONTROL_PC_TOGGLE_VID},
            {GPIO_INPUT_IO_2, SKYPE_CONTROL_PC_IMPROV_VID},
            {GPIO_INPUT_IO_3, SKYPE_CONTROL_PC_OPEN_APP},
            {GPIO_INPUT_IO_4, SKYPE_CONTROL_PC_STILL_DECIDING},
            //{GPIO_INPUT_IO_5, SKYPE_CONTROL_PC_STILL_DECIDING}
        }};

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{

    uint8_t result; // for debugging

    switch (event)
    {
    case ESP_HIDD_EVENT_REG_FINISH:
    {
        if (param->init_finish.state == ESP_HIDD_INIT_OK)
        {
            //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
            esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
            esp_ble_gap_config_adv_data(&hidd_adv_data);
        }
        break;
    }
    case ESP_BAT_EVENT_REG:
    {
        break;
    }
    case ESP_HIDD_EVENT_DEINIT_FINISH:
        break;
    case ESP_HIDD_EVENT_BLE_CONNECT:
    {
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
        hid_conn_id = param->connect.conn_id;
        //TODO: Must optimize here, sending only the first byte should be enough
        io_hardware_notify_data[0] = IO_HARDWARE_NOTIFY_BLE_CONNECT;
        io_hardware_notify_data[1] = IO_HARDWARE_BLE_LED; // use the last led
        io_hardware_notify_data[2] = LED_STATE_BLUE >> 16,
        io_hardware_notify_data[3] = (LED_STATE_BLUE >> 8) & 0xFF,
        io_hardware_notify_data[4] = LED_STATE_BLUE & 0xFF;

        result = xQueueSend(io_hardware_get_queue(), &io_hardware_notify_data, 0);
        //notify_result = io_hardware_notify_data;
        printf("notifying hardware.. result: %d\n", result);
        //return_val = xTaskGetTickCount() * portTICK_RATE_MS;

        //printf("return val: %d\n", return_val);
        break;
    }
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
    {
        sec_conn = false;
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
        esp_ble_gap_start_advertising(&hidd_adv_params);
        //TODO: Must optimize here, sending only the first byte should be enough
        io_hardware_notify_data[0] = IO_HARDWARE_NOTIFY_BLE_DISCONNECT;
        io_hardware_notify_data[1] = IO_HARDWARE_BLE_LED; // use the last led
        io_hardware_notify_data[2] = LED_STATE_BLUE >> 16,
        io_hardware_notify_data[3] = (LED_STATE_BLUE >> 8) & 0xFF,
        io_hardware_notify_data[4] = LED_STATE_BLUE & 0xFF;
        //return_val = xTaskGetTickCount() * portTICK_RATE_MS;
        result = xQueueSend(io_hardware_get_queue(), &io_hardware_notify_data, 0);
        printf("notifying hardware.. result: %d\n", result);
        //xQueueSend(external_tasks_evt_queue/*io_hardware_get_queue()*/, &notify_result, 0);

        //printf("return val: %d\n", return_val);
        break;
    }
    case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT:
    {
        ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
        ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
    }
    default:
        break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        for (int i = 0; i < ESP_BD_ADDR_LEN; i++)
        {
            ESP_LOGD(HID_DEMO_TAG, "%x:", param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success)
        {
            ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

void hid_demo_task(void *pvParameters)
{

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //static uint32_t prev_notify_result = 0;
    uint8_t result;                     // for debugging
    uint8_t user_app_selection = 0;     // default app selection value
    uint8_t user_command_selection = 1; // default command selection value
    uint8_t **button_inputs;            // variable where the current inputs values are stored
    uint8_t command_selected = 0;
    uint8_t mouse_button_value = 0;
    uint8_t i, k;

    while (1)
    {

        printf("hid_task is executing!\n");
        //vTaskDelay(1000);
        while (!command_selected)
        {
            vTaskDelay(10); // put this to let other tasks run correctly

            // check if should notify the io related task of something..
            /*if(notify_result != prev_notify_result){
                result = xQueueSend(io_hardware_get_queue(), &notify_result, 0);
                printf("notifying hardware.. result: %d\n", result);
                prev_notify_result = notify_result;
            }*/

            button_inputs = io_hardware_get_digital_inputs();
            /*printf("Printing input matrix..\n");
            for(i = 0; i < GPIO_INPUT_NUMBER; i++){
                printf("GPIO NUMBER: %d, STATE DETECTED: %d\n", 
                        (int)button_inputs[i][0], (int)button_inputs[i][1]);
            }*/

            // check all GPIO
            for (i = 0; i < GPIO_INPUT_NUMBER; i++)
            {

                if (button_inputs[i][1] == 1)
                { // if a specific button is pressed
                    user_command_selection = (app_control_io_hardware_scripts[user_app_selection][i][1]);
                    i = GPIO_INPUT_NUMBER; // end the for loop
                    command_selected = 1;
                    printf("Command Found!\n");
                }
            }
        }

        uint8_t key_value = 0;
        uint8_t key_combo_flag = 1; // 1 must be the default value

        for (i = 1;
             i < app_control_registered[user_app_selection]->scripts_max_steps;
             i++)
        {
            key_value = app_control_registered[user_app_selection]->scripts[user_command_selection][i];
            switch (key_value)
            {
            case ACTION_NONE:
                i = app_control_registered[user_app_selection]->scripts_max_steps;
                break;

            case ACTION_SPECIAL:
                printf("Special Action Detected!\n");
                key_value = app_control_registered[user_app_selection]
                                ->scripts[user_command_selection][i + 1]; // get the special function index
                app_control_special_actions[user_app_selection][key_value].host_script = app_control_registered[user_app_selection]
                                                                                             ->scripts[user_command_selection];
                app_control_special_actions[user_app_selection][key_value].hid_conn_id = hid_conn_id;
                app_control_special_actions[user_app_selection][key_value]
                    .pfunction(&app_control_special_actions[user_app_selection][key_value]);

                switch (app_control_special_actions[user_app_selection][key_value].returnCode[0])
                {
                case SPECIAL_ACTION_RETURN_CODE_END_SCRIPT:
                    i = app_control_registered[user_app_selection]->scripts_max_steps;
                    break;
                case SPECIAL_ACTION_RETURN_CODE_FAIL:
                    i = app_control_registered[user_app_selection]->scripts_max_steps;
                    break;
                case SPECIAL_ACTION_RETURN_CODE_SKIP_NEXT:
                    break;
                default:
                    break;
                }

                break;

            default:

                printf("Executing command\n");

                if ((int)(key_value / 10) == (ACTION_COMBINE_KEYS_BASE_CODE / 10))
                {
                    i++;
                    for (int kj = 0; kj < 2; kj++)
                    {
                        for (int j = 0; j < (key_value - ACTION_COMBINE_KEYS_BASE_CODE); j++)
                        {
                            esp_hidd_send_keyboard_value(
                                hid_conn_id, 0,
                                app_control_registered[user_app_selection]->scripts[user_command_selection][i + j], key_combo_flag);
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                        }
                        key_combo_flag = ~key_combo_flag & 1;
                    }
                    i += (key_value - ACTION_COMBINE_KEYS_BASE_CODE - 1);
                    key_combo_flag = 1;
                    break;
                }

                if (key_value > 250)
                {
                    mouse_button_value = (key_value == 253) ? 0x01 : 0x02;
                    // Simulate a short 'click'
                    esp_hidd_send_mouse_value(hid_conn_id, mouse_button_value, 0, 0);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    esp_hidd_send_mouse_value(hid_conn_id, 0x00, 0, 0);
                }
                else
                {
                    esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_value, 1);
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_value, 0);
                }
                break;
            }

            command_selected = 0;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

// This functions will setup the BLE functionalities and it's task. Remember to
// MAYBE to run the start the task scheduler to run the BLE task after calling this function
void ble_app_setup()
{
    esp_err_t ret;

    // Initialize NVS.
    /*ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );*/

    io_hardware_setup();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    if ((ret = esp_hidd_profile_init()) != ESP_OK)
    {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
    }

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND; //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;       //set the IO capability to No output No input
    uint8_t key_size = 16;                          //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    app_control_init(app_control_registered);

    xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 7, NULL);
}

void app_control_init(app_control_struct_t **app_control_register)
{
    zoom_control_mobile = app_control_setup_new(ZOOM_CONTROL_MOBILE_ID,
                                                ZOOM_CONTROL_MOBILE_NUM_SCRIPTS,
                                                ZOOM_CONTROL_MAX_STEPS,

                                                ZOOM_CONTROL_MOBILE_TOGGLE_MIC_SCRIPT,
                                                ZOOM_CONTROL_MOBILE_TOGGLE_VID_SCRIPT,
                                                ZOOM_CONTROL_MOBILE_JOIN1_SCRIPT,
                                                ZOOM_CONTROL_MOBILE_OPEN_APP_SCRIPT,
                                                ZOOM_CONTROL_MOBILE_STILL_DECIDING_SCRIPT);

    zoom_control_pc = app_control_setup_new(ZOOM_CONTROL_PC_ID,
                                            ZOOM_CONTROL_PC_NUM_SCRIPTS,
                                            ZOOM_CONTROL_MAX_STEPS,

                                            ZOOM_CONTROL_PC_TOGGLE_MIC_SCRIPT,
                                            ZOOM_CONTROL_PC_TOGGLE_VID_SCRIPT,
                                            ZOOM_CONTROL_PC_IMPROV_VID_SCRIPT,
                                            ZOOM_CONTROL_PC_OPEN_APP_SCRIPT,
                                            ZOOM_CONTROL_PC_STILL_DECIDING_SCRIPT);

    skype_control_mobile = app_control_setup_new(SKYPE_CONTROL_MOBILE_ID,
                                                 SKYPE_CONTROL_MOBILE_NUM_SCRIPTS,
                                                 SKYPE_CONTROL_MAX_STEPS,

                                                 SKYPE_CONTROL_MOBILE_TOGGLE_MIC_SCRIPT,
                                                 SKYPE_CONTROL_MOBILE_TOGGLE_VID_SCRIPT,
                                                 SKYPE_CONTROL_MOBILE_IMPROV_VID_SCRIPT,
                                                 SKYPE_CONTROL_MOBILE_OPEN_APP_SCRIPT,
                                                 SKYPE_CONTROL_MOBILE_STILL_DECIDING_SCRIPT);

    skype_control_pc = app_control_setup_new(SKYPE_CONTROL_PC_ID,
                                             SKYPE_CONTROL_PC_NUM_SCRIPTS,
                                             SKYPE_CONTROL_MAX_STEPS,

                                             SKYPE_CONTROL_PC_TOGGLE_MIC_SCRIPT,
                                             SKYPE_CONTROL_PC_TOGGLE_VID_SCRIPT,
                                             SKYPE_CONTROL_PC_IMPROV_VID_SCRIPT,
                                             SKYPE_CONTROL_PC_OPEN_APP_SCRIPT,
                                             SKYPE_CONTROL_PC_STILL_DECIDING_SCRIPT);

    app_control_register[0] = zoom_control_mobile;
    app_control_register[1] = zoom_control_pc;
    app_control_register[2] = skype_control_mobile;
    app_control_register[3] = skype_control_pc;
}

app_control_struct_t *app_control_setup_new(uint8_t app_id,
                                            uint8_t num_of_scripts,
                                            uint8_t max_steps_per_script, ...)
{
    static app_control_struct_t available_structs[CONTROL_SCRIPTS_MAX_SETS];
    static uint8_t current_available_index = 0;
    int i, k;

    if (current_available_index >= CONTROL_SCRIPTS_MAX_SETS)
    {
        return 0; // return a null pointer
    }

    command_code_t **scripts_commands;
    scripts_commands = (command_code_t **)malloc(sizeof(command_code_t **) *
                                                 num_of_scripts);

    for (i = 0; i < num_of_scripts; i++)
    {

        command_code_t *scripts_commands_steps; // + 1 below for the id of the command
        scripts_commands_steps = (command_code_t *)malloc(sizeof(command_code_t) *
                                                          (max_steps_per_script + 1));

        scripts_commands[i] = scripts_commands_steps;
    }

    available_structs[current_available_index].app_control_id = app_id;
    available_structs[current_available_index].num_of_scripts = num_of_scripts;
    available_structs[current_available_index].scripts = scripts_commands;
    available_structs[current_available_index].scripts_max_steps = max_steps_per_script;

    va_list commands_codes;
    va_start(commands_codes, (int)max_steps_per_script);

    for (i = 0; i < num_of_scripts; i++)
    {
        ;
        for (k = 0; k < max_steps_per_script + 1; k++)
        {

            available_structs[current_available_index].scripts[i][k] = ((command_code_t)va_arg(commands_codes, int));
        }
    }

    va_end(commands_codes);
    current_available_index++;

    return &(available_structs[current_available_index - 1]);
}

app_control_struct_t **app_control_register_new(uint8_t num_of_apps_registered, ...)
{
    uint8_t i;
    app_control_struct_t **app_control_register;

    app_control_register = ((app_control_struct_t **)malloc(sizeof(app_control_struct_t **) *
                                                            num_of_apps_registered));

    va_list apps_to_register;

    va_start(apps_to_register, (int)num_of_apps_registered);

    for (i = 0; i < num_of_apps_registered; i++)
    {

        /*app_control_struct_t *pApp_control_to_register;
        pApp_control_to_register = (
            (app_control_struct_t *)malloc(sizeof(app_control_struct_t *))
        );
        
        app_control_register[i] = pApp_control_to_register;*/
        app_control_register[i] = va_arg(apps_to_register, app_control_struct_t *);
    }

    va_end(apps_to_register);

    return app_control_register;
}

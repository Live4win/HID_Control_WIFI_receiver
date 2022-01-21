/*
 * This file containis all the implementations of the special
 * scripts declared in the .h file 
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // include to use variable arguments

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
#include "hid_dev.h"
#include "hid_app_control.h"

// LOCAL FUNCTIONS PROTOTYPES

static void type_and_connect_meeting(void *pData);
static void type_and_connect_meeting2(void *pData); // for testing
static char *convert_ASCII_to_HID(char *string, uint8_t lenght);
static void type_HID_text(char *string, uint8_t lenght, uint16_t hid_conn_id);

// GLOBAL VARIBLES

// setting up zoom mobile's special scripts (only one for now)
static app_control_special_script_t
    zoom_mobile_special_scripts[ZOOM_CONTROL_MOBILE_SPECIAL_ACTIONS] =
        {
            {.pfunction = (void *)type_and_connect_meeting,
             .arg1 = (uint8_t *)MEETING1_ID,
             .arg2 = (uint8_t *)MEETING1_PASSCODE},
            {.pfunction = (void *)type_and_connect_meeting2,
             .arg1 = (uint8_t *)MEETING1_ID,
             .arg2 = (uint8_t *)MEETING1_PASSCODE}};

static app_control_special_script_t zoom_pc_special_scripts[ZOOM_CONTROL_PC_SPECIAL_ACTIONS] = {
    {.pfunction = (void *)send_keyboard_shortcut,
     .arg1 = 123,
     .arg2 = 456,
     .returnCode = {[0] = 0xff, [1] = 0xff, [2] = 0xff}}};

// registering the special scripts of zoom mobile only
app_control_special_script_t *
    app_control_special_actions[CONTROL_SCRIPTS_SPECIAL_ACTIONS_TOTAL] = {
        (void *)&zoom_mobile_special_scripts, // zoom mobile
        (void *)&zoom_pc_special_scripts,     // zoom pc
};

// LOCAL FUNCTION DEFINITIONS

static void type_and_connect_meeting(void *pData)
{

    app_control_special_script_t *received = (app_control_special_script_t *)pData;

    uint16_t hid_id = (uint16_t)received->hid_conn_id;
    // these two will be treated like matrixes! [char_code, shift_key_bit]
    char *meeting_id = malloc(sizeof(char) * strlen((char *)received->arg1) * 2);
    char *meeting_passcode = malloc(sizeof(char) * strlen((char *)received->arg2) * 2);

    strcpy(meeting_id, (char *)received->arg1);
    strcpy(meeting_passcode, (char *)received->arg2);

    type_HID_text(meeting_id, strlen((char *)received->arg1), hid_id);
    printf("---------------------------\n");
    type_HID_text(meeting_passcode, strlen((char *)received->arg2), hid_id);

    free(meeting_id);
    free(meeting_passcode);

    received->returnCode = ;
}

static void type_and_connect_meeting2(void *pData)
{

    app_control_special_script_t *received = (app_control_special_script_t *)pData;

    char *meeting_id = (char *)received->arg1;
    char *meeting_passcode = (char *)received->arg2;

    printf("%s, %s\n", meeting_id, meeting_passcode);
    printf("DoSFDSDFSDFSne!\n");
}

// zoom pc special scripts functions
static void send_keyboard_shortcut(void *pData)
{
    app_control_special_script_t *received = (app_control_special_script_t *)pData;

    command_code_t *hostScript = received->host_script;

    bool special_code_found = false;

    // TODO: Must find out how to comunicate & interpret a shorcut's data
}

// general purpose functions used by special scripts
// WARNING: string parameter is treated as a 2d array
static void type_HID_text(char *string, uint8_t lenght, uint16_t hid_conn_id)
{

    char shift_key_prev = 0; // treated like a bool value
    uint8_t index = 0;
    uint8_t key_value = 0;
    uint8_t key_mask = 0;

    convert_ASCII_to_HID(string, lenght);

    printf("%s || %d\n", string, lenght);

    printf("Disabling caps lock..\n");
    key_value = HID_KEY_CAPS_LOCK;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_value, 0);

    for (int i = 0; i < lenght; i++)
    {

        index = (1 * lenght) + i;

        if (string[index])
        {
            if (!shift_key_prev)
            { // must press shift
                printf("SHIFT KEY PRESSED!\n");
                key_mask = LEFT_SHIFT_KEY_MASK;
                shift_key_prev = 1;
            }
            // leave as it is (with shift pressed)
        }
        else
        {
            if (shift_key_prev)
            { // must release shift
                printf("SHIFT KEY UNPRESSED!\n");
                key_mask = 0x00;
                shift_key_prev = 0;
            }
            // leave as it is (with shift unpressed)
        }
        index = (0 * lenght) + i;
        printf("'Executing' this value: %d\n", string[index]);
        key_value = string[index];
        esp_hidd_send_keyboard_value(hid_conn_id, key_mask, &key_value, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_hidd_send_keyboard_value(hid_conn_id, key_mask, &key_value, 0);
    }
}

// WARNING: string parameter is treated as a 2d array
static char *convert_ASCII_to_HID(char *string, uint8_t lenght)
{

    // the function will treat the string array like a matrix
    // with one dimension being 2 and the other being 'lenght'

    uint8_t index = 0;

    for (int i = 0; i < lenght; i++)
    { // for each row..

        index = (0 * lenght) + i; // (row_selected * cols_total) + col_selected

        if ((string[index] >= 'a' && string[index] <= 'z') ||
            (string[index] >= 'A' && string[index] <= 'Z'))
        {
            // shift key bits row first, so that we can check the char before it
            // gets converted
            string[(1 * lenght) + i] = (string[index] >= 'a') ? 0x00 : 0x01;
            string[index] -= ((string[index] >= 'a') ? 0x5D : 0x3D);
        }
        else if (string[index] >= '0' && string[index] <= '9')
        {
            // writing on the second row
            string[(1 * lenght) + i] = 0x00;
            string[index] -= ((string[index] == '0') ? 0x09 : 0x13);
        }
        else
        {
            // There's a special character!
        }
    }

    return string;
}
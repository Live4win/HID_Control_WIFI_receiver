/*
 * This file contains the various definitions for 
 * the implementation of the control of video conferencing 
 * applications like ZOOM and Skype.
 * There are a couple of functions which need to be defined
 * in the .c file that's has this header file defined 
 * 
 * ########################################################################
 * ##################### SETUP 'n' NEW APP CONTROL(s) #####################
 * ########################################################################
 * 
 * 0. HERE IN THIS HEADER FILE, WITH THE DEFINES:
 * 
 * 1. CONTROL_SCRIPTS_SETS must be updated by adding 'n'
 * 2. Define 'n' new consecutive IDs with the siffix '_ID'
 * 3. Define the number of scripts for each 'n', and define the 
 *    max steps for each script in every 'n' (you can make a shared define)
 * 4. Define the number of special actions for each 'n'
 * 5. Add the defined number of special actions to the sum of special actions
 *    in 'CONTROL_SCRIPTS_SPECIAL_ACTIONS_TOTAL'
 * 6. If there are, define the special scripts indexes for each 'n'
 * 7. Near the end of this file, make a typedef enum with the names of the 
 *    scripts available for each 'n' (eg. zoom_mobile_scripts_names_available_t)
 * 8. Define ALL the scripts, with the right number of steps. You have to
 *    follow the scheme found in the other scripts definitions for the
 *    already present app controls
 * 
 * 9. INSIDE THE ble_hidd_demo_main.c FILE!
 * 
 * 10. Define 'n' global variables for the app control imlementation (app_control_struct_t *)
 *     you can define them before the 'app_control_registered' array
 * 11. Add 'n' arrays of button-script relationships inside the 
 *     app_control_io_hardware_scripts 3d array
 * 12. Add 'n' RGB color codes in the app_control_rgb_codes array
 * 13. Add 'n' initializations of the global variables in step 10 inside the
 *     app_control_init function definition using the 'app_control_setup_new()'
 *     function. Remember to put all the correct parameters! See already
 *     present definitions for reference!
 * 14. Add 'n' initializations of 'app_control_registered' array elements 
 *     (mentioned id step 10) by following the correct indexing order after
 *     the already present initializations
 * 
 * 15. Make a final check for all steps, and you should be ready to go :)
 * 
 * ##########################################################################
 */

/* 
 * File:   HID_app_control.h
 * Author: Emmanuel Baah
 *
 * Created on 2 giugno 2020, 15.14
 */

#ifndef HID_APP_CONTROL_H
#define HID_APP_CONTROL_H

#ifdef __cplusplus
extern "C"
{
#endif

    // INCLUDES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "hid_dev.h"

    // DEFINES

#define CONTROL_SCRIPTS_MAX_SETS 10
#define CONTROL_SCRIPTS_SETS 6 // usually 2 for every app
#define CONTROL_SCRIPTS_NUM_ATTRIBUTES 3

#define ZOOM_CONTROL_MOBILE_ID 0
#define ZOOM_CONTROL_PC_ID 1
#define SKYPE_CONTROL_MOBILE_ID 2
#define SKYPE_CONTROL_PC_ID 3
#define MEET_CONTROL_MOBILE_ID 4 // Google Meet mobile
#define MEET_CONTROL_PC_ID 5     // Google Meet PC (browser)

#define ZOOM_CONTROL_MOBILE_NUM_SCRIPTS 5
#define ZOOM_CONTROL_PC_NUM_SCRIPTS 5
#define ZOOM_CONTROL_MAX_STEPS 10

#define SKYPE_CONTROL_MOBILE_NUM_SCRIPTS 5
#define SKYPE_CONTROL_PC_NUM_SCRIPTS 5
#define SKYPE_CONTROL_MAX_STEPS 10

#define MEET_CONTROL_MOBILE_NUM_SCRIPTS 5
#define MEET_CONTROL_PC_NUM_SCRIPTS 5
#define MEET_CONTROL_MAX_STEPS 10

#define ZOOM_CONTROL_MOBILE_SPECIAL_ACTIONS 2
#define ZOOM_CONTROL_PC_SPECIAL_ACTIONS 0
#define SKYPE_CONTROL_MOBILE_SPECIAL_ACTIONS 0
#define SKYPE_CONTROL_PC_SPECIAL_ACTIONS 0
#define MEET_CONTROL_MOBILE_SPECIAL_ACTIONS 0
#define MEET_CONTROL_PC_SPECIAL_ACTIONS 0

#define CONTROL_SCRIPTS_MAX_SPECIAL_ACTIONS 1

#define CONTROL_SCRIPTS_SPECIAL_ACTIONS_TOTAL  \
    ZOOM_CONTROL_MOBILE_SPECIAL_ACTIONS +      \
        ZOOM_CONTROL_PC_SPECIAL_ACTIONS +      \
        SKYPE_CONTROL_MOBILE_SPECIAL_ACTIONS + \
        SKYPE_CONTROL_PC_SPECIAL_ACTIONS +     \
        MEET_CONTROL_MOBILE_SPECIAL_ACTIONS +  \
        MEET_CONTROL_PC_SPECIAL_ACTIONS

#define ACTION_NONE 0
#define ACTION_SPECIAL 232
#define ACTION_COMBINE_KEYS_BASE_CODE 240
    // 'n' must not be higher than 9
#define ACTION_COMBINE_NEXT_KEYS(n) ((n < 2 || n > 9)  \
                                         ? ACTION_NONE \
                                         : ACTION_COMBINE_KEYS_BASE_CODE + n)

#define ZOOM_CONTROL_MOBILE_SPECIAL_1 0
#define CONTROL_PC_SPECIAL_KEY_COMBINATION 1

// Zoom for mobile scripts
#define ZOOM_CONTROL_MOBILE_TOGGLE_MIC_SCRIPT \
    ZOOM_CONTROL_MOBILE_TOGGLE_MIC,           \
        HID_MOUSE_LEFT,                       \
        HID_KEY_DOWN_ARROW,                   \
        HID_KEY_SPACEBAR,                     \
        HID_KEY_ESCAPE,                       \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

#define ZOOM_CONTROL_MOBILE_TOGGLE_VID_SCRIPT \
    ZOOM_CONTROL_MOBILE_TOGGLE_VID,           \
        HID_MOUSE_LEFT,                       \
        HID_KEY_DOWN_ARROW,                   \
        HID_KEY_RIGHT_ARROW,                  \
        HID_KEY_SPACEBAR,                     \
        HID_KEY_ESCAPE,                       \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

#define ZOOM_CONTROL_MOBILE_JOIN1_SCRIPT \
    ZOOM_CONTROL_MOBILE_JOIN1,           \
        ACTION_SPECIAL,                  \
        ZOOM_CONTROL_MOBILE_SPECIAL_1,   \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE

// THIS SCRIPT BELOW WILL PROBABLY GET SUBSTITUTED BY
// A SCRIPT THAT GOES 'BACK TO THE MEETING' IF THE
// USER IS ON THE ZOOM START PAGE
#define ZOOM_CONTROL_MOBILE_OPEN_APP_SCRIPT \
    ZOOM_CONTROL_MOBILE_OPEN_APP,           \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE

#define ZOOM_CONTROL_MOBILE_STILL_DECIDING_SCRIPT \
    ZOOM_CONTROL_MOBILE_STILL_DECIDING,           \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE

// Zoom for PC scripts
#define ZOOM_CONTROL_PC_TOGGLE_MIC_SCRIPT \
    ZOOM_CONTROL_PC_TOGGLE_MIC,           \
        ACTION_COMBINE_NEXT_KEYS(2),      \
        HID_KEY_LEFT_ALT,                 \
        HID_KEY_A,                        \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define ZOOM_CONTROL_PC_TOGGLE_VID_SCRIPT \
    ZOOM_CONTROL_PC_TOGGLE_VID,           \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define ZOOM_CONTROL_PC_IMPROV_VID_SCRIPT \
    ZOOM_CONTROL_PC_IMPROV_VID,           \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define ZOOM_CONTROL_PC_OPEN_APP_SCRIPT \
    ZOOM_CONTROL_PC_OPEN_APP,           \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE

#define ZOOM_CONTROL_PC_STILL_DECIDING_SCRIPT \
    ZOOM_CONTROL_PC_STILL_DECIDING,           \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

// Skype for mobile scripts
#define SKYPE_CONTROL_MOBILE_TOGGLE_MIC_SCRIPT \
    SKYPE_CONTROL_MOBILE_TOGGLE_MIC,           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE

#define SKYPE_CONTROL_MOBILE_TOGGLE_VID_SCRIPT \
    SKYPE_CONTROL_MOBILE_TOGGLE_VID,           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE

#define SKYPE_CONTROL_MOBILE_IMPROV_VID_SCRIPT \
    SKYPE_CONTROL_MOBILE_IMPROV_VID,           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE

#define SKYPE_CONTROL_MOBILE_OPEN_APP_SCRIPT \
    SKYPE_CONTROL_MOBILE_OPEN_APP,           \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE,                         \
        ACTION_NONE

#define SKYPE_CONTROL_MOBILE_STILL_DECIDING_SCRIPT \
    SKYPE_CONTROL_MOBILE_STILL_DECIDING,           \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE,                               \
        ACTION_NONE

// Skype for PC scripts
#define SKYPE_CONTROL_PC_TOGGLE_MIC_SCRIPT \
    SKYPE_CONTROL_PC_TOGGLE_MIC,           \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE

#define SKYPE_CONTROL_PC_TOGGLE_VID_SCRIPT \
    SKYPE_CONTROL_PC_TOGGLE_VID,           \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE

#define SKYPE_CONTROL_PC_IMPROV_VID_SCRIPT \
    SKYPE_CONTROL_PC_IMPROV_VID,           \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE,                       \
        ACTION_NONE

#define SKYPE_CONTROL_PC_OPEN_APP_SCRIPT \
    SKYPE_CONTROL_PC_OPEN_APP,           \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE,                     \
        ACTION_NONE

#define SKYPE_CONTROL_PC_STILL_DECIDING_SCRIPT \
    SKYPE_CONTROL_PC_STILL_DECIDING,           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE,                           \
        ACTION_NONE

// Google Meet for mobile scripts
#define MEET_CONTROL_MOBILE_TOGGLE_MIC_SCRIPT \
    MEET_CONTROL_MOBILE_TOGGLE_MIC,           \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

#define MEET_CONTROL_MOBILE_TOGGLE_VID_SCRIPT \
    MEET_CONTROL_MOBILE_TOGGLE_VID,           \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

#define MEET_CONTROL_MOBILE_IMPROV_VID_SCRIPT \
    MEET_CONTROL_MOBILE_IMPROV_VID,           \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

#define MEET_CONTROL_MOBILE_OPEN_APP_SCRIPT \
    MEET_CONTROL_MOBILE_OPEN_APP,           \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE,                        \
        ACTION_NONE

#define MEET_CONTROL_MOBILE_STILL_DECIDING_SCRIPT \
    MEET_CONTROL_MOBILE_STILL_DECIDING,           \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE,                              \
        ACTION_NONE

// Google Meet for PC scripts
#define MEET_CONTROL_PC_TOGGLE_MIC_SCRIPT \
    MEET_CONTROL_PC_TOGGLE_MIC,           \
        ACTION_COMBINE_NEXT_KEYS(2),      \
        HID_KEY_LEFT_CTRL,                \
        HID_KEY_D,                        \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define MEET_CONTROL_PC_TOGGLE_VID_SCRIPT \
    MEET_CONTROL_PC_TOGGLE_VID,           \
        ACTION_COMBINE_NEXT_KEYS(2),      \
        HID_KEY_LEFT_CTRL,                \
        HID_KEY_E,                        \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define MEET_CONTROL_PC_IMPROV_VID_SCRIPT \
    MEET_CONTROL_PC_IMPROV_VID,           \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE,                      \
        ACTION_NONE

#define MEET_CONTROL_PC_OPEN_APP_SCRIPT \
    MEET_CONTROL_PC_OPEN_APP,           \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE,                    \
        ACTION_NONE

#define MEET_CONTROL_PC_STILL_DECIDING_SCRIPT \
    MEET_CONTROL_PC_STILL_DECIDING,           \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE,                          \
        ACTION_NONE

// test defines..
#define MEETING1_ID "12345670"
#define MEETING1_PASSCODE "coMeValavita"

    // TYPEDEFS

    typedef int command_code_t; // 'int' because of the use of the va_arg function

    typedef struct
    {

        uint8_t app_control_id;
        uint8_t num_of_scripts;
        command_code_t **scripts;
        uint8_t scripts_max_steps;

    } app_control_struct_t;

    typedef enum
    {

        // For Zoom Meetings (mobile)
        ZOOM_CONTROL_MOBILE_TOGGLE_MIC,
        ZOOM_CONTROL_MOBILE_TOGGLE_VID,
        ZOOM_CONTROL_MOBILE_JOIN1,
        ZOOM_CONTROL_MOBILE_OPEN_APP,
        ZOOM_CONTROL_MOBILE_STILL_DECIDING
    } zoom_mobile_scripts_names_available_t;

    typedef enum
    {

        // For Zoom Meetings (pc)
        ZOOM_CONTROL_PC_TOGGLE_MIC,
        ZOOM_CONTROL_PC_TOGGLE_VID,
        ZOOM_CONTROL_PC_IMPROV_VID,
        ZOOM_CONTROL_PC_OPEN_APP,
        ZOOM_CONTROL_PC_STILL_DECIDING
    } zoom_pc_scripts_names_available_t;

    typedef enum
    {

        // For Skype (mobile)
        SKYPE_CONTROL_MOBILE_TOGGLE_MIC,
        SKYPE_CONTROL_MOBILE_TOGGLE_VID,
        SKYPE_CONTROL_MOBILE_IMPROV_VID,
        SKYPE_CONTROL_MOBILE_OPEN_APP,
        SKYPE_CONTROL_MOBILE_STILL_DECIDING
    } skype_mobile_scripts_names_available_t;

    typedef enum
    {

        // For Skype (pc)
        SKYPE_CONTROL_PC_TOGGLE_MIC,
        SKYPE_CONTROL_PC_TOGGLE_VID,
        SKYPE_CONTROL_PC_IMPROV_VID,
        SKYPE_CONTROL_PC_OPEN_APP,
        SKYPE_CONTROL_PC_STILL_DECIDING
    } skype_pc_scripts_names_available_t;

    typedef enum
    {

        // For Google Meet (mobile)
        MEET_CONTROL_MOBILE_TOGGLE_MIC,
        MEET_CONTROL_MOBILE_TOGGLE_VID,
        MEET_CONTROL_MOBILE_IMPROV_VID,
        MEET_CONTROL_MOBILE_OPEN_APP,
        MEET_CONTROL_MOBILE_STILL_DECIDING
    } meet_mobile_scripts_names_available_t;

    typedef enum
    {

        // For Google Meet (pc, probably from a browser)
        MEET_CONTROL_PC_TOGGLE_MIC,
        MEET_CONTROL_PC_TOGGLE_VID,
        MEET_CONTROL_PC_IMPROV_VID,
        MEET_CONTROL_PC_OPEN_APP,
        MEET_CONTROL_PC_STILL_DECIDING
    } meet_pc_scripts_names_available_t;

    // WARNING: DO NOT TOUCH THIS TYPEDEF IF NOT SURE!

    typedef enum
    {
        // These can be chained together in an array for extendend return codes
        SPECIAL_ACTION_RETURN_CODE_OK,
        SPECIAL_ACTION_RETURN_CODE_FAIL,
        SPECIAL_ACTION_RETURN_CODE_SKIP_NEXT,
        SPECIAL_ACTION_RETURN_CODE_END_SCRIPT,

    } special_actions_return_codes_t;

    // VARIABLES

    typedef struct
    {
        void (*pfunction)(void *);
        uint8_t *arg1;
        uint8_t *arg2;
        command_code_t *host_script; // script where the special script will be referred to
        uint16_t hid_conn_id;
        uint8_t *returnCode; // can be an array, see 'special_actions_return_codes_t' enum
    } app_control_special_script_t;

    // All apps special functions will be registered here
    extern app_control_special_script_t
        *app_control_special_actions[CONTROL_SCRIPTS_SPECIAL_ACTIONS_TOTAL];

    // FUNCTION PROTOTYPES
    void app_control_init(app_control_struct_t **app_control_register);

    app_control_struct_t *app_control_setup_new(uint8_t app_id,
                                                uint8_t num_of_scripts,
                                                uint8_t max_steps_per_script, ...);

    app_control_struct_t **app_control_register_new(uint8_t num_of_apps_registered, ...);

#ifdef __cplusplus
}
#endif

#endif /* HID_APP_CONTROL_H */

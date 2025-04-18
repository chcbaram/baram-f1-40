#pragma once


#define KBD_NAME                    "BARAM-F1-40"

#define USB_VID                     0x0483
#define USB_PID                     0x5303


// hw_def.h
//
// #define _USE_HW_VCOM

// eeprom
//
#define EEPROM_CHIP_ZD24C128 
#define EECONFIG_USER_DATA_SIZE     512               
#define TOTAL_EEPROM_BYTE_COUNT     4096

#define DYNAMIC_KEYMAP_LAYER_COUNT  8

#define MATRIX_ROWS                 4
#define MATRIX_COLS                 12

#define DEBOUNCE                    20


// #define DEBUG_KEY_SEND
#define DEBUG_MATRIX_SCAN_RATE
#define GRAVE_ESC_ENABLE
#define KILL_SWITCH_ENABLE
#define KKUK_ENABLE
/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "SD_MMC.h"
#include "RGB.h"
#include "Wireless.h"
#include "LVGL_Example.h"
#include "WebServer.h"
#include "WLED_Controller.h"

void app_main(void)
{
    // Initialize WiFi first (required for ESP-NOW)
    Wireless_Init();
    
    // Small delay to let WiFi stabilize
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Initialize ESP-NOW and trigger alarm (works without WiFi CONNECTION)
    WLED_ESPNOW_Init();
    WLED_ESPNOW_TriggerAlarm();  // Broadcast alarm immediately
    
    // Continue with normal initialization
    WebServer_Init();      // Start web server after WiFi initialization
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    SD_Init();
    LCD_Init();
    BK_Light(50);
    LVGL_Init();   // returns the screen object

/********************* Demo *********************/
    Lvgl_Example1();

    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_benchmark();
    // lv_demo_stress();
    // lv_demo_music();

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}

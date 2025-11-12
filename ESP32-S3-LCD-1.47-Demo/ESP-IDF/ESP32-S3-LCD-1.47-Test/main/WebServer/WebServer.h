#pragma once

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Initialize and start the HTTP web server
 * 
 * This function starts a web server on port 80 that serves:
 * - Main HTML page at GET /
 * - JSON API endpoint at GET /api/data
 * 
 * The web server mirrors LCD display content and shows device information.
 * Task priority is set to 3 (below LVGL priority) to avoid display interference.
 */
void WebServer_Init(void);

/**
 * @brief Stop the HTTP web server
 */
void WebServer_Stop(void);


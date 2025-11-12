/**
 * @file WLED_Controller.h
 * @brief WLED ESP-NOW Remote Controller for ESP32-S3
 * 
 * Implements WizMote-compatible ESP-NOW sender to control WLED devices.
 * Broadcasts button codes across all WiFi channels for instant response.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WLED ESP-NOW controller
 * 
 * Initializes ESP-NOW for broadcasting. Does NOT require WiFi connection.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t WLED_ESPNOW_Init(void);

/**
 * @brief Send button code to all WLED devices
 * 
 * Broadcasts WizMote-compatible button code across all WiFi channels (1-13).
 * Non-blocking - returns immediately after queuing sends.
 * 
 * @param button_code WizMote button code (0=toggle, 1-3=presets, 8/9=brightness)
 * @return ESP_OK if broadcast initiated successfully
 */
esp_err_t WLED_ESPNOW_SendButton(uint8_t button_code);

/**
 * @brief Trigger alarm on boot (preset 1)
 * 
 * Convenience function to send button code 1 (preset 1) for alarm.
 */
void WLED_ESPNOW_TriggerAlarm(void);

/**
 * @brief Get device MAC address as formatted string
 * 
 * Returns MAC address in format "AA:BB:CC:DD:EE:FF" for display/pairing.
 * 
 * @param mac_str Buffer to store MAC string (minimum 18 bytes)
 * @return ESP_OK on success
 */
esp_err_t WLED_ESPNOW_GetMAC(char *mac_str);

#ifdef __cplusplus
}
#endif

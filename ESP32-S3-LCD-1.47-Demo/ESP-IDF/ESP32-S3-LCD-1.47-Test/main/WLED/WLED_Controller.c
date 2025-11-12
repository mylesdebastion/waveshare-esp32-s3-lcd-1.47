/**
 * @file WLED_Controller.c
 * @brief WLED ESP-NOW Remote Implementation
 * 
 * Based on WizMote protocol - broadcasts button codes across all WiFi channels.
 * Reference: https://github.com/DedeHai/WLED-ESPNow-Remote
 */

#include "WLED_Controller.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WLED_ESPNOW";

// WizMote message structure (compatible with WLED)
typedef struct {
    uint8_t button;  // Button code (0-9, 16-31)
    uint8_t battery; // Battery level (255 = not battery powered)
    uint8_t flags;   // Reserved flags
} wizmote_message_t;

// Broadcast MAC address (all devices receive)
static uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Initialization state
static bool espnow_initialized = false;

/**
 * @brief ESP-NOW send callback
 */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGD(TAG, "Send success");
    } else {
        ESP_LOGD(TAG, "Send failed");
    }
}

/**
 * @brief Broadcast message on a specific WiFi channel
 */
static esp_err_t broadcast_on_channel(uint8_t channel, wizmote_message_t *msg)
{
    esp_err_t err;
    
    // Set WiFi channel
    err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set channel %d: %s", channel, esp_err_to_name(err));
        return err;
    }
    
    // Small delay to let channel change take effect
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Send message
    err = esp_now_send(broadcast_mac, (uint8_t *)msg, sizeof(wizmote_message_t));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send on channel %d: %s", channel, esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t WLED_ESPNOW_Init(void)
{
    if (espnow_initialized) {
        ESP_LOGW(TAG, "ESP-NOW already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing WLED ESP-NOW controller...");
    
    // Assume WiFi is already initialized by Wireless_Init()
    // We just need to initialize ESP-NOW
    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ESP-NOW: %s", esp_err_to_name(err));
        return err;
    }
    
    // Register send callback
    err = esp_now_register_send_cb(espnow_send_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register send callback: %s", esp_err_to_name(err));
        return err;
    }
    
    // Add broadcast peer
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    peer_info.channel = 0;  // Use current channel
    peer_info.ifidx = ESP_IF_WIFI_STA;
    peer_info.encrypt = false;
    
    err = esp_now_add_peer(&peer_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add broadcast peer: %s", esp_err_to_name(err));
        return err;
    }
    
    espnow_initialized = true;
    
    // Get and log MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    ESP_LOGI(TAG, "ESP-NOW initialized. MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Add this MAC to WLED Config -> WiFi -> ESP-NOW Remote");
    
    return ESP_OK;
}

esp_err_t WLED_ESPNOW_SendButton(uint8_t button_code)
{
    if (!espnow_initialized) {
        ESP_LOGE(TAG, "ESP-NOW not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Sending button code: %d", button_code);
    
    // Prepare WizMote message
    wizmote_message_t msg = {
        .button = button_code,
        .battery = 255,  // Not battery powered
        .flags = 0
    };
    
    // Broadcast on channels 1-13 (all WiFi channels)
    // This ensures WLED devices on any channel will receive the message
    int success_count = 0;
    for (uint8_t channel = 1; channel <= 13; channel++) {
        if (broadcast_on_channel(channel, &msg) == ESP_OK) {
            success_count++;
        }
    }
    
    ESP_LOGI(TAG, "Button %d broadcast on %d/13 channels", button_code, success_count);
    
    return (success_count > 0) ? ESP_OK : ESP_FAIL;
}

void WLED_ESPNOW_TriggerAlarm(void)
{
    if (!espnow_initialized) {
        ESP_LOGW(TAG, "Cannot trigger alarm - ESP-NOW not initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Triggering alarm (Preset 1)");
    WLED_ESPNOW_SendButton(1);  // Button 1 = Preset 1
}

esp_err_t WLED_ESPNOW_GetMAC(char *mac_str)
{
    if (!mac_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t mac[6];
    esp_err_t err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MAC: %s", esp_err_to_name(err));
        return err;
    }
    
    snprintf(mac_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return ESP_OK;
}

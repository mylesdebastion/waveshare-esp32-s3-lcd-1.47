#include "LVGL_Example.h"
#include "esp_netif.h"
#include "../WLED/WLED_Controller.h"

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * ip_label;
static lv_timer_t * update_timer;
static const lv_font_t * font_large;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void update_ip_display(lv_timer_t * timer);

void Lvgl_Example1(void)
{
    // Use the largest available font
    #if LV_FONT_MONTSERRAT_48
        font_large = &lv_font_montserrat_48;
    #elif LV_FONT_MONTSERRAT_32
        font_large = &lv_font_montserrat_32;
    #elif LV_FONT_MONTSERRAT_24
        font_large = &lv_font_montserrat_24;
    #elif LV_FONT_MONTSERRAT_18
        font_large = &lv_font_montserrat_18;
    #else
        font_large = LV_FONT_DEFAULT;
    #endif

    // Create a simple centered label
    ip_label = lv_label_create(lv_scr_act());
    lv_label_set_text(ip_label, "IP: Connecting...\nMAC: Loading...");
    lv_obj_set_style_text_font(ip_label, font_large, 0);
    lv_obj_set_style_text_color(ip_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(ip_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Center the label on screen
    lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 0);
    
    // Set background color
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x2196F3), 0);
    
    // Create timer to update IP display every 500ms
    update_timer = lv_timer_create(update_ip_display, 500, NULL);
}

void Lvgl_Example1_close(void)
{
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }
    lv_obj_clean(lv_scr_act());
}

static void update_ip_display(lv_timer_t * timer)
{
    char buf[256] = {0};
    char mac_str[18] = {0};
    
    // Get MAC address
    WLED_ESPNOW_GetMAC(mac_str);
    
    // Get IP address if WiFi is connected
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            // Check if we have a valid IP (not 0.0.0.0)
            if (ip_info.ip.addr != 0) {
                snprintf(buf, sizeof(buf), "IP: " IPSTR "\nMAC: %s", IP2STR(&ip_info.ip), mac_str);
            } else {
                snprintf(buf, sizeof(buf), "IP: Not Connected\nMAC: %s", mac_str);
            }
        } else {
            snprintf(buf, sizeof(buf), "IP: No WiFi\nMAC: %s", mac_str);
        }
    } else {
        snprintf(buf, sizeof(buf), "IP: Initializing...\nMAC: %s", mac_str);
    }
    
    lv_label_set_text(ip_label, buf);
}

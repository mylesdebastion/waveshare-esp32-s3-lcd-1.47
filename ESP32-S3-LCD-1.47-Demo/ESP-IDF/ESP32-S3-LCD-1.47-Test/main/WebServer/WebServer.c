#include "WebServer.h"
#include "SD_MMC.h"
#include "Wireless.h"
#include "WLED_Controller.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include <sys/param.h>
#include <cJSON.h>

static const char *TAG = "WebServer";
static httpd_handle_t server = NULL;

// HTML page with inline CSS and JavaScript for AJAX polling
static const char* html_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>ESP32-S3 LCD Status</title>"
"<style>"
"body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #fff; }"
"h1 { text-align: center; margin-bottom: 10px; }"
".container { max-width: 600px; margin: 0 auto; background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); padding: 30px; border-radius: 15px; box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37); }"
".section { background: rgba(255,255,255,0.15); padding: 20px; border-radius: 10px; margin-bottom: 20px; }"
".section h2 { margin-top: 0; font-size: 1.3em; border-bottom: 2px solid rgba(255,255,255,0.3); padding-bottom: 10px; }"
".info-row { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid rgba(255,255,255,0.2); }"
".info-row:last-child { border-bottom: none; }"
".label { font-weight: bold; opacity: 0.9; }"
".value { font-family: 'Courier New', monospace; background: rgba(0,0,0,0.2); padding: 4px 8px; border-radius: 4px; }"
".status { text-align: center; font-size: 0.85em; opacity: 0.7; margin-top: 20px; }"
".loading { animation: pulse 1.5s ease-in-out infinite; }"
"@keyframes pulse { 0%, 100% { opacity: 0.5; } 50% { opacity: 1; } }"
"button { background: rgba(255,255,255,0.2); border: 2px solid rgba(255,255,255,0.3); color: #fff; padding: 10px 20px; border-radius: 8px; cursor: pointer; font-size: 0.95em; margin: 5px; transition: all 0.3s; }"
"button:hover { background: rgba(255,255,255,0.3); transform: translateY(-2px); }"
"button:active { transform: translateY(0); }"
".btn-group { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 10px; }"
"input[type='text'] { background: rgba(0,0,0,0.2); border: 2px solid rgba(255,255,255,0.3); color: #fff; padding: 8px; border-radius: 6px; width: 100%; box-sizing: border-box; margin-top: 10px; }"
"input[type='text']::placeholder { color: rgba(255,255,255,0.5); }"
"#wled-list { margin-top: 10px; max-height: 150px; overflow-y: auto; }"
".wled-device { display: flex; justify-content: space-between; align-items: center; padding: 8px; background: rgba(0,0,0,0.2); border-radius: 6px; margin-bottom: 8px; }"
".wled-device button { margin: 0; padding: 5px 10px; font-size: 0.85em; }"
".message { text-align: center; padding: 10px; border-radius: 6px; margin-top: 10px; font-size: 0.9em; }"
".message.success { background: rgba(0,255,0,0.2); }"
".message.error { background: rgba(255,0,0,0.2); }"
".mac-display { font-family: 'Courier New'; font-size: 1.2em; background: rgba(0,0,0,0.3); padding: 15px; border-radius: 8px; text-align: center; user-select: all; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<h1>🖥️ ESP32-S3 LCD 1.47</h1>"
"<div class='section'>"
"<h2>Device Information</h2>"
"<div class='info-row'><span class='label'>IP Address:</span><span class='value' id='ip'>Loading...</span></div>"
"<div class='info-row'><span class='label'>Hostname:</span><span class='value' id='hostname'>Loading...</span></div>"
"<div class='info-row'><span class='label'>Uptime:</span><span class='value' id='uptime'>Loading...</span></div>"
"</div>"
"<div class='section'>"
"<h2>LCD Display Status</h2>"
"<div class='info-row'><span class='label'>SD Card Size:</span><span class='value' id='sd'>Loading...</span></div>"
"<div class='info-row'><span class='label'>Flash Size:</span><span class='value' id='flash'>Loading...</span></div>"
"<div class='info-row'><span class='label'>WiFi Networks:</span><span class='value' id='wifi'>Loading...</span></div>"
"<div class='info-row'><span class='label'>BLE Devices:</span><span class='value' id='ble'>Loading...</span></div>"
"<div class='info-row'><span class='label'>Scan Status:</span><span class='value' id='scan'>Loading...</span></div>"
"</div>"
"<div class='section'>"
"<h2>⚙️ WLED ESP-NOW Remote</h2>"
"<div class='mac-display' id='mac-address'>MAC: Loading...</div>"
"<p style='font-size:0.9em;opacity:0.8;margin-top:8px'>Add this MAC to WLED Config → WiFi → ESP-NOW Remote</p>"
"<div class='btn-group'>"
"<button onclick='sendWLED(1)'>🔴 Preset 1</button>"
"<button onclick='sendWLED(2)'>🟢 Preset 2</button>"
"<button onclick='sendWLED(3)'>🔵 Preset 3</button>"
"</div>"
"<div class='btn-group'>"
"<button onclick='sendWLED(0)'>💡 Toggle</button>"
"<button onclick='sendWLED(9)'>⬆️ Bright</button>"
"<button onclick='sendWLED(8)'>⬇️ Dim</button>"
"</div>"
"<div id='wled-message'></div>"
"</div>"
"<div class='status'>Auto-updating every 3 seconds | <span id='last-update' class='loading'>Connecting...</span></div>"
"</div>"
"<script>"
"function formatUptime(seconds) {"
"  const days = Math.floor(seconds / 86400);"
"  const hours = Math.floor((seconds % 86400) / 3600);"
"  const minutes = Math.floor((seconds % 3600) / 60);"
"  const secs = seconds % 60;"
"  if (days > 0) return `${days}d ${hours}h ${minutes}m ${secs}s`;"
"  if (hours > 0) return `${hours}h ${minutes}m ${secs}s`;"
"  if (minutes > 0) return `${minutes}m ${secs}s`;"
"  return `${secs}s`;"
"}"
"function showMessage(msg, isError) {"
"  const msgDiv = document.getElementById('wled-message');"
"  msgDiv.textContent = msg;"
"  msgDiv.className = 'message ' + (isError ? 'error' : 'success');"
"  setTimeout(() => { msgDiv.textContent = ''; msgDiv.className = ''; }, 3000);"
"}"
"function sendWLED(btn) {"
"  showMessage(`Sending button ${btn}...`, false);"
"  fetch('/api/wled/button', {"
"    method: 'POST',"
"    body: JSON.stringify({button: btn}),"
"    headers: {'Content-Type': 'application/json'}"
"  })"
"    .then(r => r.json())"
"    .then(data => showMessage(data.message, !data.success))"
"    .catch(e => showMessage('Send failed', true));"
"}"
"function loadMAC() {"
"  fetch('/api/wled/mac')"
"    .then(r => r.json())"
"    .then(data => document.getElementById('mac-address').textContent = 'MAC: ' + data.mac)"
"    .catch(e => document.getElementById('mac-address').textContent = 'MAC: Error');"
"}"
"function updateData() {"
"  fetch('/api/data')"
"    .then(response => response.json())"
"    .then(data => {"
"      document.getElementById('ip').textContent = data.ip;"
"      document.getElementById('hostname').textContent = data.hostname;"
"      document.getElementById('uptime').textContent = formatUptime(data.uptime);"
"      document.getElementById('sd').textContent = data.sd_size + ' MB';"
"      document.getElementById('flash').textContent = data.flash_size + ' MB';"
"      document.getElementById('wifi').textContent = data.wifi_count;"
"      document.getElementById('ble').textContent = data.ble_count;"
"      document.getElementById('scan').textContent = data.scan_complete ? '✓ Complete' : '⟳ Scanning...';"
"      document.getElementById('last-update').textContent = new Date().toLocaleTimeString();"
"      document.getElementById('last-update').classList.remove('loading');"
"    })"
"    .catch(error => {"
"      console.error('Error fetching data:', error);"
"      document.getElementById('last-update').textContent = 'Error updating';"
"    });"
"}"
"updateData();"
"loadMAC();"
"setInterval(updateData, 3000);"
"</script>"
"</body>"
"</html>";

/* Handler for GET / */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving root page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /api/data */
static esp_err_t data_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving API data");
    
    // Get IP address
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    char ip_str[16] = "0.0.0.0";
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    }
    
    // Get hostname
    const char *hostname = "esp32-s3";
    esp_netif_get_hostname(netif, &hostname);
    
    // Get uptime in seconds
    uint32_t uptime = esp_log_timestamp() / 1000;
    
    // Build JSON response
    char json_response[512];
    snprintf(json_response, sizeof(json_response),
        "{"
        "\"ip\":\"%s\","
        "\"hostname\":\"%s\","
        "\"uptime\":%lu,"
        "\"sd_size\":%lu,"
        "\"flash_size\":%lu,"
        "\"wifi_count\":%u,"
        "\"ble_count\":%u,"
        "\"scan_complete\":%s"
        "}",
        ip_str,
        hostname,
        (unsigned long)uptime,
        (unsigned long)SDCard_Size,
        (unsigned long)Flash_Size,
        WIFI_NUM,
        BLE_NUM,
        Scan_finish ? "true" : "false"
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /api/wled/mac */
static esp_err_t wled_mac_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving WLED MAC address");
    
    char mac_str[18];
    esp_err_t err = WLED_ESPNOW_GetMAC(mac_str);
    
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get MAC");
        return ESP_FAIL;
    }
    
    char json_response[64];
    snprintf(json_response, sizeof(json_response), "{\"mac\":\"%s\"}", mac_str);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_response);
    
    return ESP_OK;
}

/* Handler for POST /api/wled/button */
static esp_err_t wled_button_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Sending WLED button code");
    
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *button_item = cJSON_GetObjectItem(root, "button");
    if (!button_item || !cJSON_IsNumber(button_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'button' field");
        return ESP_FAIL;
    }
    
    uint8_t button_code = (uint8_t)button_item->valueint;
    esp_err_t err = WLED_ESPNOW_SendButton(button_code);
    
    char json_response[128];
    snprintf(json_response, sizeof(json_response),
        "{\"success\":%s,\"message\":\"Button %d %s\"}",
        (err == ESP_OK) ? "true" : "false",
        button_code,
        (err == ESP_OK) ? "sent" : "failed");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_response);
    
    cJSON_Delete(root);
    return ESP_OK;
}

/* URI handler structure for GET / */
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

/* URI handler structure for GET /api/data */
static const httpd_uri_t data_uri = {
    .uri       = "/api/data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    .user_ctx  = NULL
};

/* URI handler structure for GET /api/wled/mac */
static const httpd_uri_t wled_mac_uri = {
    .uri       = "/api/wled/mac",
    .method    = HTTP_GET,
    .handler   = wled_mac_get_handler,
    .user_ctx  = NULL
};

/* URI handler structure for POST /api/wled/button */
static const httpd_uri_t wled_button_uri = {
    .uri       = "/api/wled/button",
    .method    = HTTP_POST,
    .handler   = wled_button_post_handler,
    .user_ctx  = NULL
};

/* Function to start the web server */
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 3;  // Lower priority than LVGL (typically 5)
    config.core_id = 0;        // Run on core 0
    config.stack_size = 8192;  // Increased stack size for JSON formatting
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;
    
    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &wled_mac_uri);
        httpd_register_uri_handler(server, &wled_button_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/* Function to stop the web server */
static void stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
    }
}

void WebServer_Init(void)
{
    ESP_LOGI(TAG, "Initializing web server...");
    
    // Wait a bit to ensure WiFi is fully connected
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    server = start_webserver();
    
    if (server) {
        ESP_LOGI(TAG, "Web server started successfully");
        ESP_LOGI(TAG, "Visit http://<device-ip>/ to view the status page");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}

void WebServer_Stop(void)
{
    ESP_LOGI(TAG, "Stopping web server...");
    stop_webserver(server);
    server = NULL;
}


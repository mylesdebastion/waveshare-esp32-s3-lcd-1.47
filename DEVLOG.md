# Development Log - Waveshare ESP32-S3-LCD-1.47
**Date:** November 11, 2025  
**Project:** ESP32-S3-LCD-1.47 Demo Compilation and Testing

---

## Session Goals
1. Compile and flash the ESP-IDF demo project to the Waveshare ESP32-S3-LCD-1.47 board
2. Modify RGB LED to pulse red slowly (to verify our changes work)
3. Configure WiFi to connect to E2-Kids network
4. Document the entire process and learnings

---

## Hardware
- **Board:** Waveshare ESP32-S3-LCD-1.47
- **Display:** 1.47" 172x320 ST7789 LCD
- **RGB LED:** WS2812B (single LED, GRB color order)
- **Port:** COM5
- **WiFi Network:** E2-Kids (password: kidskidskids)

---

## Development Environment Setup

### ESP-IDF Installation
- **Version:** ESP-IDF v5.1.2
- **Path:** `C:\Users\myles\esp\esp-idf`
- **Tools Path:** `C:\Users\myles\.espressif\`
- **Target Chip:** ESP32-S3 (xtensa-esp32s3-elf toolchain)

### Initial Setup Process
1. Manually cloned ESP-IDF v5.1.2 from GitHub
2. Ran `install.ps1` to install toolchains (esp32s3, esp32)
3. Set up environment using `export.ps1` for each session

**Key Learning:** The Waveshare wiki recommends using the ESP-IDF VS Code/Cursor extension, but we successfully used CLI with manual ESP-IDF installation for maximum control.

---

## Major Challenges Encountered

### 1. **API Compatibility Issues (ESP-IDF v4.x → v5.1.2)**

#### Problem 1: SD Card Driver (`SD_MMC.c`)
**Error:**
```c
error: 'sdmmc_slot_config_t' has no member named 'clk', 'cmd', 'd0', 'd1', 'd2', 'd3'
```

**Root Cause:** ESP-IDF v5.x changed the SDMMC API. Pin configuration moved from `sdmmc_slot_config_t` to automatic handling by `sdmmc_host_init_slot()`.

**Solution:** Commented out manual pin assignments
```c
// NOTE: ESP-IDF v5.x API changed - pin configuration now handled by sdmmc_host_init_slot()
// slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
// slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
// ... etc
```

**File Modified:** `main/SD_Card/SD_MMC.c:79-84`

---

#### Problem 2: Bluetooth LE (`Wireless.c`)
**Error:**
```c
error: implicit declaration of function 'esp_ble_dtm_stop'
```

**Root Cause:** The vendor code incorrectly used `esp_ble_dtm_stop()` (Direct Test Mode) instead of `esp_ble_gap_stop_scanning()` for regular BLE scanning.

**Solution:** Replaced with correct API call
```c
ESP_ERROR_CHECK(esp_ble_gap_stop_scanning());
// NOTE: esp_ble_dtm_stop() is for Direct Test Mode, not for regular scanning
```

**File Modified:** `main/Wireless/Wireless.c:211`

---

### 2. **Wrong Target Chip (ESP32 vs ESP32-S3)**

#### Problem
Initial build targeted ESP32 instead of ESP32-S3, resulting in:
```
IRAM segment data does not fit
DRAM segment data does not fit
region 'iram0_0_seg' overflowed by 5568 bytes
```

**Root Cause:** Build system defaulted to ESP32 chip.

**Solution:** Set correct target
```bash
idf.py set-target esp32s3
```

**Key Learning:** Always verify target chip matches hardware before building. ESP32 and ESP32-S3 have different memory layouts.

---

### 3. **RGB LED Color Order Issue (RGB vs GRB)**

#### Problem
LED pulsed **green** instead of **red** when we set `Set_RGB(brightness, 0, 0)`.

**Root Cause:** WS2812B LEDs use **GRB** color order, not RGB. The `led_strip_set_pixel()` function expected RGB parameters, but the physical LED interprets first byte as Green.

**Solution:** Swapped parameter order in the driver
```c
// Old (incorrect):
led_strip_set_pixel(led_strip, 0, red_val, green_val, blue_val);

// New (correct for WS2812B):
led_strip_set_pixel(led_strip, 0, green_val, red_val, blue_val);
```

**File Modified:** `main/RGB/RGB.c:56`

**Key Learning:** WS2812B native format is GRB, not RGB. Always check LED datasheet for color channel order.

---

### 4. **Serial Port Lock Issues**

#### Problem
```
Could not open COM5, the port is busy or doesn't exist.
PermissionError(13, 'Access is denied.')
```

**Root Cause:** Multiple `idf_monitor` Python processes were holding COM5 open from previous sessions.

**Detection Method:**
```powershell
Get-CimInstance Win32_Process | Where-Object { 
    $_.CommandLine -like "*COM5*" -or $_.CommandLine -like "*monitor*" 
}
```

**Solution:** Kill all monitor processes
```powershell
Stop-Process -Id 37256,1504,39772,3704,30560,14948,12936 -Force
```

**Key Learning:** Always properly exit monitor sessions (Ctrl+]). After reboot or errors, check for zombie monitor processes before flashing.

---

## Code Modifications

### 1. RGB LED - Red Pulsing Effect

**File:** `main/RGB/RGB.c`

**Original Behavior:** Rainbow color cycle through 192 preset colors

**New Behavior:** Smooth red pulsing (breathing effect)

```c
void _RGB_Example(void *arg)
{
    uint8_t brightness = 0;
    int8_t direction = 1;  // 1 for increasing, -1 for decreasing
    
    while(1)
    {
        // Set LED to red only with current brightness
        Set_RGB(brightness, 0, 0);
        
        // Update brightness for pulsing effect
        brightness += direction * 2;
        
        // Reverse direction at boundaries
        if (brightness >= 250) {
            direction = -1;
        } else if (brightness <= 5) {
            direction = 1;
        }
        
        // Delay for smooth, quicker pulsing
        vTaskDelay(15 / portTICK_PERIOD_MS);  // 15ms = ~66Hz update rate
    }
}
```

**Added Includes:**
```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
```

---

### 2. WiFi Connection Implementation

**File:** `main/Wireless/Wireless.c`

**Original Behavior:** Only scanned for WiFi networks, didn't connect

**New Behavior:** Connects to specified network with credentials, auto-reconnects on disconnect

**Changes Made:**

1. **Added WiFi credentials:**
```c
#define WIFI_SSID      "E2-Kids"
#define WIFI_PASSWORD  "kidskidskids"
static bool wifi_connected = false;
```

2. **Created event handler:**
```c
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi disconnected, reconnecting...\n");
        esp_wifi_connect();
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("WiFi connected! IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}
```

3. **Modified `WIFI_Init()` to connect instead of just scan:**
   - Registered event handlers for WiFi events and IP assignment
   - Configured WiFi station mode with SSID/password
   - Wait up to 10 seconds for connection
   - Keep task alive to maintain connection (changed from `vTaskDelete(NULL)`)

**File:** `main/Wireless/Wireless.h`

**Added includes for WiFi functionality:**
```c
#include "esp_event.h"
#include "esp_netif.h"
```

---

## Build Statistics

### Final Binary Sizes
- **Bootloader:** 0x51c0 bytes (0x2e40 bytes / 36% free)
- **Application:** 0x177470 bytes (1,536,112 bytes)
- **Partition:** 0x300000 bytes (3,145,728 bytes allocated)
- **Free Space:** 0x188b90 bytes (1,609,616 bytes / 51% free)

### Compilation Stats
- **Total Files Compiled:** 1,688 files
- **Main Components:**
  - LVGL library: ~400 files
  - Bluetooth stack: ~300 files
  - WiFi/networking: ~200 files
  - Core ESP-IDF: ~600 files
  - Application code: ~10 files

---

## Key Technical Learnings

### 1. **WS2812B LED Technology**
- Uses single-wire serial protocol
- Native color order: **GRB** (not RGB!)
- Timing-sensitive: RMT peripheral handles signal generation
- 24-bit color: 8 bits per channel
- Requires 5V logic but ESP32-S3 is 3.3V (board has level shifter)

### 2. **ESP-IDF API Evolution**
- **v4.x → v5.x** introduced breaking changes
- SDMMC API: Pin configuration now automatic
- Always check `idf_component.yml` for version requirements
- Vendor code often lags behind latest ESP-IDF versions

### 3. **ESP32-S3 Memory Architecture**
- IRAM (Instruction RAM): Limited, must fit carefully
- DRAM (Data RAM): For variables and heap
- SPIRAM (PSRAM): 2MB external RAM for larger allocations
- Wrong target chip causes memory overflow errors

### 4. **FreeRTOS Task Management**
- `vTaskDelay()`: Yields CPU, allows other tasks to run
- `xTaskCreatePinnedToCore()`: Pin task to specific CPU core
- Task priorities: Higher number = higher priority
- Stack size must be sufficient (4KB-8KB typical)

### 5. **Serial Port Management**
- Monitor processes can lock serial ports
- Use `Ctrl+]` to properly exit ESP-IDF monitor
- PowerShell `Get-CimInstance Win32_Process` useful for debugging
- Always clean up zombie processes before flashing

---

## Flash Configuration

### Partition Table
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 3M,
```

### Flash Settings
- **Mode:** DIO (Dual I/O)
- **Frequency:** 80MHz
- **Size:** 16MB
- **Baud Rate:** 460800 (for flashing)

---

## Successful Build Command Sequence

```powershell
# 1. Navigate to ESP-IDF installation
cd C:\Users\myles\esp\esp-idf

# 2. Load ESP-IDF environment
.\export.ps1

# 3. Navigate to project
cd D:\Github\waveshare-esp32-s3-lcd-1.47\ESP32-S3-LCD-1.47-Demo\ESP-IDF\ESP32-S3-LCD-1.47-Test

# 4. Set correct target (first time only)
idf.py set-target esp32s3

# 5. Build
idf.py build

# 6. Flash and monitor
idf.py -p COM5 flash monitor
```

---

## Testing & Verification

### Expected Behavior
✅ **RGB LED:** Slowly pulses red (breathing effect, ~15ms update rate)  
✅ **LCD Display:** Shows LVGL demo UI with widgets  
✅ **WiFi:** Connects to "E2-Kids" network, displays IP address  
✅ **Bluetooth:** Scans for BLE devices (50 max)  
✅ **SD Card:** Initializes (if card inserted)  

### Serial Monitor Output (Expected)
```
Connecting to WiFi SSID: E2-Kids
WiFi connected! IP: 192.168.x.x
WIFI networks found: XX
BLE scan starting...
```

---

## Files Modified Summary

| File | Purpose | Changes |
|------|---------|---------|
| `main/RGB/RGB.c` | RGB LED control | Changed to red pulsing, fixed GRB order, added FreeRTOS includes |
| `main/Wireless/Wireless.c` | WiFi & BLE | Added WiFi connection logic, fixed BLE stop API |
| `main/Wireless/Wireless.h` | WiFi headers | Added esp_event.h and esp_netif.h |
| `main/SD_Card/SD_MMC.c` | SD card driver | Commented out deprecated pin config for ESP-IDF v5.x |

---

## Future Improvements

### Suggested Enhancements
1. **WiFi Credentials:** Move to NVS storage or provisioning API
2. **RGB Patterns:** Add multiple patterns (rainbow, strobe, etc.)
3. **SD Card:** Implement file browsing on LCD
4. **Power Management:** Add deep sleep when idle
5. **OTA Updates:** Enable over-the-air firmware updates

### Code Quality
1. Remove unused `RGB_Data[192][3]` array (generates warning)
2. Add error handling for WiFi connection failures
3. Implement configuration menu on LCD
4. Add runtime WiFi credential configuration

---

## Useful Commands Reference

### Build & Flash
```bash
idf.py build                    # Build only
idf.py flash                    # Flash only
idf.py monitor                  # Monitor only
idf.py -p COM5 flash monitor   # Flash and monitor
idf.py clean                    # Clean build files
idf.py fullclean               # Full clean including sdkconfig
idf.py set-target esp32s3      # Set target chip
```

### Debugging
```bash
idf.py menuconfig              # Configure project settings
idf.py size                    # Show memory usage
idf.py app-flash              # Flash app only (faster)
```

### PowerShell (Port Debugging)
```powershell
Get-CimInstance Win32_SerialPort                    # List COM ports
Get-Process | Where-Object { $_.Name -like "*python*" }  # Find Python processes
Stop-Process -Id <PID> -Force                       # Kill process
```

---

## Resources

### Documentation
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/)
- [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.47)
- [LVGL Documentation](https://docs.lvgl.io/8.3/)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

### Component Versions
- ESP-IDF: v5.1.2
- LVGL: 8.3.11
- LED Strip Driver: 2.5.5

---

## Conclusion

Successfully compiled and modified the Waveshare ESP32-S3-LCD-1.47 demo project. Key achievements:
- ✅ Fixed ESP-IDF v5.x API compatibility issues
- ✅ Implemented custom RGB LED pulsing effect (red breathing)
- ✅ Added WiFi connectivity with auto-reconnect
- ✅ Identified and resolved WS2812B GRB color order issue
- ✅ Documented entire build process and learnings

**Total Development Time:** ~2-3 hours (including troubleshooting)

**Biggest Challenge:** Serial port lock issues from zombie monitor processes

**Biggest Learning:** WS2812B uses GRB, not RGB! Always check hardware datasheets for color order.

---

## Notes for Next Session

1. Port still occasionally locks - consider creating cleanup script
2. Test SD card functionality with image files
3. Implement OTA updates for easier firmware deployment
4. Create web interface for WiFi provisioning
5. Add touch screen input examples (if board has touch)

---

*This log documents real development challenges and solutions. Keep this updated for future reference.*


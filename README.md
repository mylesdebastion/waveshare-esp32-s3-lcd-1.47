# Waveshare ESP32-S3-LCD-1.47 Demo Project

Custom firmware for the Waveshare ESP32-S3-LCD-1.47 development board with enhanced RGB LED control and WiFi connectivity.

## 📋 Hardware

- **Board:** Waveshare ESP32-S3-LCD-1.47
- **Display:** 1.47" 172x320 ST7789 LCD
- **RGB LED:** WS2812B (single LED, GRB color order)
- **MCU:** ESP32-S3 (Xtensa dual-core)
- **Flash:** 16MB
- **PSRAM:** 2MB

## ✨ Features

- ✅ **Red Pulsing RGB LED** - Smooth breathing effect (15ms update rate)
- ✅ **WiFi Connectivity** - Connects to configured network with auto-reconnect
- ✅ **LVGL GUI** - Demo UI with widgets on ST7789 LCD
- ✅ **Bluetooth LE** - Scans for nearby BLE devices
- ✅ **SD Card Support** - SDMMC interface (API updated for ESP-IDF v5.x)
- ✅ **Fixed API Compatibility** - Works with ESP-IDF v5.1.2

## 🚀 Quick Start

### Prerequisites

- ESP-IDF v5.1.2 or later
- Python 3.7+
- USB cable for flashing
- Git

### 1. Clone Repository

```bash
git clone https://github.com/YOUR_USERNAME/waveshare-esp32-s3-lcd-1.47.git
cd waveshare-esp32-s3-lcd-1.47
```

### 2. Configure WiFi Credentials

```bash
cd ESP32-S3-LCD-1.47-Demo/ESP-IDF/ESP32-S3-LCD-1.47-Test/main
cp wifi_config.h.example wifi_config.h
```

Edit `wifi_config.h` with your WiFi credentials:

```c
#define WIFI_SSID      "YourWiFiSSID"
#define WIFI_PASSWORD  "YourWiFiPassword"
```

**⚠️ Important:** `wifi_config.h` is gitignored to protect your credentials!

### 3. Set Up ESP-IDF Environment

**Option A: Using ESP-IDF Installer (Recommended)**
```bash
# Windows
C:\Users\YOUR_USER\esp\esp-idf\export.ps1

# Linux/Mac
. $HOME/esp/esp-idf/export.sh
```

**Option B: Manual Installation**
Follow the [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32s3/get-started/index.html)

### 4. Build & Flash

```bash
cd ESP32-S3-LCD-1.47-Demo/ESP-IDF/ESP32-S3-LCD-1.47-Test

# Set target (first time only)
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor (replace COM5 with your port)
idf.py -p COM5 flash monitor
```

**Exit monitor:** Press `Ctrl+]`

## 🎯 What's Different From Stock Firmware?

### 1. **RGB LED: GRB Color Order Fixed**
**Problem:** Stock code used RGB order, but WS2812B uses GRB natively.  
**Solution:** Swapped color channels in `main/RGB/RGB.c`

```c
// Now correctly outputs RED (not green)
led_strip_set_pixel(led_strip, 0, green_val, red_val, blue_val);
```

### 2. **Red Pulsing Effect**
**Changed:** From rainbow cycle → smooth red breathing effect  
**File:** `main/RGB/RGB.c`

### 3. **WiFi Auto-Connect**
**Added:** Full WiFi station mode with event handlers  
**Features:**
- Auto-connect on boot
- Auto-reconnect on disconnect
- IP address logging
- Connection timeout (10s)

**File:** `main/Wireless/Wireless.c`

### 4. **ESP-IDF v5.x API Compatibility**

#### SD Card Driver (`main/SD_Card/SD_MMC.c`)
```c
// Old API (v4.x) - REMOVED
slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
// Pin config now handled automatically
```

#### Bluetooth LE (`main/Wireless/Wireless.c`)
```c
// Fixed: Use correct API for BLE scanning
esp_ble_gap_stop_scanning();  // Not esp_ble_dtm_stop()
```

## 📁 Project Structure

```
ESP32-S3-LCD-1.47-Demo/ESP-IDF/ESP32-S3-LCD-1.47-Test/
├── main/
│   ├── LCD_Driver/         # ST7789 display driver
│   ├── LVGL_Driver/        # LVGL integration
│   ├── LVGL_UI/            # UI examples
│   ├── RGB/                # WS2812B LED control (MODIFIED)
│   ├── SD_Card/            # SD card interface (FIXED for v5.x)
│   ├── Wireless/           # WiFi & BLE (ADDED WiFi connect)
│   ├── wifi_config.h       # WiFi credentials (gitignored)
│   └── wifi_config.h.example  # Template for WiFi config
├── components/
│   ├── espressif__led_strip/  # LED strip driver
│   └── lvgl__lvgl/            # LVGL library v8.3.11
├── CMakeLists.txt
├── partitions.csv
└── sdkconfig              # ESP-IDF configuration
```

## 🐛 Troubleshooting

### Port Busy Error
```
Could not open COM5, the port is busy
```

**Solution:** Kill zombie monitor processes
```powershell
# Windows PowerShell
Get-Process | Where-Object { $_.Name -like "*python*" -and $_.CommandLine -like "*monitor*" } | Stop-Process -Force
```

### Build Fails: "has no member named 'clk'"
**Cause:** Using old SD card API  
**Solution:** Already fixed in this repo! Update from stock firmware.

### LED Shows Wrong Color
**Cause:** RGB vs GRB color order  
**Solution:** Already fixed! WS2812B uses GRB natively.

### WiFi Won't Connect
1. Check `main/wifi_config.h` exists (copy from `.example`)
2. Verify SSID/password are correct
3. Check router settings (2.4GHz, not hidden)
4. Monitor serial output for error messages

## 📚 Documentation

- **[DEVLOG.md](DEVLOG.md)** - Detailed development notes and learnings
- **[Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.47)** - Official hardware documentation
- **[ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/)** - Framework documentation

## 🔧 Build Configuration

- **ESP-IDF:** v5.1.2
- **LVGL:** 8.3.11
- **Target:** ESP32-S3
- **Flash Mode:** DIO, 80MHz
- **Flash Size:** 16MB
- **Partition:** 3MB app, 4MB factory

## 📝 License

Based on Waveshare demo code. Modifications licensed under MIT.

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch
3. **Don't commit `wifi_config.h`!** (It's gitignored)
4. Commit your changes
5. Push to the branch
6. Open a Pull Request

## ⚡ Quick Commands Reference

```bash
# Build only
idf.py build

# Flash only (faster for subsequent flashes)
idf.py -p COM5 flash

# Monitor only
idf.py -p COM5 monitor

# Clean build
idf.py clean

# Full clean (including sdkconfig)
idf.py fullclean

# Open configuration menu
idf.py menuconfig

# Check binary size
idf.py size
```

## 🎓 Key Learnings

1. **WS2812B uses GRB, not RGB!** - Always check LED datasheet for color order
2. **ESP-IDF v5.x broke SD card API** - Pin config now automatic
3. **Serial port locks persist** - Kill zombie processes after crashes
4. **Target chip matters** - ESP32 vs ESP32-S3 have different memory layouts
5. **WiFi credentials in code = bad** - Always use config files + .gitignore

---

**Built with ❤️ for the Waveshare ESP32-S3-LCD-1.47**

*Last Updated: 2025-11-12*


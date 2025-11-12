# Claude Agent Guide - ESP32-S3-LCD-1.47 Project

This document provides instructions for AI agents to compile and flash this ESP32-S3 project.

---

## 🎯 Quick Reference

**Project Path:** `D:\Github\waveshare-esp32-s3-lcd-1.47\ESP32-S3-LCD-1.47-Demo\ESP-IDF\ESP32-S3-LCD-1.47-Test`  
**Target Board:** ESP32-S3  
**ESP-IDF Version:** v5.1.2  
**COM Port:** COM5 (typical, verify with user)  
**Build System:** ESP-IDF (CMake-based)

---

## 📁 Environment Paths

### ESP-IDF Installation
- **IDF Path:** `C:\Users\myles\esp\esp-idf`
- **IDF Tools:** `C:\Users\myles\.espressif\`
- **Toolchain:** `xtensa-esp32s3-elf` (for ESP32-S3)
- **Python:** Managed by ESP-IDF installer

### VS Code/Cursor Extension
- **Extension Path:** `C:\Users\myles\.cursor\extensions\espressif.esp-idf-extension-1.10.2-universal`
- **Note:** We use the manual ESP-IDF installation at `C:\Users\myles\esp\esp-idf`, not the extension's bundled version

---

## 🔧 Build & Flash Workflow

### 1. Environment Setup (Required for Every New Shell)

Before any ESP-IDF commands, you **MUST** source the environment:

```powershell
cd C:\Users\myles\esp\esp-idf
.\export.ps1
```

This sets up:
- IDF_PATH environment variable
- Python virtual environment
- Toolchain paths (xtensa-esp32s3-elf-gcc, etc.)
- ESP-IDF tools (esptool.py, idf.py, etc.)

### 2. Navigate to Project

```powershell
cd D:\Github\waveshare-esp32-s3-lcd-1.47\ESP32-S3-LCD-1.47-Demo\ESP-IDF\ESP32-S3-LCD-1.47-Test
```

### 3. Build the Project

```powershell
idf.py build
```

**Output:**
- Binary: `build/ESP32-S3-LCD-1.47.bin`
- Build takes ~2-3 minutes for full rebuild
- Incremental builds are much faster

### 4. Flash to Device

```powershell
idf.py -p COM5 flash
```

### 5. Monitor Serial Output

```powershell
idf.py -p COM5 monitor
```

**Exit monitor:** Press `Ctrl+]`

### 6. Combined Flash + Monitor

```powershell
idf.py -p COM5 flash monitor
```

---

## ⚠️ Common Issues & Solutions

### Issue 1: `idf.py: command not found`

**Cause:** ESP-IDF environment not sourced  
**Solution:**
```powershell
cd C:\Users\myles\esp\esp-idf
.\export.ps1
```

### Issue 2: COM5 Access Denied / Port Busy

**Cause:** Previous monitor session still holding the port  
**Solution:**
```powershell
# Find and kill zombie processes
$processes = Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like "*COM5*" -or $_.Name -like "*idf_monitor*" }
$processes | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }

# Wait a moment, then retry
Start-Sleep -Seconds 1
idf.py -p COM5 flash monitor
```

### Issue 3: Wrong Target (ESP32 instead of ESP32-S3)

**Symptoms:** Out of memory errors, wrong toolchain  
**Solution:**
```powershell
idf.py set-target esp32s3
idf.py build
```

### Issue 4: "CMake Error: could not find CMAKE_MAKE_PROGRAM"

**Cause:** Environment not properly set up  
**Solution:** Re-run `.\export.ps1` from IDF directory

### Issue 5: Build Errors After Code Changes

**For incremental issues:**
```powershell
idf.py clean
idf.py build
```

**For persistent issues (nuclear option):**
```powershell
Remove-Item -Recurse -Force build/
idf.py build
```

---

## 📂 Project Structure

```
ESP32-S3-LCD-1.47-Test/
├── main/
│   ├── main.c                      # Entry point (app_main)
│   ├── CMakeLists.txt              # Component sources
│   ├── wifi_config.h               # WiFi credentials (GITIGNORED)
│   ├── wifi_config.h.example       # Template for credentials
│   ├── LCD_Driver/                 # ST7789 display driver
│   ├── LVGL_Driver/                # LVGL integration
│   ├── LVGL_UI/                    # UI implementation
│   ├── RGB/
│   │   ├── RGB.c                   # WS2812B LED control
│   │   └── RGB.h                   # ⚠️ LED is GRB order, not RGB!
│   ├── SD_Card/
│   │   └── SD_MMC.c                # ⚠️ Fixed for ESP-IDF v5.x API
│   └── Wireless/
│       ├── Wireless.c              # WiFi + BLE
│       └── Wireless.h              # ⚠️ Uses wifi_config.h for credentials
├── components/
│   ├── lvgl__lvgl/                 # LVGL library v8.3
│   └── espressif__led_strip/       # LED strip driver
├── build/                          # Build output (gitignored)
├── CMakeLists.txt                  # Main project config
├── sdkconfig                       # ESP-IDF configuration (auto-generated)
├── sdkconfig.defaults              # Default config (16MB flash, SPIRAM)
└── partitions.csv                  # Flash partition table
```

---

## 🔑 Important Project-Specific Notes

### 1. WiFi Configuration

**WiFi credentials are NOT hardcoded!** They must be in `main/wifi_config.h`:

```c
#define WIFI_SSID      "YourSSID"
#define WIFI_PASSWORD  "YourPassword"
```

**First-time setup:**
```powershell
cd main
cp wifi_config.h.example wifi_config.h
# Edit wifi_config.h with actual credentials
```

### 2. RGB LED Color Order

**CRITICAL:** The WS2812B LED uses **GRB order**, not RGB!

```c
// In RGB.c - Set_RGB() function:
led_strip_set_pixel(led_strip, 0, green_val, red_val, blue_val);
//                                  ^^^ GRB order!
```

**When user says "red":** `Set_RGB(255, 0, 0)` → Internally becomes `set_pixel(0, 0, 255, 0)`

### 3. API Compatibility (ESP-IDF v5.x)

This project has been **fixed for ESP-IDF v5.x**:

**SD_MMC.c:** Pin configuration now handled by `sdmmc_host_init_slot()`, not manually:
```c
// ❌ OLD (v4.x): slot_config.clk = GPIO_NUM_39;
// ✅ NEW (v5.x): Pins auto-configured by sdmmc_host_init_slot()
```

**Wireless.c:** BLE scanning uses `esp_ble_gap_stop_scanning()`, not `esp_ble_dtm_stop()`:
```c
// ❌ OLD: esp_ble_dtm_stop();  // Direct Test Mode
// ✅ NEW: esp_ble_gap_stop_scanning();
```

### 4. Target Configuration

**Always verify target before building:**
```powershell
# Check current target
idf.py --version
cat sdkconfig | Select-String "CONFIG_IDF_TARGET"

# Should show: CONFIG_IDF_TARGET="esp32s3"
```

---

## 🔍 Verification Commands

### Check if ESP-IDF is Active
```powershell
$env:IDF_PATH
# Should output: C:\Users\myles\esp\esp-idf
```

### Check Toolchain
```powershell
xtensa-esp32s3-elf-gcc --version
# Should show: gcc version 12.2.0 (crosstool-NG esp-12.2.0_...)
```

### Find COM Port
```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description
# Look for "USB Serial Device" or "CP210x"
```

### Verify Build Output
```powershell
ls build/*.bin
# Should show: build/ESP32-S3-LCD-1.47.bin
```

---

## 🚀 Complete Build & Flash Command (One-Liner)

For a fresh shell, use this single command:

```powershell
cd C:\Users\myles\esp\esp-idf ; .\export.ps1 ; cd D:\Github\waveshare-esp32-s3-lcd-1.47\ESP32-S3-LCD-1.47-Demo\ESP-IDF\ESP32-S3-LCD-1.47-Test ; idf.py build ; idf.py -p COM5 flash monitor
```

**Breakdown:**
1. Navigate to ESP-IDF → Source environment → Navigate to project → Build → Flash & Monitor

---

## 📊 Typical Build Output

**Success indicators:**
```
[1688/1688] Generating binary image from built executable
esptool.py v4.6.2
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated D:/Github/waveshare-esp32-s3-lcd-1.47/ESP32-S3-LCD-1.47-Demo/ESP-IDF/ESP32-S3-LCD-1.47-Test/build/ESP32-S3-LCD-1.47.bin
Project build complete.
```

**Binary size:** ~1.5 MB (typical for this project with LVGL)

---

## 🛠️ Configuration Management

### View/Edit Configuration Menu
```powershell
idf.py menuconfig
```

**Key settings (already configured in `sdkconfig.defaults`):**
- Flash size: 16MB
- SPIRAM: Enabled (Octal, 80MHz)
- LVGL features: Chart, Performance Monitor
- Partition table: Custom (`partitions.csv`)

### Reset to Defaults
```powershell
Remove-Item sdkconfig -Force
idf.py build  # Will regenerate from sdkconfig.defaults
```

---

## 📝 Code Modification Workflow

When user requests code changes:

1. **Modify source files** (RGB.c, Wireless.c, main.c, etc.)
2. **Rebuild** (incremental build is fast):
   ```powershell
   idf.py build
   ```
3. **Kill any zombie monitors** (if flashing fails):
   ```powershell
   Stop-Process -Name idf_monitor -Force -ErrorAction SilentlyContinue
   ```
4. **Flash**:
   ```powershell
   idf.py -p COM5 flash monitor
   ```

**For major structural changes:** `idf.py fullclean && idf.py build`

---

## 🎓 Key Learnings from Initial Development

1. **Always source `export.ps1`** before using `idf.py`
2. **GRB color order** for WS2812B (not RGB)
3. **ESP-IDF v4 → v5 API changes** are significant (SD_MMC, BLE)
4. **Monitor zombie processes** can lock COM ports
5. **Target must be ESP32-S3**, not ESP32 (different toolchain/memory)
6. **WiFi credentials** must be in separate header (security)

---

## 📚 Resources

- **ESP-IDF Documentation:** https://docs.espressif.com/projects/esp-idf/en/v5.1.2/
- **Waveshare Wiki:** https://www.waveshare.com/wiki/ESP32-S3-LCD-1.47
- **LVGL Docs:** https://docs.lvgl.io/8.3/
- **Project DEVLOG:** `DEVLOG.md` (detailed troubleshooting history)
- **Project README:** `README.md` (user-facing documentation)

---

## ✅ Pre-Flight Checklist

Before claiming "ready to build":

- [ ] ESP-IDF environment sourced (`.\export.ps1`)
- [ ] In correct directory (ESP32-S3-LCD-1.47-Test)
- [ ] Target set to `esp32s3` (check `sdkconfig`)
- [ ] `wifi_config.h` exists (if WiFi needed)
- [ ] No zombie `idf_monitor` processes on COM5
- [ ] User confirmed correct COM port

---

**Last Updated:** November 12, 2025  
**Agent:** Claude (Anthropic)  
**Project Status:** ✅ Working (red pulsing LED, WiFi connected, LVGL GUI active)


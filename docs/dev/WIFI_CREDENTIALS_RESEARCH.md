# WiFi Credentials Storage Research

## Summary
This document describes how WiFi credentials are currently stored, read, and written in the ESP32 Radio project.

---

## 1. Storage Location

**File:** `/wifi.json` (stored in LittleFS filesystem)

**Format:** JSON

**Primary WiFi Only:**
```json
{
  "ssid": "YourNetworkName",
  "password": "YourPassword"
}
```

**With Fallback Network (Backup):**
```json
{
  "ssid": "Primary_Network",
  "password": "primary_password",
  "fallback_ssid": "Backup_Network",
  "fallback_password": "backup_password",
  "fallback_enabled": true
}
```

**Fallback Network Features:**
- Optional backup WiFi credentials for automatic failover
- If `fallback_enabled` is `true`, device switches to fallback when primary fails
- Fallback is attempted after manual reconnection fails (~90s mark)
- Useful for mobile hotspots, backup routers, or redundant networks

---

## 2. Data Structure

### In Memory (config.h)
```cpp
struct WiFiCredentials {
    String ssid;
    String password;
    String fallbackSsid;
    String fallbackPassword;
    bool fallbackEnabled = false;
};

extern WiFiCredentials wifiConfig;  // Global variable
```

**Location:** `src/config.h` (lines 68-75)

**Fields:**
- `ssid` - Primary network SSID
- `password` - Primary network password
- `fallbackSsid` - Backup network SSID (optional)
- `fallbackPassword` - Backup network password (optional)
- `fallbackEnabled` - Enable/disable fallback network feature

---

## 3. Reading WiFi Credentials

### Function: `load_wifi_config()`
**File:** `src/config.cpp` (lines 44-73)

**Process:**
1. Opens `/wifi.json` from LittleFS
2. If file doesn't exist → returns `false` (triggers AP mode)
3. Parses JSON using ArduinoJson library
4. Loads `ssid` and `password` into global `wifiConfig` struct
5. Validates that SSID is not empty
6. Returns `true` on success, `false` on failure

**Code:**
```cpp
bool load_wifi_config() {
    File configFile = LittleFS.open("/wifi.json", "r");
    if (!configFile) {
        Serial.println("Файл wifi.json не найден. Запуск в режиме AP.");
        wifiConfig.ssid = "";
        wifiConfig.password = "";
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("Ошибка парсинга wifi.json: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonObject obj = doc.as<JsonObject>();
    wifiConfig.ssid = obj["ssid"].as<String>();
    wifiConfig.password = obj["password"].as<String>();
    wifiConfig.fallbackSsid = obj["fallback_ssid"] | String("");
    wifiConfig.fallbackPassword = obj["fallback_password"] | String("");
    wifiConfig.fallbackEnabled = obj["fallback_enabled"] | false;
    
    if (wifiConfig.ssid.isEmpty()) {
        Serial.println("Пустой SSID в wifi.json");
        return false;
    }
    
    Serial.println("Конфигурация WiFi загружена: " + wifiConfig.ssid);
    if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty()) {
        Serial.println("Fallback сеть: " + wifiConfig.fallbackSsid);
    }
    return true;
}
```

**Key Changes:**
- Added loading of `fallback_ssid`, `fallback_password`, and `fallback_enabled` fields
- Uses `|` operator for default values (empty strings and `false`)
- Logs fallback network if enabled

---

## 4. Writing WiFi Credentials

### Function: `save_wifi_config()`
**File:** `src/config.cpp` (lines 75-97)

**Process:**
1. Opens `/wifi.json` for writing (overwrites existing file)
2. Creates JSON document
3. Writes `ssid` and `password` from global `wifiConfig` struct
4. Serializes JSON to file
5. Returns `true` on success, `false` on failure

**Code:**
```cpp
bool save_wifi_config() {
    File configFile = LittleFS.open("/wifi.json", "w");
    if (!configFile) {
        Serial.println("Не удалось создать wifi.json для записи.");
        return false;
    }

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    
    obj["ssid"] = wifiConfig.ssid;
    obj["password"] = wifiConfig.password;
    obj["fallback_ssid"] = wifiConfig.fallbackSsid;
    obj["fallback_password"] = wifiConfig.fallbackPassword;
    obj["fallback_enabled"] = wifiConfig.fallbackEnabled;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Ошибка записи в wifi.json.");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("Конфигурация WiFi сохранена: " + wifiConfig.ssid);
    if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty()) {
        Serial.println("Fallback сеть: " + wifiConfig.fallbackSsid);
    }
    return true;
}
```

**Key Changes:**
- Added writing of `fallback_ssid`, `fallback_password`, and `fallback_enabled` fields
- Logs fallback network if enabled

---

## 5. WiFi Configuration Flow

### AP Mode (Access Point) - Initial Setup
**File:** `src/web_server_manager.cpp` (lines 159-220)

**Web Interface:** `data/ap_mode.html`

**Process:**
1. Device starts in AP mode if `/wifi.json` doesn't exist
2. User connects to `ESP32-Radio-Setup` WiFi network
3. User accesses web interface at AP IP address
4. User can:
   - Scan for available networks
   - Select network from dropdown OR enter SSID manually
   - Enter password
5. On form submit (`/save` endpoint):
   - Validates SSID and password are provided
   - Updates global `wifiConfig` struct
   - Calls `save_wifi_config()` to write to `/wifi.json`
   - Reboots device

**Code (web_server_manager.cpp, lines 200-220):**
```cpp
server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid;
    if (request->hasParam("ssid_manual", true) && !request->getParam("ssid_manual", true)->value().isEmpty()) {
        ssid = request->getParam("ssid_manual", true)->value();
    } else if (request->hasParam("ssid_select", true)) {
        ssid = request->getParam("ssid_select", true)->value();
    }

    if (request->hasParam("password", true) && !ssid.isEmpty()) {
        String password = request->getParam("password", true)->value();
        show_message("Saving config...", ssid);
        wifiConfig.ssid = ssid;
        wifiConfig.password = password;
        if (save_wifi_config()) {
            request->send(200, "text/plain", "WiFi settings saved. Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            request->send(500, "text/plain", "Failed to save WiFi settings.");
        }
    } else {
        request->send(400, "text/plain", "Bad Request: SSID or password missing.");
    }
});
```

### STA Mode (Station) - Normal Operation
**File:** `src/wifi_manager.cpp` (lines 28-82)

**Process:**
1. Device boots and calls `load_wifi_config()`
2. If config exists, calls `connect_to_wifi()`
3. Uses credentials from global `wifiConfig` struct
4. Attempts connection with WiFi optimizations:
   - Disables power save mode
   - Sets auto-reconnect
   - Sets max TX power
5. Makes 2 connection attempts with 8-second timeout each
6. On success: displays IP address, logs connection
7. On failure: logs error, may trigger AP mode

**Code (wifi_manager.cpp, lines 28-82):**
```cpp
bool connect_to_wifi() {
    if (wifiConfig.ssid.isEmpty()) {
        log_message("Конфигурация WiFi отсутствует. Запуск точки доступа.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    
    // === ОПТИМИЗАЦИЯ ДЛЯ СТРИМИНГА ===
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    // 2 попытки подключения
    for (int attempt = 0; attempt < 2; attempt++) {
        show_message("Connecting to:", wifiConfig.ssid);
        log_message(formatString("Подключение к WiFi: %s (Попытка %d/2)", wifiConfig.ssid.c_str(), attempt + 1));
        
        WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        
        unsigned long startTime = millis();
        unsigned long lastCheckTime = millis();
        int timeout = WIFI_CONNECTION_TIMEOUT;
        
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
            if (millis() - lastCheckTime >= 200) {
                lastCheckTime = millis();
            }
            yield();
            delay(10);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            String ip = WiFi.localIP().toString();
            int rssi = WiFi.RSSI();
            
            show_ip_address(ip, 2000);
            log_message(formatString("✅ WiFi OK: %s (%d dBm)", ip.c_str(), rssi));
            
            if (rssi < -80) {
                log_message(formatString("⚠️ ВНИМАНИЕ: Слабый WiFi сигнал (%d dBm). Возможны проблемы со стримингом.", rssi));
            }
            
            wifiReconnectAttempts = 0;
            reset_inactivity_timer();
            return true;
        }
        
        show_message(wifiConfig.ssid, "Failed!", 1500);
        log_message(formatString("❌ Не удалось подключиться к %s", wifiConfig.ssid.c_str()));
        WiFi.disconnect();
        
        if (attempt < 1) {
            delay(WIFI_RETRY_DELAY);
        }
    }
    
    log_message("Все WiFi недоступны!");
    wifiReconnectAttempts++;
    return false;
}
```

---

## 8. Fallback Network Feature

### Function: `connect_to_wifi_fallback()`
**File:** `src/wifi_manager.cpp` (lines 26-56)

**Purpose:** Attempts to connect to backup WiFi network when primary network fails

**Process:**
1. Validates that fallback SSID is not empty
2. Configures WiFi in Station mode with same optimizations as primary
3. Attempts single connection to fallback network
4. Returns `true` on success, `false` on failure

**Code:**
```cpp
bool connect_to_wifi_fallback() {
    if (wifiConfig.fallbackSsid.isEmpty()) {
        log_message("❌ Fallback SSID пустой.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    show_message("Fallback WiFi:", wifiConfig.fallbackSsid);
    log_message("🔀 Попытка подключения к fallback сети: " + wifiConfig.fallbackSsid);

    WiFi.begin(wifiConfig.fallbackSsid.c_str(), wifiConfig.fallbackPassword.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
        yield();
        delay(10);
    }

    if (WiFi.status() == WL_CONNECTED) {
        log_message("✅ Подключено к fallback сети: " + wifiConfig.fallbackSsid);
        return true;
    }

    log_message("❌ Не удалось подключиться к fallback сети.");
    return false;
}
```

**Integration with Recovery System:**
- Fallback network is attempted in `handle_wifi_recovery()` function
- Triggered after manual reconnection to primary network fails
- New recovery state: `WIFI_FALLBACK_CONNECTING` (Level 3.5)
- If fallback succeeds, returns to `WIFI_OK` state
- If fallback fails, proceeds to reboot countdown

---

## 9. WiFi Recovery Flow (Updated with Fallback)

### Complete Timeline with Fallback

```
TIME    STATE                      ACTION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
0s      WIFI_OK                    Normal operation
        ↓ WiFi lost detected
        
0s      WIFI_AUTO_RECONNECTING     Waiting for auto-reconnect
40s     WIFI_AUTO_RECONNECTING     Auto-reconnect failed
        ↓
        
40s     WIFI_MANUAL_RECONNECTING   Manual reconnect to primary network
90s     WIFI_MANUAL_RECONNECTING   Manual reconnect failed
        ↓
        
90s     WIFI_FALLBACK_CONNECTING   Attempting fallback network
        ├─ SUCCESS → WIFI_OK
        └─ FAILURE → WIFI_FAILED_REBOOTING
        
95s     WIFI_FAILED_REBOOTING      Countdown to reboot
        ↓
100s    ESP.restart()              Reboot device
```

**Key Points:**
- Fallback is attempted only once per recovery cycle
- Static variable `fallbackAttempted` prevents infinite fallback loops
- Fallback provides additional 5-8 seconds before reboot
- If fallback succeeds, device continues normal operation with backup network

---

## 10. Important Notes (Updated)

1. **Dual Network Support:** Current implementation supports TWO WiFi networks (primary + fallback)
2. **Plain Text Storage:** Both primary and fallback passwords are stored unencrypted in `/wifi.json`
3. **No Runtime WiFi Update:** To change WiFi credentials (including fallback), must factory reset or manually edit `/wifi.json`
4. **Fallback Priority:** Primary network is always attempted first; fallback is last resort before reboot
5. **Manual Configuration Required:** Fallback credentials must be manually added to `/wifi.json` (no web UI support)
6. **Single Fallback Attempt:** Each recovery cycle attempts fallback only once to prevent loops

---

## 11. Factory Reset

WiFi credentials (including fallback) are deleted during factory reset:

**File:** `src/web_server_manager.cpp`

**Endpoints:**
- `/api/factory-reset` (requires password authentication)
- `/api/forgot-password-reset` (no authentication - for forgotten passwords)

**Process:**
```cpp
LittleFS.remove("/wifi.json");  // Deletes WiFi credentials
LittleFS.remove("/credentials.json");
LittleFS.remove("/stations.json");
LittleFS.remove("/state.json");
LittleFS.remove("/log.txt");
ESP.restart();
```

---

## 12. Security Considerations

### Current Implementation:
- ✅ WiFi passwords stored in LittleFS (internal flash)
- ✅ Not transmitted over network (only during initial setup in AP mode)
- ✅ Web interface requires authentication to access
- ❌ Passwords stored in **PLAIN TEXT** in `/wifi.json` (both primary and fallback)
- ❌ No encryption of WiFi credentials at rest
- ❌ Fallback credentials visible in same file as primary

### Potential Improvements:
1. Encrypt WiFi password in storage
2. Use ESP32 NVS (Non-Volatile Storage) with encryption
3. Implement secure credential storage using hardware encryption

---

## 13. Related Files

### Core Files:
- `src/config.h` - WiFiCredentials struct definition (including fallback fields)
- `src/config.cpp` - load_wifi_config() and save_wifi_config()
- `src/wifi_manager.cpp` - connect_to_wifi() and connect_to_wifi_fallback()
- `src/wifi_manager.h` - WiFi manager declarations and recovery states

### Web Interface:
- `data/ap_mode.html` - WiFi setup page (AP mode)
- `src/web_server_manager.cpp` - Web server endpoints for WiFi config

### Configuration:
- `platformio.ini` - Build configuration
- `/wifi.json` - Runtime storage (LittleFS)

---

## 14. Key Constants

**File:** `src/config.h`

```cpp
#define WIFI_CONNECTION_TIMEOUT     8000     // 8 seconds per attempt
#define WIFI_CHECK_INTERVAL         20000    // 20 seconds
#define WIFI_RETRY_DELAY            500      // 0.5 seconds between attempts

// WiFi Recovery
#define WIFI_AUTO_RECONNECT_CHECKS  4        // 4 checks × 10s = 40s
#define WIFI_AUTO_RECONNECT_INTERVAL 10000   // 10 seconds
#define WIFI_MANUAL_RECONNECT_TIMEOUT 50000  // 50 seconds
#define WIFI_REBOOT_COUNTDOWN       5        // 5 seconds countdown
```

---

## 15. Usage Example

### Reading Credentials:
```cpp
// In main.cpp setup()
if (load_wifi_config()) {
    // WiFi config loaded successfully
    if (connect_to_wifi()) {
        // Connected to WiFi
        start_web_server_sta();
    } else {
        // Connection failed
        setup_ap_mode();
    }
} else {
    // No WiFi config found
    setup_ap_mode();
}
```

### Writing Credentials (with Fallback):
```cpp
// In AP mode web handler or manual configuration
wifiConfig.ssid = "MyNetwork";
wifiConfig.password = "MyPassword123";
wifiConfig.fallbackSsid = "MyBackupNetwork";
wifiConfig.fallbackPassword = "BackupPass456";
wifiConfig.fallbackEnabled = true;

if (save_wifi_config()) {
    ESP.restart();  // Reboot to apply
}
```

### Manual /wifi.json Creation:
```json
{
  "ssid": "Home_WiFi",
  "password": "home_password",
  "fallback_ssid": "Mobile_Hotspot",
  "fallback_password": "hotspot_pass",
  "fallback_enabled": true
}
```

**Use Cases for Fallback:**
1. **Mobile Hotspot Backup:** Primary home WiFi, fallback to phone hotspot
2. **Dual Router Setup:** Primary 5GHz router, fallback to 2.4GHz router
3. **Office + Home:** Primary office WiFi, fallback to home network
4. **IoT Network Segregation:** Primary guest network, fallback to main network

---

## 16. Debugging

### Serial Output Examples:

**Success (with Fallback):**
```
LittleFS смонтирована.
Конфигурация WiFi загружена: MyNetwork
Fallback сеть: MyBackupNetwork
Подключение к WiFi: MyNetwork (Попытка 1/2)
✅ WiFi OK: 192.168.1.100 (-45 dBm)
```

**Primary Failed, Fallback Success:**
```
Конфигурация WiFi загружена: MyNetwork
Fallback сеть: MyBackupNetwork
Подключение к WiFi: MyNetwork (Попытка 1/2)
❌ Не удалось подключиться к MyNetwork
⚠️ WiFi потерян! Запуск авто-реконнекта...
🔄 Авто-реконнект проверка 1/4
...
🔀 Основная сеть недоступна. Переключение на fallback...
🔀 Попытка подключения к fallback сети: MyBackupNetwork
✅ Подключено к fallback сети: MyBackupNetwork
✅ WiFi восстановлен через fallback сеть!
```

**Both Networks Failed:**
```
Конфигурация WiFi загружена: MyNetwork
Fallback сеть: MyBackupNetwork
❌ Не удалось подключиться к MyNetwork
🔀 Попытка подключения к fallback сети: MyBackupNetwork
❌ Не удалось подключиться к fallback сети.
❌ Fallback подключение не удалось. Перезагрузка через 5 сек...
🔄 Перезагрузка через 4 сек...
...
🔄 ПЕРЕЗАГРУЗКА...
```

**No Config:**
```
Файл wifi.json не найден. Запуск в режиме AP.
Запуск в режиме точки доступа (AP)...
AP IP: 192.168.4.1
```

**Connection Failed:**
```
Конфигурация WiFi загружена: MyNetwork
Подключение к WiFi: MyNetwork (Попытка 1/2)
❌ Не удалось подключиться к MyNetwork
Подключение к WiFi: MyNetwork (Попытка 2/2)
❌ Не удалось подключиться к MyNetwork
Все WiFi недоступны!
```

---

## 17. Important Notes (Summary)

1. **Dual Network Support:** Current implementation supports TWO WiFi networks (primary + optional fallback)
2. **Plain Text Storage:** Both passwords are stored unencrypted in `/wifi.json`
3. **No Web UI for Fallback:** Fallback credentials must be manually added to `/wifi.json`
4. **Fallback Priority:** Primary network is always tried first, fallback is last resort
5. **Single Attempt:** Fallback is attempted only once per recovery cycle
6. **Manual Configuration:** To use fallback, manually edit `/wifi.json` or create custom AP mode UI

---

## End of Research Document

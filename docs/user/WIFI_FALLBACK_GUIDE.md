# WiFi Fallback Network - Quick Guide

## Overview

ESP32-C3 Internet Radio supports optional fallback (backup) WiFi network. If the primary network becomes unavailable, the device automatically switches to the fallback network to maintain internet connectivity.

---

## When to Use Fallback WiFi

**Common Use Cases:**

1. **Mobile Hotspot Backup**
   - Primary: Home WiFi
   - Fallback: Phone mobile hotspot
   - Use case: Continue streaming when home internet is down

2. **Dual Router Setup**
   - Primary: 5GHz router (faster, shorter range)
   - Fallback: 2.4GHz router (slower, longer range)
   - Use case: Better coverage throughout home

3. **Office + Home**
   - Primary: Office WiFi (during work hours)
   - Fallback: Home network
   - Use case: Portable radio that moves between locations

4. **Neighbor's WiFi (with permission)**
   - Primary: Your home WiFi
   - Fallback: Neighbor's guest network (shared password)
   - Use case: Extra redundancy

5. **IoT Network Segregation**
   - Primary: Guest/IoT network (isolated from main network)
   - Fallback: Main home network
   - Use case: Security + reliability balance

---

## How It Works

### Recovery Timeline

```
Primary WiFi Lost
       ↓
0-40s: Auto-reconnect attempts (waiting for primary to come back)
       ↓
40-90s: Manual reconnect attempts (force reconnect to primary)
       ↓
~90s: Switch to fallback network (if configured)
       ├─ SUCCESS → Continue operation on fallback network
       └─ FAILURE → Reboot device after 5 seconds
```

**Key Points:**
- Fallback is attempted only after primary network fails completely
- Device always tries to reconnect to primary first
- If fallback succeeds, device continues normal operation
- Fallback is attempted once per recovery cycle to prevent loops

---

## Configuration

### Manual Configuration (Recommended)

Edit `/wifi.json` file in LittleFS filesystem:

```json
{
  "ssid": "Home_WiFi_5G",
  "password": "primary_password_here",
  "fallback_ssid": "Home_WiFi_2G",
  "fallback_password": "fallback_password_here",
  "fallback_enabled": true
}
```

**How to Edit:**

#### Option 1: Via Serial/USB (PlatformIO)
```bash
# Upload modified wifi.json to LittleFS
# Place wifi.json in /data/ folder, then:
pio run --target uploadfs
```

#### Option 2: Via Web Interface (Future Feature)
Currently, there is no web UI for fallback configuration. Must manually edit the file.

#### Option 3: Direct Flash Access
Use ESP32 filesystem tools (esptool.py) to read/write LittleFS partition.

---

## Configuration Examples

### Example 1: Home WiFi + Mobile Hotspot

```json
{
  "ssid": "HomeNetwork",
  "password": "home_wifi_password",
  "fallback_ssid": "iPhone_Hotspot",
  "fallback_password": "hotspot_password",
  "fallback_enabled": true
}
```

**Benefit:** Continue streaming even if home internet goes down (using phone data).

---

### Example 2: 5GHz + 2.4GHz Routers

```json
{
  "ssid": "MyRouter_5GHz",
  "password": "router_password",
  "fallback_ssid": "MyRouter_2.4GHz",
  "fallback_password": "router_password",
  "fallback_enabled": true
}
```

**Benefit:** Better WiFi coverage (2.4GHz has longer range but slower speed).

---

### Example 3: No Fallback (Default)

```json
{
  "ssid": "MyWiFi",
  "password": "wifi_password"
}
```

**Behavior:** Device reboots after ~95 seconds if WiFi is lost.

---

### Example 4: Fallback Disabled

```json
{
  "ssid": "PrimaryNetwork",
  "password": "primary_password",
  "fallback_ssid": "BackupNetwork",
  "fallback_password": "backup_password",
  "fallback_enabled": false
}
```

**Behavior:** Fallback credentials are stored but not used (same as Example 3).

---

## Testing Fallback

### Test Procedure

1. **Setup:** Configure wifi.json with primary and fallback networks
2. **Flash:** Upload wifi.json to device (via `uploadfs`)
3. **Boot:** Device should connect to primary network
4. **Disconnect Primary:** Turn off primary router or block device MAC address
5. **Wait:** Device will attempt auto-reconnect for 40 seconds
6. **Manual Reconnect:** Device attempts manual reconnect for 50 seconds
7. **Fallback Switch:** At ~90 seconds, device switches to fallback network
8. **Verify:** Check Serial Monitor for "✅ Подключено к fallback сети"

### Expected Serial Output

```
Конфигурация WiFi загружена: PrimaryNetwork
Fallback сеть: BackupNetwork
Подключение к WiFi: PrimaryNetwork (Попытка 1/2)
✅ WiFi OK: 192.168.1.100 (-45 dBm)

... (device running normally) ...

⚠️ WiFi потерян! Запуск авто-реконнекта...
🔄 Авто-реконнект проверка 1/4
🔄 Авто-реконнект проверка 2/4
🔄 Авто-реконнект проверка 3/4
🔄 Авто-реконнект проверка 4/4
⚠️ Авто-реконнект не сработал. Запуск ручного переподключения...
🔧 Попытка ручного переподключения...
❌ Не удалось подключиться к PrimaryNetwork
🔀 Основная сеть недоступна. Переключение на fallback...
🔀 Попытка подключения к fallback сети: BackupNetwork
✅ Подключено к fallback сети: BackupNetwork
✅ WiFi восстановлен через fallback сеть!
```

---

## Troubleshooting

### Fallback Not Working

**Problem:** Device reboots instead of switching to fallback

**Checklist:**
1. ✅ `fallback_enabled` is set to `true`
2. ✅ `fallback_ssid` is not empty
3. ✅ `fallback_password` is correct
4. ✅ Fallback network is within range
5. ✅ ESP32 supports fallback network security type (WPA2-PSK recommended)
6. ✅ `/wifi.json` was uploaded to device (via `uploadfs`)

**Debug:** Check Serial Monitor for log messages:
- "Fallback сеть: [name]" - indicates fallback is loaded
- "🔀 Попытка подключения к fallback сети" - indicates fallback is attempted
- "❌ Fallback SSID пустой" - fallback is not configured

---

### Fallback Network Slow/Unstable

**Problem:** Streaming works but has buffering issues on fallback

**Solutions:**
1. **Check Signal Strength:** Move device closer to fallback router/hotspot
2. **Check Bitrate:** Use lower bitrate radio stations (128kbps instead of 320kbps)
3. **Mobile Data:** Ensure phone hotspot has good cellular signal
4. **Buffer Size:** Device has 128KB buffer for smooth playback (should handle weak WiFi)

---

### Device Stuck on Fallback

**Problem:** Primary network is back online, but device stays on fallback

**Behavior:** By design, device stays on fallback until next reboot or WiFi disconnect
- Fallback network works like primary network once connected
- No automatic "switch back" to primary network

**Solution:** Reboot device to reconnect to primary network:
- Via Web UI: Settings → System → Reboot
- Via Physical: Long press encoder button (5 seconds) to shutdown, then restart

---

## Security Considerations

⚠️ **Important:**
- Both primary and fallback passwords are stored in **plain text** in `/wifi.json`
- LittleFS partition is not encrypted by default
- Anyone with physical access to ESP32 can read WiFi credentials

**Best Practices:**
1. Use separate password for IoT devices (not your main WiFi password)
2. Use guest network for IoT devices when possible
3. Enable WPA3 if your router supports it (ESP32-C3 compatible)
4. Regularly update router firmware
5. Monitor connected devices on your router admin panel

---

## Future Improvements

**Planned Features (Not Yet Implemented):**
- [ ] Web UI for fallback configuration
- [ ] Automatic switch back to primary network when available
- [ ] Multiple fallback networks (priority order)
- [ ] WiFi credential encryption in LittleFS
- [ ] WPS support for easy setup
- [ ] WiFi network quality monitoring and auto-selection

**Contribute:** If you want to implement these features, submit a Pull Request!

---

## Related Documentation

- `README.md` - Main project documentation
- `WIFI_CREDENTIALS_RESEARCH.md` - Detailed WiFi credential storage research
- `WIFI_RECONNECT_FLOW_RESEARCH.md` - WiFi recovery state machine details
- `src/wifi_manager.cpp` - WiFi recovery implementation
- `src/config.cpp` - WiFi configuration loading/saving

---

## Quick Reference

### File Format (`/wifi.json`)

```json
{
  "ssid": "Required - Primary network SSID",
  "password": "Required - Primary network password",
  "fallback_ssid": "Optional - Backup network SSID",
  "fallback_password": "Optional - Backup network password",
  "fallback_enabled": false
}
```

### Serial Monitor Logs

| Log Message | Meaning |
|-------------|---------|
| `Fallback сеть: [name]` | Fallback credentials loaded successfully |
| `🔀 Попытка подключения к fallback сети` | Switching to fallback network |
| `✅ Подключено к fallback сети` | Fallback connection successful |
| `❌ Fallback SSID пустой` | Fallback not configured |
| `❌ Не удалось подключиться к fallback сети` | Fallback connection failed |

### Recovery Timeline

| Time | Action |
|------|--------|
| 0-40s | Auto-reconnect to primary |
| 40-90s | Manual reconnect to primary |
| ~90s | Switch to fallback (if configured) |
| ~95-103s | Reboot (if fallback also fails) |

---

## End of Guide

**Questions?** Open an issue on GitHub or check the main README.md.

# 📻 User Manual — ESP32-C3 Internet Radio

Complete guide for daily use and configuration.

---

## 🎮 Physical Controls

### Rotary Encoder (KY-040)

| Action | Function |
|--------|----------|
| Rotate clockwise | Volume UP (+2%) |
| Rotate counterclockwise | Volume DOWN (-2%) |
| Single click | Next station |
| Double click (fast) | Previous station |
| Long press (5 seconds) | Power OFF (deep sleep) |

**Wake from sleep:** Press encoder button once

---

## 🌐 Web Interface

### Access

1. **Find device IP:**
   - Look at OLED display (shows for 5 seconds on boot)
   - Check router DHCP client list
   - Serial Monitor output

2. **Open browser:**
   ```
   http://192.168.1.XXX/
   ```

3. **Login:**
   - Username and password set during first setup
   - Session remains active (auto-login)

### Main Page

**Controls:**
- Play/Pause button
- Volume slider (0-100%)
- Current station name
- Next/Previous buttons

**Station List:**
- Drag and drop to reorder
- Click station name to play
- Edit/Delete buttons

**System Status:**
- WiFi signal strength (dBm)
- Free RAM
- Uptime
- Current time

### Settings Page

**Display:**
- Rotation: 0° or 180°
- Visualizer style (0-8)

**Audio:**
- Volume control
- Station quick switch

**System:**
- Reboot button
- Factory reset (requires password)
- Logout

**WiFi:**
- Signal strength
- IP address
- Connected network name

---

## 📻 Managing Stations

### Add Station

1. Open web interface
2. Click **"Add Station"**
3. Enter:
   - **Name:** Station display name
   - **URL:** MP3 stream URL (HTTP/HTTPS)
4. Click **Save**

**Supported formats:** MP3 streaming (up to 320kbps)

### Edit Station

1. Click **Edit** button next to station
2. Modify name or URL
3. Click **Save**

### Delete Station

1. Click **Delete** button
2. Confirm deletion

### Reorder Stations

**Drag and drop** stations in the list to reorder.

Order is saved automatically.

### Import/Export

**Export:**
- Click **"Export Stations"** button
- Download `stations.json` file
- Backup for later use

**Import:**
- Click **"Import Stations"** button
- Select `stations.json` file
- Upload to device

---

## 🎨 Visualizer Modes

9 visualization styles available:

| ID | Name | Description |
|----|------|-------------|
| 0 | Bars | Classic vertical bars |
| 1 | Wave | Scrolling sine wave |
| 2 | Circle | Circular equalizer |
| 3 | Hexagon | Cyberpunk hexagons |
| 4 | Stars | Audio-reactive starfield |
| 5 | Mirror | Mirrored vertical bars |
| 6 | Lightning | Lightning between peaks |
| 7 | Tesseract | 4D hypercube animation |
| 8 | Plasma | Liquid plasma effect |

**Change via:**
- Web interface → Settings → Visualizer Style
- Or API: `POST /api/visualizer/style`

**Auto-activate:** After 15 seconds of inactivity (no encoder input)

**Return to info:** Rotate encoder or click button

---

## 📶 WiFi Management

### First Setup

1. Device creates AP: `ESP32-Radio-Setup`
2. Connect to it (no password)
3. Open: `http://192.168.4.1`
4. Click **"Scan Networks"**
5. Select your WiFi or enter manually
6. Enter password
7. Create web login credentials
8. Click **"Save & Reboot"**

### Change WiFi

**Factory reset** is required to change WiFi:

1. Open web interface
2. Go to Settings
3. Scroll to **Danger Zone**
4. Enter password
5. Click **"Factory Reset"**
6. Device reboots to AP mode
7. Follow first setup steps

### Fallback Network (Backup WiFi)

Device supports optional backup WiFi network.

**Manual configuration required** (edit `/wifi.json` file):

```json
{
  "ssid": "Primary_WiFi",
  "password": "primary_password",
  "fallback_ssid": "Backup_WiFi",
  "fallback_password": "backup_password",
  "fallback_enabled": true
}
```

See [WIFI_FALLBACK_GUIDE.md](WIFI_FALLBACK_GUIDE.md) for details.

---

## 🔋 Power Management

### Normal Operation

**Power consumption:**
- Idle (no audio): ~120mA @ 5V
- Playing audio: ~200-500mA @ 5V (depends on volume)
- Display active: +20mA

**Power source:**
- USB 3.0 port (1.5A recommended)
- Phone charger (2A recommended)
- Power bank (10000mAh = ~40 hours)

### Deep Sleep Mode

**Activate:** Long press encoder button (5 seconds)

**Sleep consumption:** ~10µA (microamps)

**Wake up:** Press encoder button once

**State saved:**
- Volume level
- Visualizer style
- Display rotation

**NOT saved:**
- Current station (always starts from station #1)

### Auto Power-Off

Not implemented. Device runs continuously when powered.

To save power: Use deep sleep or disconnect USB.

---

## 🛡️ Security

### Password Protection

Web interface requires login:
- Username and password
- SHA-256 hashed storage
- Session token (stays logged in)

**Change password:**
- Factory reset only (no password change feature yet)

### Forgotten Password

1. Open: `http://[IP]/forgot-password`
2. Click **"Factory Reset"** (no password required)
3. All data erased
4. Device reboots to AP mode

### Network Security

**Device security:**
- No HTTPS (HTTP only)
- No rate limiting on login
- Factory reset accessible without auth

**Recommendations:**
- Use on private LAN only
- Do NOT expose to internet
- Use strong web password
- Use separate guest network for IoT devices

---

## 🔧 Troubleshooting

### No Audio

**Check:**
- Volume is not 0% (rotate encoder)
- Station URL is valid (try another station)
- Speaker is connected
- MAX98357A GAIN pin connected to GND

**Try:**
- Reboot device
- Delete and re-add station
- Check Serial Monitor for errors

### WiFi Disconnects

**Automatic recovery:**
1. 0-40s: Auto-reconnect (wait)
2. 40-90s: Manual reconnect (device retries)
3. 90s: Switch to fallback (if configured)
4. 95-103s: Reboot device

**Permanent fix:**
- Move closer to router
- Use 2.4GHz WiFi (not 5GHz)
- Check router signal strength
- Configure fallback network

### Display Not Working

**Check:**
- I2C connections (GPIO8/9)
- Display is 3.3V (not 5V)
- I2C address is 0x3C

**Try:**
- Reboot device
- Re-flash firmware

### Encoder Not Responding

**Check:**
- Button (GPIO2), CLK (GPIO10), DT (GPIO3) connected
- Encoder is 5V powered

**Try:**
- Clean encoder contacts
- Replace encoder (may be defective)

### Web Interface Not Loading

**Check:**
- Correct IP address
- Device is connected to WiFi (check OLED)
- Web browser can access device (ping IP)

**Try:**
- Clear browser cache
- Try different browser
- Re-flash filesystem (`uploadfs`)

### Station Buffering / Stuttering

**Causes:**
- Weak WiFi signal (< -80 dBm)
- High bitrate stream (320kbps on slow WiFi)
- Station server overloaded

**Solutions:**
- Move closer to router
- Use lower bitrate station (128kbps)
- Try different station
- Configure fallback WiFi network

---

## 📊 System Information

### File System (LittleFS)

**Size:** 1440 KB

**Files:**
```
/wifi.json         WiFi credentials
/stations.json     Radio station list
/credentials.json  Web login (hashed)
/state.json        Volume, visualizer, rotation
/log.txt           System logs (20KB max)
```

**Backup:** Export stations via web interface

**Factory reset erases:** All files above

### Memory Usage

**RAM:** ~400KB total
- ~80KB free (idle)
- ~60KB free (playing audio)

**Flash:** 4MB total
- 2.5MB firmware
- 1.4MB filesystem
- 100KB system reserved

### Session Management

**Auto-clear:** Every 25 reboots
- Session token deleted
- Must login again

**Logout:** Manual via web interface

---

## 📖 Useful Links

- [Build Guide](BUILD_GUIDE.md) — Assembly instructions
- [Quick Start](QUICK_START.md) — Firmware flash guide
- [WiFi Fallback Guide](WIFI_FALLBACK_GUIDE.md) — Backup network setup
- [README](../../README.md) — Full project documentation
- [GitHub Issues](https://github.com/makepkg/ESP32-C3-Internet-Radio/issues) — Report bugs

---

## 📝 Station URL Examples

**Radio Paradise (High Quality):**
```
http://stream.radioparadise.com/mp3-320
```

**SomaFM Groove Salad:**
```
http://ice1.somafm.com/groovesalad-256-mp3
```

**BBC Radio 1:**
```
http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one
```

**Search for more:**
- https://www.radio-browser.info/
- https://streamurl.link/
- https://fmstream.org/

---

**Version:** v4.5  
**Last Updated:** 2025-01-08

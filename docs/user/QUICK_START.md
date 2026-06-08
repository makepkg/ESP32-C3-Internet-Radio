# ⚡ Quick Start ESP32 Radio

## 🎯 Flash in 3 Minutes

### Method 1: VS Code (EASIEST)

```
1. Install VS Code + "PlatformIO IDE" extension
2. File → Open Folder → select this folder
3. Connect ESP32 via USB
4. Click → button (Upload)           ← Flash firmware
5. Click 🔌 button (Upload Filesystem) ← Flash web interface
6. Done! Open http://192.168.4.1
```

### Method 2: Command Line

```bash
# Install PlatformIO
pip3 install platformio

# Flash
git clone https://github.com/makepkg/ESP32-C3-Internet-Radio.git
cd ESP32-C3-Internet-Radio
pio run --target upload     # 1. Code
pio run --target uploadfs   # 2. Web interface

# Console
pio device monitor -b 115200
```

---

## ⚠️ CRITICALLY IMPORTANT

**BOTH commands required!**

| What | Command | Why |
|-----|---------|-------|
| Code | `upload` | Logic, audio |
| Web | `uploadfs` | Web control panel |

**Without `uploadfs` there will be no web panel!**

---

## 🎮 Controls

### KY-040 Encoder:
- **Rotate** → Volume
- **Click** → Next station
- **Double click** → Previous
- **Hold 5s** → Power off

### Web Interface:
- First run: WiFi `ESP32-Radio-Setup` → http://192.168.4.1
- Register → Configure WiFi → Add stations
- After connection: http://[IP-address]/

---

## 🔧 Troubleshooting

### ❌ Port not found
```bash
# Linux
sudo usermod -a -G dialout $USER
# Reboot!

# Windows
# Install CH340/CP2102 driver
```

### ❌ Timed out waiting
1. Hold **BOOT** button on ESP32
2. Click Upload in VS Code
3. Release BOOT at "Connecting..."

### ❌ `pio` commands don't work in VS Code
**Use buttons at bottom of screen!**  
`pio` commands only work if PlatformIO is installed separately.

---

## 📚 Full Documentation

See [README.md](README.md)

# 🔧 Build Guide — Step by Step

Complete assembly instructions for ESP32-C3 Internet Radio.

---

## 📦 Parts List

### Required Components

| Item | Specification | Quantity | Price* |
|------|--------------|----------|--------|
| ESP32-C3-DevKitM-1 | RISC-V, 4MB Flash, WiFi 2.4GHz | 1 | $2-3 |
| OLED Display | 0.91" SSD1306, 128×32, I2C | 1 | $1-2 |
| KY-040 Encoder | Rotary encoder with button | 1 | $0.5 |
| MAX98357A Amplifier | I2S Class D, 3W | 1 | $1 |
| Speaker | 3W, 8Ω, 40-50mm | 1 | $1-2 |
| USB Cable | Type-C, quality | 1 | $1 |
| Wires | 0.25-0.5mm², colored | set | $1 |
| Solder | 60/40 or lead-free | - | - |

**Total: ~$8-10** (AliExpress/Taobao prices)

*Prices are approximate and vary by region

---

## 🛒 Where to Buy

### AliExpress (Worldwide Shipping)
Search terms:
- "ESP32-C3-DevKitM-1"
- "0.91 OLED 128x32 I2C white"
- "KY-040 rotary encoder"
- "MAX98357A amplifier module"
- "3W 8ohm speaker 40mm"

### Local Electronics Stores
- Faster delivery (same day)
- Can check compatibility before buying
- Usually more expensive

---

## 🔌 Wiring Diagram

```
ESP32-C3-DevKitM-1
┌─────────────────────────────────┐
│ 3V3 ────── OLED VCC             │
│ GND ─┬──── OLED GND             │
│      ├──── KY-040 GND           │
│      ├──── MAX98357A GND        │
│      └──── Speaker GND          │
│                                 │
│ 5V ──┬──── KY-040 VCC           │
│      └──── MAX98357A VIN        │
│                                 │
│ GPIO2  ──── KY-040 SW (button)  │
│ GPIO3  ──── KY-040 DT           │
│ GPIO10 ──── KY-040 CLK          │
│                                 │
│ GPIO4  ──── MAX98357A BCLK      │
│ GPIO5  ──── MAX98357A LRC       │
│ GPIO6  ──── MAX98357A DIN       │
│                                 │
│ GPIO8  ──── OLED SDA            │
│ GPIO9  ──── OLED SCL            │
└─────────────────────────────────┘
         │
         │ MAX98357A
         └──── SPK+ ──── Speaker + (red)
         └──── SPK- ──── Speaker - (black)
```

---

## 🔨 Assembly Steps

### Step 1: Check Components

Before soldering, verify:
- OLED display is **I2C** (4 pins: VCC, GND, SCL, SDA)
- ESP32-C3 has **built-in USB** (Type-C connector on board)
- MAX98357A has **speaker terminals** (screw or solder pads)

### Step 2: Prepare Wires

Cut wires to length:
- 10-15cm for connections between modules
- Use different colors:
  - **Red** = 3.3V / 5V
  - **Black** = GND
  - **Yellow/Green/Blue** = Signal pins

Strip 3-5mm of insulation from wire ends.

### Step 3: Solder OLED Display

**OLED → ESP32-C3:**
```
OLED VCC → ESP32 3V3
OLED GND → ESP32 GND
OLED SCL → ESP32 GPIO9
OLED SDA → ESP32 GPIO8
```

**Tips:**
- Solder on ESP32 pins (easier than board pads)
- Keep wires short to reduce interference
- Use heat shrink tubing or electrical tape

### Step 4: Solder KY-040 Encoder

**KY-040 → ESP32-C3:**
```
KY-040 VCC → ESP32 5V
KY-040 GND → ESP32 GND
KY-040 CLK → ESP32 GPIO10
KY-040 DT  → ESP32 GPIO3
KY-040 SW  → ESP32 GPIO2
```

### Step 5: Solder MAX98357A Amplifier

**MAX98357A → ESP32-C3:**
```
MAX98357A VIN  → ESP32 5V
MAX98357A GND  → ESP32 GND
MAX98357A BCLK → ESP32 GPIO4
MAX98357A LRC  → ESP32 GPIO5
MAX98357A DIN  → ESP32 GPIO6
```

**GAIN pin → GND** (6dB gain, recommended)

### Step 6: Connect Speaker

**Speaker → MAX98357A:**
```
Speaker + (red)   → MAX98357A SPK+
Speaker - (black) → MAX98357A SPK-
```

**No polarity** for speaker (can swap +/- if needed)

### Step 7: Check Connections

**Before powering on:**
- [ ] No shorts between 3.3V and GND
- [ ] No shorts between 5V and GND
- [ ] All signal pins connected correctly
- [ ] Solder joints are clean and solid

Use multimeter to check continuity.

---

## ⚠️ Important Notes

### Power Requirements

- **USB 2.0 port:** May not provide enough current for amplifier at high volume
- **USB 3.0 port:** Recommended (1.5A+)
- **Phone charger:** Best option (2A+)

### GAIN Setting

Connect MAX98357A **GAIN pin to GND** for 6dB gain:
- Quieter volume
- Lower power consumption
- Recommended for USB power

Leave **GAIN floating** for 9dB gain:
- Medium volume
- Moderate power consumption

Connect **GAIN to 3.3V** for 12dB gain:
- Louder volume
- Higher power consumption
- May cause brownout on weak USB ports

### System Pins (DO NOT USE!)

Never connect anything to:
- **GPIO0** — Boot mode selector
- **GPIO1** — USB data
- **GPIO7** — Flash SPI (will brick device!)
- **GPIO11-17** — Flash SPI
- **GPIO18-19** — USB data

---

## 🧪 Testing

### Step 1: Visual Check
- All wires properly soldered
- No exposed metal touching other connections
- Components secured (not hanging loose)

### Step 2: Power Test
1. Connect USB cable to ESP32-C3
2. **Do not connect to computer yet**
3. Check if any component gets hot (>5 seconds)
4. If anything gets hot → disconnect immediately, check for shorts

### Step 3: First Boot
1. Connect to computer
2. Open Serial Monitor (115200 baud)
3. Expected output:
   ```
   ESP32-C3 Internet Radio v4.5
   LittleFS смонтирована.
   Файл wifi.json не найден.
   Запуск в режиме AP...
   AP IP: 192.168.4.1
   ```

### Step 4: OLED Test
- Display should turn on
- Show "AP Mode" or IP address
- If blank → check I2C connections (GPIO8/GPIO9)

### Step 5: Encoder Test
- Rotate encoder → should feel clicks
- Press button → should hear/feel click
- If not working → check GPIO2/3/10 connections

### Step 6: Speaker Test
- Will test after firmware flash and WiFi setup

---

## 🐛 Troubleshooting

### OLED not working
- **Check I2C address:** Most displays use 0x3C (default in firmware)
- **Check wiring:** SDA=GPIO8, SCL=GPIO9
- **Check voltage:** Must be 3.3V (not 5V!)

### Encoder not responding
- **Check button:** GPIO2 must be connected
- **Check rotation:** GPIO3 (DT) and GPIO10 (CLK)
- **Check voltage:** Must be 5V

### No audio
- **Check I2S pins:** GPIO4/5/6
- **Check speaker:** Use multimeter to verify continuity
- **Check GAIN:** Try connecting to GND
- **Check volume:** May be set to 0% in firmware

### Device not booting
- **Check GPIO7:** Must NOT be connected to anything
- **Check USB cable:** Must support data (not charge-only)
- **Check power:** Try different USB port

### WiFi not connecting
- **Check frequency:** ESP32-C3 only supports 2.4GHz (not 5GHz)
- **Check password:** Case-sensitive, no spaces
- **Check signal:** Move closer to router

---

## 📦 Enclosure (Optional)

### Size
- Minimum: 100×70×40mm
- Recommended: 120×80×50mm

### Material
- Acrylic (transparent)
- 3D printed (PLA/PETG)
- Wooden box

### Mounting
- M3 screws for ESP32-C3
- Hot glue for speaker
- Double-sided tape for other modules

---

## ✅ Assembly Checklist

- [ ] All components purchased
- [ ] OLED soldered (3.3V)
- [ ] Encoder soldered (5V)
- [ ] Amplifier soldered (5V)
- [ ] Speaker connected
- [ ] No shorts verified
- [ ] First boot successful
- [ ] OLED displays text
- [ ] Encoder clicks work
- [ ] Ready for firmware flash

---

## 📖 Next Steps

After assembly:
1. [Flash firmware](QUICK_START.md) — Upload code to ESP32-C3
2. [Setup WiFi](WIFI_FALLBACK_GUIDE.md) — Connect to network
3. Add radio stations via web interface

---

**Questions?** See [README.md](../../README.md) or open an issue on GitHub.

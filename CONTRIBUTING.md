# Contributing to ESP32-C3 Internet Radio

Thanks for your interest! Contributions of all kinds are welcome — code, docs, bug reports, ideas.

## 🎯 Ways to contribute

### 🐛 Report bugs
Use the [Bug Report](https://github.com/makepkg/ESP32-C3-Internet-Radio/issues/new?template=bug_report.yml) template

### ✨ Request features
Use the [Feature Request](https://github.com/makepkg/ESP32-C3-Internet-Radio/issues/new?template=feature_request.yml) template

### 💬 Ask questions
Open a [Discussion](https://github.com/makepkg/ESP32-C3-Internet-Radio/discussions) — issues are for bugs and feature requests

### 🔨 Submit code
- Fix bugs
- Add visualizers
- Improve web UI
- Update documentation
- Optimize performance

## 🛠️ Development setup

### Prerequisites
- **PlatformIO Core** or **PlatformIO IDE** (VS Code extension)
- **Python 3.7+** (for PlatformIO)
- **ESP32-C3-DevKitM-1** board
- **USB cable** with data support

### Clone and build

```bash
# Clone repository
git clone https://github.com/makepkg/ESP32-C3-Internet-Radio.git
cd ESP32-C3-Internet-Radio

# Build firmware
pio run

# Build + flash firmware
pio run --target upload

# Build + flash web interface (LittleFS)
pio run --target uploadfs

# Serial monitor
pio device monitor -b 115200

# Clean build
pio run --target clean
```

### Quick test cycle

```bash
# 1. Make changes
# 2. Build and upload
pio run -t upload && pio device monitor

# Alternative: upload only changed files
pio run -t upload --upload-port /dev/ttyUSB0
```

## 📂 Project structure

```
ESP32-C3-Internet-Radio/
│
├── src/                      # Main C++ source files
│   ├── main.cpp             # Entry point, FreeRTOS task setup
│   ├── config.h/.cpp        # Global configuration and state
│   ├── audio_manager.h/.cpp # Audio streaming and decoding
│   ├── display_manager.h/.cpp  # OLED display control
│   ├── encoder_manager.h/.cpp  # Rotary encoder input
│   ├── web_server_manager.h/.cpp  # AsyncWebServer endpoints
│   ├── wifi_manager.h/.cpp  # WiFi connection and recovery
│   ├── visualizer_base.h    # Abstract visualizer interface
│   ├── visualizer_manager.h/.cpp  # Visualizer orchestration
│   ├── visualizer_styles.h  # Enum of all styles
│   │
│   └── visualizers/         # Individual visualizer implementations
│       ├── visualizer_bars.h/.cpp
│       ├── visualizer_wave.h/.cpp
│       ├── visualizer_plasma.h/.cpp
│       └── ... (11 total)
│
├── include/                 # Additional headers (optional)
├── lib/                     # Local libraries (currently empty)
├── data/                    # Web interface → flashed to LittleFS
│   ├── index.html          # Main web UI (single-page app)
│   └── ...                 # Other web assets
│
├── docs/                    # Documentation
│   ├── dev/                # Developer docs (API, architecture)
│   └── user/               # User docs (manual, quick start)
│
├── .github/                 # GitHub configuration
│   ├── workflows/          # CI/CD (build, lint, release)
│   └── ISSUE_TEMPLATE/     # Issue forms
│
├── platformio.ini          # PlatformIO build config
├── partitions.csv          # ESP32 flash partition layout
├── CHANGELOG.md            # Version history
├── CONTRIBUTING.md         # This file
├── LICENSE                 # MIT License
└── README.md               # Main project documentation
```

### Key files explained

| File | Purpose |
|------|---------|
| `main.cpp` | App entry, creates FreeRTOS tasks (audio, display, input, system) |
| `config.h` | Global vars, mutexes, enums, station list, WiFi credentials |
| `audio_manager.cpp` | ESP32-audioI2S integration, stream buffering, MP3/AAC decode |
| `display_manager.cpp` | OLED I2C control, renders UI and visualizers |
| `encoder_manager.cpp` | ISR for rotary encoder, debouncing, click detection |
| `web_server_manager.cpp` | API endpoints (`/api/stations`, `/api/volume`, etc.) |
| `wifi_manager.cpp` | WiFi connection, auto-reconnect, fallback network |
| `visualizer_*.cpp` | Audio-reactive visualizer implementations |
| `data/index.html` | Web interface (vanilla JS, no frameworks) |
| `platformio.ini` | Board config, libraries, build flags |
| `partitions.csv` | Flash layout: bootloader, app, LittleFS, NVS |

## 🎨 Adding a new visualizer

### Step-by-step guide

1. **Create visualizer class**
   ```bash
   # Create header
   touch src/visualizers/visualizer_myeffect.h
   # Create implementation
   touch src/visualizers/visualizer_myeffect.cpp
   ```

2. **Implement VisualizerBase interface**
   ```cpp
   // visualizer_myeffect.h
   #pragma once
   #include "../visualizer_base.h"
   
   class VisualizerMyEffect : public VisualizerBase {
   public:
       VisualizerMyEffect();
       void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override;
       const char* getName() override { return "My Effect"; }
   };
   ```

3. **Register in visualizer_styles.h**
   ```cpp
   enum VisualizerStyle : uint8_t {
       // ...existing styles
       STYLE_MY_EFFECT = 11
   };
   
   #define VISUALIZER_STYLE_COUNT 12  // Increment
   ```

4. **Register in visualizer_manager.cpp**
   ```cpp
   #include "visualizers/visualizer_myeffect.h"
   
   // In getVisualizer() switch:
   case STYLE_MY_EFFECT:
       visualizers[style] = new VisualizerMyEffect();
       break;
   
   // In getStyleName() switch:
   case STYLE_MY_EFFECT: return "My Effect";
   ```

5. **Test**
   ```bash
   pio run -t upload
   # Check web interface → should see new style
   ```

### Visualizer tips

- **IRAM_ATTR**: Add to `draw()` for performance
- **Use existing helpers**: `map()`, `constrain()`, fast lookup tables
- **Keep it fast**: Target ~30 FPS on OLED refresh
- **Audio-reactive**: Use `bands[]` array (frequency spectrum)
- **Smooth animations**: Add easing, avoid jerky movements

## 📐 Code style

### Formatting
We use `.clang-format` for automatic formatting:

```bash
# Check formatting
clang-format --dry-run src/**/*.cpp include/**/*.h

# Auto-fix formatting
clang-format -i src/**/*.cpp include/**/*.h
```

### Guidelines
- **Indentation**: 4 spaces (no tabs)
- **Line length**: Max 120 characters
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
  - Variables: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
  - Classes: `PascalCase`
  - Functions: `camelCase`
- **Comments**: 
  - Use `//` for single-line
  - Use `/* */` for multi-line
  - Document complex logic, especially ISR and FreeRTOS

### ESP32-specific
- **ISR functions**: Mark with `IRAM_ATTR`, keep short
- **FreeRTOS**: Defer work from ISR to tasks via queues
- **Memory**: Be mindful of RAM usage (ESP32-C3 has limited heap)
- **I2C/SPI**: Use mutexes for thread-safe access

### Static analysis

```bash
# Run cppcheck
cppcheck --enable=all --suppress=missingIncludeSystem src/ include/
```

## 📝 Commit messages

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

[optional body]

[optional footer]
```

### Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Formatting, missing semicolons, etc.
- `refactor`: Code restructuring without behavior change
- `perf`: Performance improvement
- `test`: Adding tests
- `build`: Build system changes
- `ci`: CI/CD changes
- `chore`: Maintenance tasks

### Examples

```bash
# Good commits
feat(visualizer): add plasma effect with wave interference
fix(encoder): prevent double-click on fast rotation
docs(wiring): update diagram for ESP32-C3-DevKitM-1
refactor(audio): split task into decoder and streamer
perf(display): optimize OLED refresh to 400 kHz I2C

# Scopes (optional)
visualizer, audio, display, web, wifi, encoder, config, build, docs
```

### Multi-line commit

```bash
git commit -m "feat(visualizer): add tesseract 4D hypercube effect" \
           -m "Implements rotating 4D wireframe with perspective projection" \
           -m "Closes #42"
```

## 🔄 Pull Request process

### Before submitting

1. **Create a branch**
   ```bash
   git checkout -b feat/my-new-feature
   ```

2. **Make changes and test**
   ```bash
   pio run -t upload
   # Test on real hardware
   ```

3. **Format code**
   ```bash
   clang-format -i src/**/*.cpp include/**/*.h
   ```

4. **Commit with conventional format**
   ```bash
   git commit -m "feat(audio): add AAC codec support"
   ```

5. **Push to your fork**
   ```bash
   git push origin feat/my-new-feature
   ```

### PR Checklist

When you submit a PR, ensure:

- [ ] **Code compiles** without warnings (`pio run`)
- [ ] **Tested on real hardware** (ESP32-C3-DevKitM-1)
- [ ] **No hardcoded credentials** or personal WiFi info
- [ ] **README updated** if behavior changed
- [ ] **CHANGELOG updated** if user-facing change
- [ ] **Code formatted** with clang-format
- [ ] **Commit messages** follow conventional format
- [ ] **PR description** explains what/why
- [ ] **Tests added** if applicable (unit tests welcome!)

### PR Review

- PRs require **at least 1 approval** before merge
- **CI checks must pass** (build + lint)
- Address review comments or explain why not
- Keep PRs **focused** — one feature/fix per PR
- Large changes? Consider **opening an issue first** to discuss

### After merge

- Delete your branch
- Update your fork
- Celebrate! 🎉

## 🧪 Testing

### Manual testing checklist

When testing changes:

- [ ] **Boot sequence** — no crashes, clean serial output
- [ ] **WiFi connection** — connects successfully
- [ ] **Audio streaming** — plays without glitches
- [ ] **OLED display** — renders correctly
- [ ] **Rotary encoder** — click/double-click/long-press work
- [ ] **Web interface** — all controls functional
- [ ] **Deep sleep** — wakes up correctly (if testing power)
- [ ] **Memory** — no leaks (check heap over time)

### Serial monitor tips

```bash
# Watch for errors
pio device monitor -b 115200 | grep -i error

# Save logs
pio device monitor -b 115200 > test.log

# Filter noise
pio device monitor -b 115200 | grep -v "D ("
```

## 📚 Resources

### Documentation
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [Arduino-ESP32 API](https://docs.espressif.com/projects/arduino-esp32/)
- [FreeRTOS Guide](https://www.freertos.org/Documentation/RTOS_book.html)

### Libraries used
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) — Audio decoding
- [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306) — OLED display
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) — Web server
- [ArduinoJson](https://arduinojson.org/) — JSON parsing

### Helpful tools
- **ESP32 Web Flash Tool**: [https://espressif.github.io/esptool-js/](https://espressif.github.io/esptool-js/)
- **OLED Font Generator**: [https://oleddisplay.squix.ch/](https://oleddisplay.squix.ch/)
- **Online Radio Streams**: [https://dir.xiph.org/](https://dir.xiph.org/)

## 💬 Communication

- 💡 **Ideas & Questions**: [Discussions](https://github.com/makepkg/ESP32-C3-Internet-Radio/discussions)
- 🐛 **Bug Reports**: [Issues](https://github.com/makepkg/ESP32-C3-Internet-Radio/issues)
- 📖 **Documentation**: [README](README.md) → [User Manual](docs/user/USER_MANUAL.md)

## 📜 License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

**Thank you for contributing! 🎉**

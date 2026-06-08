# Adding a New Visualizer

Step-by-step guide to add custom visualization mode.

---

## Architecture Overview

```
VisualizerBase (abstract class)
    ↓
VisualizerBars, VisualizerWave, etc. (implementations)
    ↓
VisualizerManager (manages all visualizers)
    ↓
main.cpp (renders active visualizer)
```

**Key files:**
- `src/visualizer_base.h` — Base class interface
- `src/visualizer_styles.h` — Enum with style IDs
- `src/visualizer_manager.cpp` — Factory pattern
- `src/visualizers/` — Individual visualizer implementations

---

## Step 1: Create Visualizer Files

Create two files in `src/visualizers/`:

**File:** `visualizer_yourname.h`
```cpp
#ifndef VISUALIZER_YOURNAME_H
#define VISUALIZER_YOURNAME_H

#include "../visualizer_base.h"
#include "../config.h"

class VisualizerYourName : public VisualizerBase {
private:
    // Your private state variables here
    int frameCounter;
    
public:
    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override;
    const char* getName() override { return "YourName"; }
};

#endif // VISUALIZER_YOURNAME_H
```

**File:** `visualizer_yourname.cpp`
```cpp
#include "visualizer_yourname.h"

void IRAM_ATTR VisualizerYourName::draw(Adafruit_SSD1306& display, int* bands, int bandCount) {
    // Your visualization logic here
    // Example: draw simple animation
    
    for (int i = 0; i < bandCount; i++) {
        int height = bands[i];
        int x = i * (SCREEN_WIDTH / bandCount);
        
        display.fillRect(x, SCREEN_HEIGHT - height, 
                        SCREEN_WIDTH / bandCount - 1, 
                        height, 
                        SSD1306_WHITE);
    }
}
```

---

## Step 2: Register in Enum

**File:** `src/visualizer_styles.h`

Add new style to enum:
```cpp
enum VisualizerStyle : uint8_t {
    STYLE_BARS = 0,
    STYLE_WAVE = 1,
    // ... existing styles ...
    STYLE_PLASMA = 8,
    STYLE_YOURNAME = 9,  // ← ADD HERE
};

#define VISUALIZER_STYLE_COUNT 10  // ← INCREMENT
```

---

## Step 3: Add to Factory

**File:** `src/visualizer_manager.cpp`

### 3.1: Include header
```cpp
#include "visualizers/visualizer_yourname.h"
```

### 3.2: Add case in `getVisualizer()`
```cpp
VisualizerBase* VisualizerManager::getVisualizer(VisualizerStyle style) {
    if (!visualizers[style]) {
        switch(style) {
            case STYLE_BARS:
                visualizers[style] = new VisualizerBars();
                break;
            // ... existing cases ...
            case STYLE_YOURNAME:
                visualizers[style] = new VisualizerYourName();  // ← ADD
                break;
        }
    }
    return visualizers[style];
}
```

### 3.3: Add name in `getStyleName()`
```cpp
const char* VisualizerManager::getStyleName(VisualizerStyle style) {
    switch(style) {
        case STYLE_BARS: return "Bars";
        // ... existing cases ...
        case STYLE_YOURNAME: return "Your Name";  // ← ADD
        default: return "Unknown";
    }
}
```

---

## Step 4: Update Documentation

**File:** `README.md`

Add to visualizer table:
```markdown
| 9 | **Your Name** | Description of your visualizer |
```

**File:** `CHANGELOG.md`

Add to latest version:
```markdown
### Added
- **YourName** visualizer — description
```

---

## Technical Details

### Input Data

**Function signature:**
```cpp
void draw(Adafruit_SSD1306& display, int* bands, int bandCount)
```

**Parameters:**
- `display` — Adafruit SSD1306 object (128×32 OLED)
- `bands` — Array of 16 amplitude values (0-32)
- `bandCount` — Always 16 (frequency bands)

**Band indices:**
```
bands[0-3]   → Low frequencies (bass)
bands[4-11]  → Mid frequencies
bands[12-15] → High frequencies (treble)
```

### Display Coordinates

```
(0,0) ────────────────► X (128)
│
│  ┌──────────────┐
│  │   OLED       │
│  │  128×32      │
│  └──────────────┘
▼
Y (32)
```

**Origin:** Top-left corner
**Color:** `SSD1306_WHITE` (only monochrome)

### Drawing Functions

**Pixels:**
```cpp
display.drawPixel(x, y, SSD1306_WHITE);
```

**Lines:**
```cpp
display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
display.drawFastHLine(x, y, width, SSD1306_WHITE);  // Horizontal
display.drawFastVLine(x, y, height, SSD1306_WHITE); // Vertical
```

**Rectangles:**
```cpp
display.drawRect(x, y, width, height, SSD1306_WHITE);       // Outline
display.fillRect(x, y, width, height, SSD1306_WHITE);       // Filled
display.fillRoundRect(x, y, w, h, radius, SSD1306_WHITE);   // Rounded
```

**Circles:**
```cpp
display.drawCircle(x, y, radius, SSD1306_WHITE);      // Outline
display.fillCircle(x, y, radius, SSD1306_WHITE);      // Filled
```

**Triangles:**
```cpp
display.drawTriangle(x1, y1, x2, y2, x3, y3, SSD1306_WHITE);
display.fillTriangle(x1, y1, x2, y2, x3, y3, SSD1306_WHITE);
```

### Performance Considerations

**Frame rate:** 60 FPS target (16ms per frame)

**Optimizations:**
1. Use `IRAM_ATTR` macro on `draw()` function (runs from RAM)
2. Minimize `display.clearDisplay()` calls
3. Draw only changed pixels when possible
4. Use lookup tables for sin/cos (see `visualizer_plasma.cpp`)
5. Avoid floating point math (use integer arithmetic)
6. Limit pixel iteration (skip pixels if needed)

**Memory:**
- Each visualizer instance uses ~100-500 bytes
- Lazy loading: created only when first used
- Deleted on style switch

---

## Example 1: Simple Bars with Color

```cpp
void IRAM_ATTR VisualizerSimple::draw(Adafruit_SSD1306& display, int* bands, int bandCount) {
    int bandWidth = SCREEN_WIDTH / bandCount;
    
    for (int i = 0; i < bandCount; i++) {
        int height = constrain(bands[i], 0, SCREEN_HEIGHT);
        int x = i * bandWidth;
        
        // Draw bar from bottom
        display.fillRect(
            x + 1,                      // X with gap
            SCREEN_HEIGHT - height,     // Y (from bottom)
            bandWidth - 2,              // Width with gap
            height,                     // Height
            SSD1306_WHITE
        );
    }
}
```

---

## Example 2: Wave with Smoothing

```cpp
class VisualizerWave : public VisualizerBase {
private:
    int history[128];  // Previous values
    int index;
    
public:
    VisualizerWave() : index(0) {
        memset(history, 0, sizeof(history));
    }
    
    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override {
        // Calculate average amplitude
        int avgAmp = 0;
        for (int i = 0; i < bandCount; i++) {
            avgAmp += bands[i];
        }
        avgAmp /= bandCount;
        
        // Store in circular buffer
        history[index] = avgAmp;
        index = (index + 1) % SCREEN_WIDTH;
        
        // Draw wave
        int prev = 0;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int histIndex = (index + x) % SCREEN_WIDTH;
            int y = SCREEN_HEIGHT - history[histIndex];
            
            if (x > 0) {
                display.drawLine(x - 1, prev, x, y, SSD1306_WHITE);
            }
            prev = y;
        }
    }
    
    const char* getName() override { return "Wave"; }
};
```

---

## Example 3: Frequency-Based Animation

```cpp
void IRAM_ATTR VisualizerFreq::draw(Adafruit_SSD1306& display, int* bands, int bandCount) {
    // Split frequencies
    int bass = (bands[0] + bands[1] + bands[2] + bands[3]) / 4;
    int mid = (bands[6] + bands[7] + bands[8] + bands[9]) / 4;
    int high = (bands[12] + bands[13] + bands[14] + bands[15]) / 4;
    
    // Bass: Draw bottom rectangle
    if (bass > 5) {
        int height = map(bass, 0, SCREEN_HEIGHT, 0, 10);
        display.fillRect(0, SCREEN_HEIGHT - height, SCREEN_WIDTH, height, SSD1306_WHITE);
    }
    
    // Mid: Draw center circle
    if (mid > 5) {
        int radius = map(mid, 0, SCREEN_HEIGHT, 2, 15);
        display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, radius, SSD1306_WHITE);
    }
    
    // High: Draw top sparkles
    if (high > 5) {
        for (int i = 0; i < SCREEN_WIDTH; i += 8) {
            if (high > random(0, SCREEN_HEIGHT)) {
                display.drawPixel(i, random(0, 8), SSD1306_WHITE);
            }
        }
    }
}
```

---

## Example 4: Using Lookup Tables

For expensive math operations (sin, cos), use lookup tables:

```cpp
class VisualizerCircular : public VisualizerBase {
private:
    // Sine table: 256 values, range -127...127
    static const int8_t sinTable[256];
    
    int8_t fastSin(uint8_t angle) {
        return sinTable[angle];
    }
    
    int8_t fastCos(uint8_t angle) {
        return sinTable[(angle + 64) & 0xFF];  // cos = sin(x + 90°)
    }
    
public:
    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override {
        int centerX = SCREEN_WIDTH / 2;
        int centerY = SCREEN_HEIGHT / 2;
        
        for (int i = 0; i < bandCount; i++) {
            uint8_t angle = (i * 256) / bandCount;
            int radius = bands[i] / 2;
            
            // Calculate point on circle
            int x = centerX + (fastCos(angle) * radius) / 127;
            int y = centerY + (fastSin(angle) * radius) / 127;
            
            // Draw line from center
            display.drawLine(centerX, centerY, x, y, SSD1306_WHITE);
        }
    }
    
    const char* getName() override { return "Circular"; }
};

// Define lookup table in .cpp file
const int8_t VisualizerCircular::sinTable[256] = {
    0, 3, 6, 9, 12, 15, 18, 21, ...  // 256 values
};
```

---

## Animation Patterns

### Gravity Effect (Smooth Fall)

```cpp
class VisualizerGravity : public VisualizerBase {
private:
    int currentHeights[16];
    
public:
    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override {
        for (int i = 0; i < bandCount; i++) {
            int target = bands[i];
            int current = currentHeights[i];
            
            // Instant rise, smooth fall
            if (target > current) {
                currentHeights[i] = target;
            } else {
                currentHeights[i] = max(0, current - 2);  // Gravity
            }
            
            // Draw bar
            int x = i * (SCREEN_WIDTH / bandCount);
            display.fillRect(x, SCREEN_HEIGHT - currentHeights[i], 
                           SCREEN_WIDTH / bandCount - 1, 
                           currentHeights[i], 
                           SSD1306_WHITE);
        }
    }
    
    const char* getName() override { return "Gravity"; }
};
```

### Scrolling Effect

```cpp
class VisualizerScroll : public VisualizerBase {
private:
    uint8_t buffer[128][32];  // Screen buffer
    
public:
    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override {
        // Shift buffer left
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH - 1; x++) {
                buffer[x][y] = buffer[x + 1][y];
            }
        }
        
        // Add new column from bands
        int avgAmp = 0;
        for (int i = 0; i < bandCount; i++) {
            avgAmp += bands[i];
        }
        avgAmp /= bandCount;
        
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            buffer[SCREEN_WIDTH - 1][y] = (y > (SCREEN_HEIGHT - avgAmp)) ? 1 : 0;
        }
        
        // Render buffer to display
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                if (buffer[x][y]) {
                    display.drawPixel(x, y, SSD1306_WHITE);
                }
            }
        }
    }
    
    const char* getName() override { return "Scroll"; }
};
```

---

## Testing

### Serial Monitor Test

Check if visualizer loads:
```
Visualizer loaded: Your Name
```

### Web Interface Test

1. Open `http://[IP]/`
2. Go to Settings
3. Select "Your Name" from dropdown
4. Play music
5. Verify animation on OLED

### API Test

```bash
curl -X POST http://192.168.1.100/api/visualizer/style \
  -d "style=9" \
  -b cookies.txt
```

---

## Common Patterns

### Peak Detection

```cpp
int peak = 0;
for (int i = 0; i < bandCount; i++) {
    if (bands[i] > peak) peak = bands[i];
}

if (peak > SCREEN_HEIGHT * 0.8) {
    // Draw special effect on peak
}
```

### Frequency Splitting

```cpp
int bass = (bands[0] + bands[1] + bands[2] + bands[3]) / 4;
int mid = (bands[7] + bands[8]) / 2;
int high = (bands[14] + bands[15]) / 2;
```

### Energy Calculation

```cpp
long totalEnergy = 0;
for (int i = 0; i < bandCount; i++) {
    totalEnergy += bands[i];
}
int avgEnergy = totalEnergy / bandCount;
```

### Frame Counter Animation

```cpp
private:
    int frameCounter;

void draw(...) {
    frameCounter++;
    
    // Animate every 4 frames
    if (frameCounter % 4 == 0) {
        // Update animation state
    }
}
```

---

## Memory & Performance

**Available RAM:** ~80KB free during operation

**Per-visualizer budget:**
- Static data: ~100 bytes (reasonable)
- Lookup tables: 256-1024 bytes (acceptable)
- Frame buffers: <4KB (avoid full screen buffer)

**CPU budget:**
- 16ms per frame (60 FPS)
- ~10ms for visualization (6ms reserved for audio)

**Optimization checklist:**
- [ ] Use `IRAM_ATTR` on `draw()` function
- [ ] Avoid `float` operations (use `int` math)
- [ ] Minimize `display` function calls
- [ ] Use lookup tables for trigonometry
- [ ] Skip pixels if performance drops
- [ ] Test with high bitrate stream (320kbps)

---

## File Checklist

Before committing:

- [ ] `src/visualizers/visualizer_yourname.h` created
- [ ] `src/visualizers/visualizer_yourname.cpp` created
- [ ] `src/visualizer_styles.h` updated (enum + count)
- [ ] `src/visualizer_manager.cpp` updated (include + factory + name)
- [ ] `README.md` updated (visualizer table)
- [ ] `CHANGELOG.md` updated (added entry)
- [ ] Tested on hardware (not just compiled)
- [ ] No compiler warnings
- [ ] Performance acceptable (60 FPS maintained)

---

## Reference: Existing Visualizers

| ID | Name | File | Complexity | Features |
|----|------|------|------------|----------|
| 0 | Bars | `visualizer_bars.*` | Simple | Gravity effect |
| 1 | Wave | `visualizer_wave.*` | Simple | Scrolling history |
| 2 | Circle | `visualizer_circle.*` | Medium | Polar coordinates |
| 3 | Hexagon | `visualizer_hexagon.*` | Medium | Geometric shapes |
| 4 | Stars | `visualizer_stars.*` | Medium | Particle system |
| 5 | Mirror | `visualizer_mirror.*` | Simple | Symmetry |
| 6 | Lightning | `visualizer_lightning.*` | Complex | Peak detection |
| 7 | Tesseract | `visualizer_tesseract.*` | Complex | 3D projection |
| 8 | Plasma | `visualizer_plasma.*` | Complex | Wave interference |

**Study these for examples!**

---

## Further Reading

- [Adafruit GFX Library](https://learn.adafruit.com/adafruit-gfx-graphics-library)
- [SSD1306 Reference](https://www.adafruit.com/product/326)
- [Audio Spectrum Analysis](https://en.wikipedia.org/wiki/Spectral_density)
- [Fast Fourier Transform](https://www.arduino.cc/reference/en/libraries/arduinofft/)

---

**Version:** 2025-01-08  
**Tested:** v4.5

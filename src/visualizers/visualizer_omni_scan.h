#pragma once
#include "visualizer_base.h"
#include "../config.h"

class VisualizerOmniScan : public VisualizerBase {
public:
    VisualizerOmniScan() : smoothAmp(0), scrollTimer(0) {
        for (int i = 0; i < SCREEN_WIDTH; i++) scrollBuf[i] = 0;
    }

    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override;
    const char* getName() override { return "Omni Scan"; }

private:
    uint8_t scrollBuf[SCREEN_WIDTH];
    int smoothAmp;
    uint8_t scrollTimer;
};

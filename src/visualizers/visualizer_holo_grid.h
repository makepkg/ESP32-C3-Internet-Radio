#pragma once
#include "../config.h"
#include "visualizer_base.h"

class VisualizerHoloGrid : public VisualizerBase {
   public:
    VisualizerHoloGrid() : pulseY(0), pulseTimer(0) {
        for (int i = 0; i < 16; i++) smoothBands[i] = 0;
    }

    void draw(Adafruit_SSD1306& display, int* bands, int bandCount) override;
    const char* getName() override {
        return "Holo Grid";
    }

   private:
    uint8_t pulseY;
    uint8_t pulseTimer;
    int smoothBands[16];
};

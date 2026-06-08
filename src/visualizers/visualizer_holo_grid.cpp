#include "visualizer_holo_grid.h"

void IRAM_ATTR VisualizerHoloGrid::draw(Adafruit_SSD1306& display, int* bands, int bandCount) {
    const int VP_X = SCREEN_WIDTH / 2;  // 64 — точка схода по центру
    const int VP_Y = 0;                 // точка схода вверху
    const int NUM_V = 9;                // радиальные линии
    const int NUM_H = 5;                // горизонтальные линии

    // Сглаживание полос (attack быстрый, decay плавный)
    for (int i = 0; i < bandCount && i < 16; i++) {
        if (bands[i] > smoothBands[i]) {
            smoothBands[i] = bands[i];
        } else {
            smoothBands[i] = (smoothBands[i] * 3 + bands[i]) / 4;
        }
    }

    // Средняя амплитуда для деформации горизонталей
    int avgAmp = 0;
    for (int i = 0; i < bandCount; i++) avgAmp += smoothBands[i];
    avgAmp /= bandCount;

    // === Радиальные линии (перспектива) ===
    for (int v = 0; v < NUM_V; v++) {
        int bottom_x = (v * (SCREEN_WIDTH - 1)) / (NUM_V - 1);
        
        // Привязываем полосу к радиальной линии
        int bandIdx = (v * bandCount) / NUM_V;
        if (bandIdx >= bandCount) bandIdx = bandCount - 1;
        
        int lift = constrain(smoothBands[bandIdx] / 3, 0, SCREEN_HEIGHT / 2);
        int bottom_y = SCREEN_HEIGHT - 1 - lift;
        
        display.drawLine(VP_X, VP_Y, bottom_x, bottom_y, SSD1306_WHITE);
    }

    // === Горизонтальные линии (перспективное расстояние) ===
    for (int h = 1; h <= NUM_H; h++) {
        // Квадратичный spacing для эффекта перспективы
        int y = VP_Y + (h * h * (SCREEN_HEIGHT - 1)) / (NUM_H * NUM_H);
        
        // Ширина линии пропорциональна расстоянию от VP
        int spread = ((y - VP_Y) * VP_X) / (SCREEN_HEIGHT - VP_Y);
        int x_left  = constrain(VP_X - spread, 0, SCREEN_WIDTH - 1);
        int x_right = constrain(VP_X + spread, 0, SCREEN_WIDTH - 1);
        
        // Деформация от аудио — дальние линии двигаются сильнее
        int deform = (avgAmp * h) / (NUM_H * 5);
        y = constrain(y - deform, VP_Y + 1, SCREEN_HEIGHT - 1);
        
        display.drawLine(x_left, y, x_right, y, SSD1306_WHITE);
    }

    // === Сканирующая линия (sci-fi эффект) ===
    pulseTimer++;
    if (pulseTimer >= 4) {
        pulseTimer = 0;
        pulseY++;
        if (pulseY >= SCREEN_HEIGHT) pulseY = 0;
    }
    
    int scanSpread = ((int)pulseY * VP_X) / SCREEN_HEIGHT;
    int sx_left  = constrain(VP_X - scanSpread, 0, SCREEN_WIDTH - 1);
    int sx_right = constrain(VP_X + scanSpread, 0, SCREEN_WIDTH - 1);
    display.drawLine(sx_left, pulseY, sx_right, pulseY, SSD1306_WHITE);
}

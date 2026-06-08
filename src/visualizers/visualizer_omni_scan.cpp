#include "visualizer_omni_scan.h"

void IRAM_ATTR VisualizerOmniScan::draw(Adafruit_SSD1306& display, int* bands, int bandCount) {
    // === Средняя амплитуда текущего кадра ===
    int totalAmp = 0;
    for (int i = 0; i < bandCount; i++) totalAmp += bands[i];
    int avgAmp = totalAmp / bandCount;

    // Сглаживание
    if (avgAmp > smoothAmp) smoothAmp = avgAmp;
    else smoothAmp = (smoothAmp * 3 + avgAmp) / 4;

    // === Скролл буфера влево на 1 пиксель ===
    scrollTimer++;
    if (scrollTimer >= 2) {
        scrollTimer = 0;
        for (int x = 0; x < SCREEN_WIDTH - 1; x++) {
            scrollBuf[x] = scrollBuf[x + 1];
        }
        // Новый столбик справа — высота от амплитуды
        int h = constrain((smoothAmp * (SCREEN_HEIGHT - 4)) / SCREEN_HEIGHT, 0, SCREEN_HEIGHT - 4);
        scrollBuf[SCREEN_WIDTH - 1] = (uint8_t)h;
    }

    // === Рисуем буфер ===
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int h = scrollBuf[x];
        if (h > 0) {
            int y = (SCREEN_HEIGHT - 2) - h;
            display.drawFastVLine(x, y, h, SSD1306_WHITE);
        }
    }

    // === Вертикальная линия сканера (правый край — где пишется новое) ===
    display.drawFastVLine(SCREEN_WIDTH - 1, 0, SCREEN_HEIGHT, SSD1306_WHITE);

    // === Горизонтальная baseline ===
    display.drawFastHLine(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, SSD1306_WHITE);

    // === Угловые sci-fi маркеры ===
    display.drawFastHLine(0, 0, 5, SSD1306_WHITE);
    display.drawFastVLine(0, 0, 3, SSD1306_WHITE);

    display.drawFastHLine(SCREEN_WIDTH - 5, 0, 5, SSD1306_WHITE);

    display.drawFastHLine(0, SCREEN_HEIGHT - 1, 5, SSD1306_WHITE);
    display.drawFastVLine(0, SCREEN_HEIGHT - 3, 3, SSD1306_WHITE);

    display.drawFastHLine(SCREEN_WIDTH - 5, SCREEN_HEIGHT - 1, 5, SSD1306_WHITE);
}

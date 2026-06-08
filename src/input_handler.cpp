#include "input_handler.h"

#include <Arduino.h>

#include <atomic>

#include "audio_manager.h"
#include "config.h"
#include "display_manager.h"
#include "system_manager.h"

// ATOMIC: используется в ISR (прерывании) энкодера
// Атомарные операции гарантируют memory barriers
std::atomic<int16_t> encoderPos(0);
std::atomic<bool> encoderChanged(false);

// Для обработки кнопки
unsigned long buttonPressStartTime = 0;
bool buttonHeld = false;
const unsigned long shortPressDuration = INPUT_SHORT_PRESS;
const unsigned long longPressDuration = INPUT_LONG_PRESS;
const unsigned long doubleClickMaxDelay = INPUT_DOUBLE_CLICK_DELAY;
unsigned long singleClickPendingTime = 0;  // 0, если клик не ожидает обработки

// === DEBOUNCE СОХРАНЕНИЯ ГРОМКОСТИ (защита от износа Flash) ===
static unsigned long lastVolumeSaveTime = 0;
static bool volumePendingSave = false;

void IRAM_ATTR encoderISR() {
    static unsigned long lastInterrupt = 0;
    unsigned long interruptTime = millis();

    // Debounce: игнорируем слишком частые прерывания
    if (interruptTime - lastInterrupt < INPUT_DEBOUNCE_TIME) {
        return;
    }
    lastInterrupt = interruptTime;

    // === QUADRATURE DECODER: правильное определение направления ===
    // Для KY-040: сохраняем предыдущее состояние для определения фазы
    static uint8_t lastState = 0;

    // Читаем текущее состояние обоих пинов
    uint8_t clk = digitalRead(ENCODER_CLK);
    uint8_t dt = digitalRead(ENCODER_DT);
    uint8_t currentState = (clk << 1) | dt;  // 2-битное состояние: CLK|DT

    // Таблица переходов для определения направления (Gray code)
    // По часовой: 00 -> 10 -> 11 -> 01 -> 00
    // Против часовой: 00 -> 01 -> 11 -> 10 -> 00

    // Определяем направление по переходу состояния
    if (lastState == 0b00 && currentState == 0b10) {  // 0 -> 2: по часовой
        encoderPos.fetch_add(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b00 && currentState == 0b01) {  // 0 -> 1: против часовой
        encoderPos.fetch_sub(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b10 && currentState == 0b11) {  // 2 -> 3: по часовой
        encoderPos.fetch_add(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b01 && currentState == 0b11) {  // 1 -> 3: против часовой
        encoderPos.fetch_sub(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b11 && currentState == 0b01) {  // 3 -> 1: по часовой
        encoderPos.fetch_add(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b11 && currentState == 0b10) {  // 3 -> 2: против часовой
        encoderPos.fetch_sub(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b01 && currentState == 0b00) {  // 1 -> 0: по часовой
        encoderPos.fetch_add(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    } else if (lastState == 0b10 && currentState == 0b00) {  // 2 -> 0: против часовой
        encoderPos.fetch_sub(1, std::memory_order_relaxed);
        encoderChanged.store(true, std::memory_order_relaxed);
    }
    // Игнорируем неправильные переходы (дребезг)

    lastState = currentState;
}

void setup_input() {
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    // ⚡ Прерывания на ОБА пина энкодера для лучшего детектирования
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);

    Serial.printf("✅ Энкодер инициализирован: CLK=%d, DT=%d, SW=%d (quadrature decoder)\n", ENCODER_CLK, ENCODER_DT,
                  ENCODER_SW);
}

void loop_input() {
    // --- Обработка поворота энкодера (громкость) ---
    // ATOMIC: чтение/запись без отключения прерываний благодаря atomic
    if (encoderChanged.load(std::memory_order_relaxed)) {
        // Читаем и сбрасываем флаг атомарно
        int16_t currentPos = encoderPos.load(std::memory_order_relaxed);
        encoderChanged.store(false, std::memory_order_relaxed);

        static int16_t lastPos = 0;
        if (currentPos != lastPos) {
            float new_volume = volume;
            if (currentPos > lastPos) {
                new_volume = min(VOLUME_MAX, volume + VOLUME_STEP);
            } else {
                new_volume = max(VOLUME_MIN, volume - VOLUME_STEP);
            }
            set_volume(new_volume);

            // ✅ НЕ сохраняем сразу - отложенное сохранение через 5 сек
            volumePendingSave = true;
            lastVolumeSaveTime = millis();
            lastPos = currentPos;
        }
    }

    // --- Отложенное сохранение громкости (debounce) ---
    if (volumePendingSave && millis() - lastVolumeSaveTime > VOLUME_SAVE_DELAY) {
        save_state();
        volumePendingSave = false;
        Serial.printf("💾 Громкость сохранена: %.2f (debounced)\n", volume);
    }

    // --- Обработка отложенного одиночного клика ---
    if (singleClickPendingTime != 0 && millis() - singleClickPendingTime > doubleClickMaxDelay) {
        next_station();
        singleClickPendingTime = 0;
    }

    // --- Обработка нажатия кнопки (смена станции / выключение) ---
    if (digitalRead(ENCODER_SW) == LOW) {
        // Кнопка нажата
        if (!buttonHeld) {
            // Это начало нажатия
            buttonHeld = true;
            buttonPressStartTime = millis();
        } else {
            // Кнопка все еще удерживается
            unsigned long holdDuration = millis() - buttonPressStartTime;
            if (holdDuration >= longPressDuration) {
                // Удерживали достаточно долго для выключения
                shutdown_system();
            } else if (holdDuration > INPUT_SHUTDOWN_ANIM_START) {
                // Показываем анимацию выключения
                float progress = (float)holdDuration / longPressDuration;
                show_shutdown_progress(progress);
            }
        }
    } else {
        // Кнопка отпущена
        if (buttonHeld) {
            buttonHeld = false;
            unsigned long holdDuration = millis() - buttonPressStartTime;
            if (holdDuration < shortPressDuration) {
                // Это было короткое нажатие

                // Приоритет: IP Display pause/resume
                if (is_ip_display_active()) {
                    Serial.printf("🔘 Button click on IP display (paused=%d)\n", is_ip_display_paused());
                    if (is_ip_display_paused()) {
                        resume_ip_display();
                    } else {
                        pause_ip_display();
                    }
                    singleClickPendingTime = 0;  // Отменяем отложенный клик
                } else if (singleClickPendingTime != 0) {
                    // Это двойное нажатие, т.к. первое еще не обработано
                    previous_station();
                    singleClickPendingTime = 0;  // Отменяем обработку одиночного
                } else {
                    // Это первое нажатие, ставим его в очередь
                    singleClickPendingTime = millis();
                }
            } else {
                // Это было длинное нажатие, но недостаточное для выключения.
                // Просто сбрасываем экран в инфо-режим и отменяем отложенный клик.
                singleClickPendingTime = 0;
                reset_inactivity_timer();
            }
        }
    }
}

// === ПРИНУДИТЕЛЬНОЕ СОХРАНЕНИЕ ГРОМКОСТИ ===
// Вызывается при shutdown для сохранения отложенной громкости
void force_save_volume() {
    if (volumePendingSave) {
        save_state();
        volumePendingSave = false;
        Serial.println("💾 Громкость сохранена при shutdown");
    }
}
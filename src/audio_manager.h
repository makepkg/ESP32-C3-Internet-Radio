#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>

#include <atomic>

// Состояния аудио системы
enum AudioState {
    AUDIO_IDLE,
    AUDIO_CONNECTING,
    AUDIO_STARTING,
    AUDIO_BUFFERING,  // Предварительная буферизация
    AUDIO_PLAYING,
    AUDIO_ERROR
};

extern int currentStation;
extern float volume;
extern AudioState audioState;

// 🎯 Флаг: аудио декодирует данные (приоритет над display)
// ATOMIC: используется между audio loop и main loop
extern std::atomic<bool> audio_decoding_active;

void setup_audio();
void loop_audio();
void next_station();
void previous_station();
void set_volume(float new_volume);
void force_audio_reset();

#endif  // AUDIO_MANAGER_H

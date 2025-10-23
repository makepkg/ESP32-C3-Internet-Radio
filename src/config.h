#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "visualizer_styles.h"

// ========================================
// ESP32-C3 GPIO ПИНЫ
// ========================================
// ДОСТУПНЫ ДЛЯ ИСПОЛЬЗОВАНИЯ: GPIO2-6, GPIO8-10, GPIO20-21
// 
// СИСТЕМНЫЕ (НЕДОСТУПНЫ):
//   GPIO0, GPIO1    - Boot Mode / USB D-
//   GPIO7           - Flash SPI (!НЕ ИСПОЛЬЗОВАТЬ!)
//   GPIO11-17       - Flash SPI (CLK, CMD, D0-D3)
//   GPIO18-19       - USB D+/D- (если нативный USB включен)
//
// ИСПОЛЬЗУЕМЫЕ В ПРОЕКТЕ:
//   GPIO2, 3, 10    - Энкодер KY-040
//   GPIO4, 5, 6     - I2S (MAX98357A)
//   GPIO8, 9        - I2C (OLED SSD1306)
//
// СВОБОДНЫЕ: GPIO20, GPIO21 (для будущих расширений)
// ========================================

// === OLED ДИСПЛЕЙ ===
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// === I2C КОНФИГУРАЦИЯ ===
#define I2C_FAST_MODE_FREQ          400000   // Частота I2C Fast Mode (400kHz) - оптимально для OLED
#define I2C_STANDARD_MODE_FREQ      100000   // Частота I2C Standard Mode (100kHz)
#define I2C_RETRY_INTERVAL          5000     // Интервал повторных попыток инициализации OLED (мс)

// === ПИНЫ ЭНКОДЕРА KY-040 ===
#define ENCODER_CLK  10
#define ENCODER_DT   3
#define ENCODER_SW   2

// === ПИНЫ I2S ДЛЯ MAX98357A ===
#define I2S_BCLK     4
#define I2S_LRC      5
#define I2S_DOUT     6

// === ТАЙМАУТЫ И ИНТЕРВАЛЫ (в миллисекундах) ===
#define AUDIO_BUFFER_SIZE           65536    // Размер буфера (64KB) - для медленного WiFi и buffer underflow
#define AUDIO_PREBUFFER_TIME        5000     // Предварительная буферизация перед стартом (5с)
#define AUDIO_BUFFER_LOW_THRESHOLD  15       // Порог низкого уровня буфера (15% = ~10KB)
#define AUDIO_CONNECTION_TIMEOUT    20000    // Таймаут подключения (увеличен)
#define AUDIO_STATE_DELAY           1000     // Задержка между состояниями аудио
#define AUDIO_START_TIMEOUT         10000    // Таймаут запуска потока (увеличен)
#define AUDIO_I2S_RETRY_INTERVAL    5000     // Интервал повторных попыток инициализации I2S (мс)

#define WIFI_CONNECTION_TIMEOUT     8000     // Таймаут подключения к WiFi
#define WIFI_CHECK_INTERVAL         20000    // Интервал проверки WiFi соединения
#define WIFI_RETRY_DELAY            500      // Задержка между попытками WiFi

// === WIFI RECOVERY (при потере связи во время работы) ===
#define WIFI_AUTO_RECONNECT_CHECKS  4        // Количество проверок перед ручным переподключением (4 × 10s = 40s)
#define WIFI_AUTO_RECONNECT_INTERVAL 10000   // Интервал проверки авто-реконнекта (10 сек)
#define WIFI_MANUAL_RECONNECT_TIMEOUT 50000  // Время на ручное переподключение (50 сек)
#define WIFI_REBOOT_COUNTDOWN       5        // Обратный отсчет перед перезагрузкой (секунды)

#define SYSTEM_STATUS_INTERVAL      60000    // Интервал вывода статуса системы

#define INPUT_DEBOUNCE_TIME         5        // Антидребезг энкодера (мс)
#define INPUT_SHORT_PRESS           300      // Короткое нажатие кнопки (мс)
#define INPUT_LONG_PRESS            5000     // Долгое нажатие для выключения (мс)
#define INPUT_DOUBLE_CLICK_DELAY    400      // Задержка для двойного клика (мс)
#define INPUT_SHUTDOWN_ANIM_START   1000     // Начало анимации выключения (мс)

#define DISPLAY_INACTIVITY_TIMEOUT  15000    // Таймаут неактивности для визуализатора
#define DISPLAY_INIT_DELAY          1000     // Задержка после инициализации дисплея
// 📝 Дисплей: 60 FPS (16ms), I2C 400 kHz, приоритет аудио (в main.cpp)

#define VOLUME_STEP                 0.02f    // Шаг изменения громкости
#define VOLUME_MIN                  0.0f     // Минимальная громкость
#define VOLUME_MAX                  1.0f     // Максимальная громкость
#define VOLUME_DEFAULT              0.05f    // Громкость по умолчанию
#define VOLUME_SAVE_DELAY           5000     // Задержка сохранения громкости в Flash (защита от износа)

// === ВИЗУАЛИЗАТОР КОНФИГУРАЦИЯ ===
#define VISUALIZER_SAMPLE_BUFFER    256      // Размер буфера аудио сэмплов для визуализатора
#define VISUALIZER_BANDS            16       // Количество частотных полос
#define VISUALIZER_MAX_AMPLITUDE    15000    // Максимальная амплитуда для map() (int16_t диапазон)
#define VISUALIZER_MIN_STACK        512      // Минимум свободного стека для безопасной обработки (байт)

// === FREERTOS КОНФИГУРАЦИЯ ===
#define COMMAND_QUEUE_SIZE          10       // Размер очереди команд между веб-сервером и основным loop

// === SERIAL / DEBUG КОНФИГУРАЦИЯ ===
#define SERIAL_BAUD_RATE            115200   // Скорость UART для Serial Monitor
#define SERIAL_INIT_DELAY           2000     // Задержка при старте для инициализации Serial (мс)

// === POWER MANAGEMENT ===
#define POWER_STABILIZATION_DELAY   100      // Задержка стабилизации питания после включения (мс)
#define POWER_BROWNOUT_EXTRA_DELAY  500      // Дополнительная задержка после brown-out detection (мс)

// === ОБЩИЕ ИНТЕРВАЛЫ ЛОГИРОВАНИЯ ===
#define WARNING_LOG_INTERVAL        5000     // Интервал повторного логирования предупреждений (мс)

// === WIFI НАСТРОЙКИ ===
struct WiFiCredentials {
    String ssid;
    String password;
};

extern WiFiCredentials wifiConfig;  // Одна WiFi сеть (без backup)

// === РАДИОСТАНЦИИ ===
#define MAX_RADIO_STATIONS 25  // Максимальное количество радиостанций

struct RadioStation {
    String name;
    String url;
    bool isAvailable;
};

extern std::vector<RadioStation> stations;
extern int totalStations;

// === ЗАЩИТА ОТ RACE CONDITION ===
// Мьютекс для защиты stations вектора от одновременного доступа
// из main loop (audio) и асинхронного веб-сервера
extern SemaphoreHandle_t stationsMutex;

// Макросы для безопасного доступа к stations
#define STATIONS_LOCK()   xSemaphoreTake(stationsMutex, portMAX_DELAY)
#define STATIONS_UNLOCK() xSemaphoreGive(stationsMutex)

// === УЧЕТНЫЕ ДАННЫЕ ВЕБ-ИНТЕРФЕЙСА ===
struct WebCredentials {
    String username;
    String passwordHash; // SHA256
};

extern WebCredentials webCredentials;

// === СТИЛЬ ВИЗУАЛИЗАТОРА ===
extern VisualizerStyle visualizerStyle;
extern uint8_t displayRotation; // 0=Normal, 2=Flipped 180°

// === СЕССИЯ И АВТОЛОГИН ===
#define SESSION_TOKEN_LENGTH        32       // Длина токена сессии (символов)

extern String sessionToken;      // Токен для автоматического входа
extern int rebootCounter;        // Счетчик перезагрузок (очистка каждые 25)

// === ФУНКЦИИ УПРАВЛЕНИЯ КОНФИГУРАЦИЕЙ ===
void setup_filesystem();
bool load_wifi_config();
bool save_wifi_config();
bool load_stations_config();
bool save_stations_config();
bool load_state();
void save_state();
bool load_credentials();
bool save_credentials(const String& username, const String& password);
bool verify_credentials(const String& username, const String& password);

#endif // CONFIG_H
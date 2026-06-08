#include "config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <mbedtls/md.h>

#include "audio_manager.h"
#include "display_manager.h"

// Глобальные переменные для хранения конфигураций
WiFiCredentials wifiConfig;  // Одна WiFi сеть
std::vector<RadioStation> stations;
int totalStations = 0;
WebCredentials webCredentials;

// 🛡️ Мьютекс для защиты stations от race condition
SemaphoreHandle_t stationsMutex = nullptr;

// Настройки визуализатора и дисплея
VisualizerStyle visualizerStyle = STYLE_BARS;
uint8_t displayRotation = 2;  // По умолчанию flipped

// Сессия и автологин
String sessionToken = "";
int rebootCounter = 0;

// --- Инициализация файловой системы ---
void setup_filesystem() {
    // 🛡️ Создаем мьютекс для защиты stations
    stationsMutex = xSemaphoreCreateMutex();
    if (stationsMutex == nullptr) {
        Serial.println("❌ КРИТИЧНО: Не удалось создать stationsMutex!");
        delay(5000);
        ESP.restart();
    }
    Serial.println("✅ stationsMutex создан успешно");

    if (!LittleFS.begin()) {
        Serial.println("Ошибка монтирования LittleFS!");
        Serial.println("Форматирование...");
        if (LittleFS.format()) {
            Serial.println("Файловая система отформатирована.");
            ESP.restart();
        } else {
            Serial.println("Ошибка форматирования.");
        }
    } else {
        Serial.println("LittleFS смонтирована.");
    }
}

// --- Управление конфигурацией WiFi ---

bool load_wifi_config() {
    File configFile = LittleFS.open("/wifi.json", "r");
    if (!configFile) {
        Serial.println("Файл wifi.json не найден. Запуск в режиме AP.");
        wifiConfig.ssid = "";
        wifiConfig.password = "";
        return false;  // Нет конфига - запускаем AP mode
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("Ошибка парсинга wifi.json: ");
        Serial.println(error.c_str());
        return false;
    }

    // Загружаем одну WiFi сеть
    JsonObject obj = doc.as<JsonObject>();
    wifiConfig.ssid = obj["ssid"].as<String>();
    wifiConfig.password = obj["password"].as<String>();
    wifiConfig.fallbackSsid = obj["fallback_ssid"] | String("");
    wifiConfig.fallbackPassword = obj["fallback_password"] | String("");
    wifiConfig.fallbackEnabled = obj["fallback_enabled"] | false;

    if (wifiConfig.ssid.isEmpty()) {
        Serial.println("Пустой SSID в wifi.json");
        return false;
    }

    Serial.println("Конфигурация WiFi загружена: " + wifiConfig.ssid);
    return true;
}

bool save_wifi_config() {
    File configFile = LittleFS.open("/wifi.json", "w");
    if (!configFile) {
        Serial.println("Не удалось создать wifi.json для записи.");
        return false;
    }

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();

    obj["ssid"] = wifiConfig.ssid;
    obj["password"] = wifiConfig.password;
    obj["fallback_ssid"] = wifiConfig.fallbackSsid;
    obj["fallback_password"] = wifiConfig.fallbackPassword;
    obj["fallback_enabled"] = wifiConfig.fallbackEnabled;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Ошибка записи в wifi.json.");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("Конфигурация WiFi сохранена: " + wifiConfig.ssid);
    return true;
}

// --- Управление конфигурацией станций ---

bool load_stations_config() {
    File configFile = LittleFS.open("/stations.json", "r");
    if (!configFile) {
        Serial.println("Файл stations.json не найден. Список станций пуст.");
        stations.clear();
        totalStations = 0;
        return false;  // Нет конфига - список пуст
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("Ошибка парсинга stations.json: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonArray array = doc.as<JsonArray>();
    stations.clear();
    for (JsonObject obj : array) {
        stations.push_back({
            obj["name"].as<String>(), obj["url"].as<String>(),
            true  // isAvailable всегда true при загрузке
        });
    }
    totalStations = stations.size();
    Serial.printf("✅ Загружено %d станций в порядке:\n", totalStations);
    for (size_t i = 0; i < stations.size(); i++) {
        Serial.printf("  %d. %s\n", i, stations[i].name.c_str());
    }
    return true;
}

bool save_stations_config() {
    File configFile = LittleFS.open("/stations.json", "w");
    if (!configFile) {
        Serial.println("Не удалось создать stations.json для записи.");
        return false;
    }

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& station : stations) {
        JsonObject obj = array.add<JsonObject>();
        obj["name"] = station.name;
        obj["url"] = station.url;
    }

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Ошибка записи в stations.json.");
        configFile.close();
        return false;
    }

    configFile.close();
    totalStations = stations.size();
    Serial.println("Конфигурация станций сохранена.");
    return true;
}

// --- Управление состоянием (громкость и станция) ---

bool load_state() {
    File configFile = LittleFS.open("/state.json", "r");
    if (!configFile) {
        Serial.println("Файл state.json не найден. Используются значения по умолчанию.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.println("Ошибка парсинга state.json.");
        return false;
    }

    volume = doc["volume"] | VOLUME_DEFAULT;
    // ✅ НЕ загружаем currentStation - всегда начинаем с первой станции (0)
    currentStation = 0;
    displayRotation = doc["displayRotation"] | 2;  // default: 2 (flipped)
    visualizerStyle = (VisualizerStyle)(doc["visualizerStyle"] | STYLE_BARS);

    // Загружаем сессию и счетчик перезагрузок
    sessionToken = doc["sessionToken"] | "";
    rebootCounter = doc["rebootCounter"] | 0;

    if (sessionToken.length() > 0) {
        Serial.printf("🔑 Session token загружен: %s...\n", sessionToken.substring(0, 8).c_str());
    } else {
        Serial.println("❌ Session token отсутствует.");
    }

    // Инкрементируем счетчик перезагрузок
    rebootCounter++;

    // Каждые 25 перезагрузок - очищаем сессию
    if (rebootCounter >= 25) {
        Serial.println("❗ 25 перезагрузок - очистка сессии!");
        sessionToken = "";
        rebootCounter = 0;
    }

    // НЕ сохраняем здесь! Сохранение в main.cpp после setup

    Serial.printf("Состояние загружено: Громкость=%.2f, Станция=%d, Поворот=%d, Стиль=%d, Reboot=%d/25\n", volume,
                  currentStation, displayRotation, visualizerStyle, rebootCounter);
    return true;
}

void save_state() {
    File configFile = LittleFS.open("/state.json", "w");
    if (!configFile) {
        Serial.println("Не удалось создать state.json для записи.");
        return;
    }

    JsonDocument doc;
    doc["volume"] = volume;
    // ✅ НЕ сохраняем currentStation - всегда начинаем с первой станции
    // doc["station"] = currentStation; // REMOVED
    doc["displayRotation"] = displayRotation;
    doc["visualizerStyle"] = (int)visualizerStyle;
    doc["sessionToken"] = sessionToken;
    doc["rebootCounter"] = rebootCounter;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Ошибка записи в state.json.");
    } else {
        Serial.println("Состояние сохранено.");
    }
    configFile.close();
}

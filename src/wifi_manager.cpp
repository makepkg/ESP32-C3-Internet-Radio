#include <WiFi.h>
#include "config.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "input_handler.h"
#include "log_manager.h"
#include "web_server_manager.h"
#include "string_utils.h"
#include "system_manager.h"
#include "audio_manager.h"

#include "log_manager.h"

static unsigned long lastWiFiCheck = 0;
static int wifiReconnectAttempts = 0;

// WiFi Recovery состояние и контекст
WiFiRecoveryState wifiRecoveryState = WIFI_OK;
static unsigned long wifiDisconnectTime = 0;
static int autoReconnectChecks = 0;
static unsigned long lastAutoReconnectCheck = 0;
static int rebootCountdown = 0;
static unsigned long lastRebootCountdownUpdate = 0;

static bool fallbackAttempted = false;

bool connect_to_wifi_fallback() {
    if (wifiConfig.fallbackSsid.isEmpty()) {
        log_message("❌ Fallback SSID пустой.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    show_message("Fallback WiFi:", wifiConfig.fallbackSsid);
    log_message("🔀 Попытка подключения к fallback сети: " + wifiConfig.fallbackSsid);

    WiFi.begin(wifiConfig.fallbackSsid.c_str(), wifiConfig.fallbackPassword.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
        yield();
        delay(10);
    }

    if (WiFi.status() == WL_CONNECTED) {
        log_message("✅ Подключено к fallback сети: " + wifiConfig.fallbackSsid);
        return true;
    }

    log_message("❌ Не удалось подключиться к fallback сети.");
    return false;
}

bool connect_to_wifi() {
    if (wifiConfig.ssid.isEmpty()) {
        log_message("Конфигурация WiFi отсутствует. Запуск точки доступа.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    
    // === ОПТИМИЗАЦИЯ ДЛЯ СТРИМИНГА ===
    WiFi.setSleep(false);              // Отключаем power save - нет задержек
    WiFi.setAutoReconnect(true);       // Автоматическое переподключение
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Максимальная мощность передатчика
    
    // 2 попытки подключения
    for (int attempt = 0; attempt < 2; attempt++) {
        show_message("Connecting to:", wifiConfig.ssid);
        log_message(formatString("Подключение к WiFi: %s (Попытка %d/2)", wifiConfig.ssid.c_str(), attempt + 1));
        
        WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        
        unsigned long startTime = millis();
        unsigned long lastCheckTime = millis();
        int timeout = WIFI_CONNECTION_TIMEOUT;
        
        // Неблокирующее ожидание подключения
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
            // Проверяем статус каждые 200ms
            if (millis() - lastCheckTime >= 200) {
                lastCheckTime = millis();
                // Можно добавить анимацию или обновление дисплея здесь
            }
            yield();  // Даем время WiFi стеку для обработки
            delay(10); // Минимальная задержка для предотвращения зависания watchdog
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            String ip = WiFi.localIP().toString();
            int rssi = WiFi.RSSI();
            
            show_ip_address(ip, 2000);
            log_message(formatString("✅ WiFi OK: %s (%d dBm)", ip.c_str(), rssi));
            
            // IP display will be handled in main loop()
            // User can click to pause/resume
            
            // Предупреждение о слабом сигнале
            if (rssi < -80) {
                log_message(formatString("⚠️ ВНИМАНИЕ: Слабый WiFi сигнал (%d dBm). Возможны проблемы со стримингом.", rssi));
            }
            
            wifiReconnectAttempts = 0;
            reset_inactivity_timer();
            return true;
        }
        
        show_message(wifiConfig.ssid, "Failed!", 1500);
        log_message(formatString("❌ Не удалось подключиться к %s", wifiConfig.ssid.c_str()));
        WiFi.disconnect();
        
        if (attempt < 1) {  // Если еще есть попытки
            delay(WIFI_RETRY_DELAY);
        }
    }
    
    log_message("Все WiFi недоступны!");
    wifiReconnectAttempts++;
    return false;
}

// === WiFi Recovery Система ===
// Вызывается из main loop каждую итерацию
void handle_wifi_recovery() {
    // Работает только в режиме STA
    if (systemState != STATE_STA) return;
    
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    
    // === УРОВЕНЬ 0: WiFi работает нормально ===
    if (wifiRecoveryState == WIFI_OK) {
        if (isConnected) {
            return; // Все хорошо
        }
        
        // WiFi потерян! Запускаем авто-реконнект
        wifiRecoveryState = WIFI_AUTO_RECONNECTING;
        wifiDisconnectTime = millis();
        autoReconnectChecks = 0;
        lastAutoReconnectCheck = millis();
        
        log_message("⚠️ WiFi потерян! Запуск авто-реконнекта...");
        show_message("WiFi lost", "Reconnecting...");
        return;
    }
    
    // === УРОВЕНЬ 1: Автоматическое переподключение (0-40s) ===
    if (wifiRecoveryState == WIFI_AUTO_RECONNECTING) {
        // Если переподключилось автоматически
        if (isConnected) {
            wifiRecoveryState = WIFI_OK;
            log_message("✅ WiFi восстановлен автоматически!");
            show_ip_address(WiFi.localIP().toString(), 2000);
            // IP display handled in main loop
            reset_inactivity_timer();
            return;
        }
        
        // Проверка каждые 10 секунд
        if (millis() - lastAutoReconnectCheck >= WIFI_AUTO_RECONNECT_INTERVAL) {
            lastAutoReconnectCheck = millis();
            autoReconnectChecks++;
            
            log_message(formatString("🔄 Авто-реконнект проверка %d/%d", autoReconnectChecks, WIFI_AUTO_RECONNECT_CHECKS));
            
            // Если прошло 4 проверки (40 сек) - переходим к ручному реконнекту
            if (autoReconnectChecks >= WIFI_AUTO_RECONNECT_CHECKS) {
                wifiRecoveryState = WIFI_MANUAL_RECONNECTING;
                log_message("⚠️ Авто-реконнект не сработал. Запуск ручного переподключения...");
                show_message("Manual", "Reconnect...");
                
                // Останавливаем аудио перед ручным реконнектом
                force_audio_reset();
            }
        }
        return;
    }
    
    // === УРОВЕНЬ 2: Ручное переподключение (40-90s) ===
    if (wifiRecoveryState == WIFI_MANUAL_RECONNECTING) {
        // Проверяем таймаут ручного реконнекта
        unsigned long elapsedTime = millis() - wifiDisconnectTime;
        
        if (elapsedTime < WIFI_MANUAL_RECONNECT_TIMEOUT + (WIFI_AUTO_RECONNECT_CHECKS * WIFI_AUTO_RECONNECT_INTERVAL)) {
            // Пытаемся переподключиться
            log_message("🔧 Попытка ручного переподключения...");
            
            if (connect_to_wifi()) {
                // Успешное переподключение!
                wifiRecoveryState = WIFI_OK;
                log_message("✅ WiFi восстановлен вручную!");
                reset_inactivity_timer();
                return;
            }
            
            // Не удалось - проверяем fallback
            if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty() && !fallbackAttempted) {
                wifiRecoveryState = WIFI_FALLBACK_CONNECTING;
                log_message("🔀 Основная сеть недоступна. Переключение на fallback...");
            } else {
                wifiRecoveryState = WIFI_FAILED_REBOOTING;
                rebootCountdown = WIFI_REBOOT_COUNTDOWN;
                lastRebootCountdownUpdate = millis();
                log_message(formatString("❌ Ручное переподключение не удалось. Перезагрузка через %d сек...", rebootCountdown));
            }
        } else {
            // Таймаут - проверяем fallback
            if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty() && !fallbackAttempted) {
                wifiRecoveryState = WIFI_FALLBACK_CONNECTING;
                log_message("🔀 Таймаут основной сети. Переключение на fallback...");
            } else {
                wifiRecoveryState = WIFI_FAILED_REBOOTING;
                rebootCountdown = WIFI_REBOOT_COUNTDOWN;
                lastRebootCountdownUpdate = millis();
                log_message(formatString("❌ Таймаут ручного переподключения. Перезагрузка через %d сек...", rebootCountdown));
            }
        }
        return;
    }
    
    // === УРОВЕНЬ 3: Перезагрузка (5s countdown) ===
    if (wifiRecoveryState == WIFI_FAILED_REBOOTING) {
        // Обратный отсчет каждую секунду
        if (millis() - lastRebootCountdownUpdate >= 1000) {
            lastRebootCountdownUpdate = millis();
            rebootCountdown--;
            
            // Обновляем дисплей
            show_message("WiFi Error!", "Rebooting in " + String(rebootCountdown) + "s");
            log_message(formatString("🔄 Перезагрузка через %d сек...", rebootCountdown));
            
            if (rebootCountdown <= 0) {
                log_message("🔄 ПЕРЕЗАГРУЗКА...");
                delay(500);
                ESP.restart();
            }
        }
        return;
    }

    // === УРОВЕНЬ 3.5: Fallback WiFi ===
    if (wifiRecoveryState == WIFI_FALLBACK_CONNECTING) {
        fallbackAttempted = true;
        
        if (connect_to_wifi_fallback()) {
            wifiRecoveryState = WIFI_OK;
            log_message("✅ WiFi восстановлен через fallback сеть!");
            show_ip_address(WiFi.localIP().toString(), 2000);
            reset_inactivity_timer();
        } else {
            wifiRecoveryState = WIFI_FAILED_REBOOTING;
            rebootCountdown = WIFI_REBOOT_COUNTDOWN;
            lastRebootCountdownUpdate = millis();
            log_message(formatString("❌ Fallback подключение не удалось. Перезагрузка через %d сек...", rebootCountdown));
        }
        return;
    }
}

void setup_ap_mode() {
    systemState = STATE_AP;
    IPAddress ap_ip;
    log_message("Запуск в режиме точки доступа (AP)...");
    WiFi.mode(WIFI_AP);
    
    String ap_ssid = "ESP32-Radio-Setup";
    WiFi.softAP(ap_ssid.c_str());
    
    ap_ip = WiFi.softAPIP();
    log_message("AP IP: " + ap_ip.toString());
    
    set_display_mode_ap(ap_ip.toString());
}
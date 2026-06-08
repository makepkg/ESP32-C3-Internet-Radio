# WiFi Reconnect Flow - Detailed Research

## Overview
This document maps the exact flow of WiFi reconnection attempts, state transitions, counters, and triggers for AP mode/reboot.

---

## State Variables & Counters

### Location: `src/wifi_manager.cpp` (Lines 14-23)

```cpp
static unsigned long lastWiFiCheck = 0;
static int wifiReconnectAttempts = 0;              // ← Counter for initial connection attempts

// WiFi Recovery состояние и контекст
WiFiRecoveryState wifiRecoveryState = WIFI_OK;    // ← Current recovery state
static unsigned long wifiDisconnectTime = 0;       // ← Timestamp when WiFi was lost
static int autoReconnectChecks = 0;                // ← Counter for auto-reconnect checks (0-4)
static unsigned long lastAutoReconnectCheck = 0;   // ← Timestamp of last check
static int rebootCountdown = 0;                    // ← Countdown timer (5 → 0)
static unsigned long lastRebootCountdownUpdate = 0; // ← Timestamp for countdown updates

static bool fallbackAttempted = false;             // ← Flag to prevent fallback loops
```

**Fallback Variables:**
- `fallbackAttempted` - Ensures fallback network is attempted only once per recovery cycle

---

## WiFi Recovery States (Enum)

### Location: `src/wifi_manager.h` (Lines 6-11)

```cpp
enum WiFiRecoveryState {
    WIFI_OK,                    // Level 0: WiFi working normally
    WIFI_AUTO_RECONNECTING,     // Level 1: Waiting for auto-reconnect (0-40s)
    WIFI_MANUAL_RECONNECTING,   // Level 2: Manual reconnect attempts (40-90s)
    WIFI_FALLBACK_CONNECTING,   // Level 3: Fallback network attempt (optional)
    WIFI_FAILED_REBOOTING       // Level 4: Countdown to reboot (5s)
};
```

**Note:** `WIFI_FALLBACK_CONNECTING` is only used when fallback WiFi credentials are configured.

---

## Initial Connection Flow (Boot Time)

### Location: `src/main.cpp` (Lines 90-124)

```cpp
// Line 90: Load WiFi config from /wifi.json
bool wifi_config_exists = load_wifi_config();

// Line 111: Try to connect if config exists
if (wifi_config_exists && connect_to_wifi()) {
    // SUCCESS: Enter STA mode
    systemState = STATE_STA;
    log_message("Подключено к WiFi. Запуск в режиме станции (STA).");
    setup_audio();
    start_web_server_sta();
} else {
    // FAILURE: Enter AP mode
    systemState = STATE_AP;
    log_message("Не удалось подключиться к WiFi. Запуск в режиме точки доступа (AP).");
    setup_ap_mode();              // ← AP MODE TRIGGERED HERE
    start_web_server_ap();
}
```

### Initial Connection Attempts

**Function:** `connect_to_wifi()` in `src/wifi_manager.cpp` (Lines 26-93)

**Process:**
1. **2 attempts** with 8-second timeout each (Line 40)
2. Each attempt:
   - Calls `WiFi.begin(ssid, password)` (Line 44)
   - Waits up to `WIFI_CONNECTION_TIMEOUT` (8000ms) (Line 48)
   - Checks status every 200ms (Line 51-54)
3. On success:
   - Resets `wifiReconnectAttempts = 0` (Line 75)
   - Returns `true`
4. On failure:
   - Increments `wifiReconnectAttempts++` (Line 90)
   - Returns `false`

**Code:**
```cpp
// Line 40: 2 попытки подключения
for (int attempt = 0; attempt < 2; attempt++) {
    show_message("Connecting to:", wifiConfig.ssid);
    log_message(formatString("Подключение к WiFi: %s (Попытка %d/2)", 
                wifiConfig.ssid.c_str(), attempt + 1));
    
    WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
    
    unsigned long startTime = millis();
    int timeout = WIFI_CONNECTION_TIMEOUT;  // 8000ms
    
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        yield();
        delay(10);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiReconnectAttempts = 0;  // ← Reset counter
        return true;
    }
    
    WiFi.disconnect();
    if (attempt < 1) {
        delay(WIFI_RETRY_DELAY);  // 500ms between attempts
    }
}

wifiReconnectAttempts++;  // ← Increment failure counter
return false;
```

---

## Runtime Recovery Flow (After Connection Loss)

### Main Function: `handle_wifi_recovery()`
**Location:** `src/wifi_manager.cpp` (Lines 96-202)
**Called from:** `main.cpp` loop() every iteration

### Entry Condition
```cpp
// Line 98: Only works in STA mode
if (systemState != STATE_STA) return;

bool isConnected = (WiFi.status() == WL_CONNECTED);
```

---

## LEVEL 0: Normal Operation

### Location: Lines 103-118

```cpp
if (wifiRecoveryState == WIFI_OK) {
    if (isConnected) {
        return; // All good, nothing to do
    }
    
    // WiFi LOST! Start auto-reconnect
    wifiRecoveryState = WIFI_AUTO_RECONNECTING;  // ← STATE TRANSITION
    wifiDisconnectTime = millis();               // ← Record disconnect time
    autoReconnectChecks = 0;                     // ← Reset check counter
    lastAutoReconnectCheck = millis();
    
    log_message("⚠️ WiFi потерян! Запуск авто-реконнекта...");
    show_message("WiFi lost", "Reconnecting...");
    return;
}
```

**Trigger:** WiFi connection lost (WiFi.status() != WL_CONNECTED)

**Actions:**
- Transition to `WIFI_AUTO_RECONNECTING`
- Reset `autoReconnectChecks = 0`
- Record disconnect timestamp

---

## LEVEL 1: Auto-Reconnect (0-40 seconds)

### Location: Lines 120-149

```cpp
if (wifiRecoveryState == WIFI_AUTO_RECONNECTING) {
    // Check if auto-reconnected
    if (isConnected) {
        wifiRecoveryState = WIFI_OK;  // ← BACK TO LEVEL 0
        log_message("✅ WiFi восстановлен автоматически!");
        show_ip_address(WiFi.localIP().toString(), 2000);
        reset_inactivity_timer();
        return;
    }
    
    // Check every 10 seconds
    if (millis() - lastAutoReconnectCheck >= WIFI_AUTO_RECONNECT_INTERVAL) {  // 10000ms
        lastAutoReconnectCheck = millis();
        autoReconnectChecks++;  // ← INCREMENT CHECK COUNTER
        
        log_message(formatString("🔄 Авто-реконнект проверка %d/%d", 
                    autoReconnectChecks, WIFI_AUTO_RECONNECT_CHECKS));
        
        // After 4 checks (40 seconds) → manual reconnect
        if (autoReconnectChecks >= WIFI_AUTO_RECONNECT_CHECKS) {  // 4 checks
            wifiRecoveryState = WIFI_MANUAL_RECONNECTING;  // ← STATE TRANSITION
            log_message("⚠️ Авто-реконнект не сработал. Запуск ручного переподключения...");
            show_message("Manual", "Reconnect...");
            
            force_audio_reset();  // Stop audio before manual reconnect
        }
    }
    return;
}
```

**Duration:** 0-40 seconds (4 checks × 10 seconds)

**Counter:** `autoReconnectChecks` (0 → 4)

**Success Path:**
- WiFi auto-reconnects → Return to `WIFI_OK`

**Failure Path:**
- After 4 checks (40s) → Transition to `WIFI_MANUAL_RECONNECTING`

**Constants (config.h):**
```cpp
#define WIFI_AUTO_RECONNECT_CHECKS  4        // 4 checks
#define WIFI_AUTO_RECONNECT_INTERVAL 10000   // 10 seconds per check
```

---

## LEVEL 2: Manual Reconnect (40-90 seconds)

### Location: Lines 152-196

```cpp
if (wifiRecoveryState == WIFI_MANUAL_RECONNECTING) {
    unsigned long elapsedTime = millis() - wifiDisconnectTime;
    
    // Check if within timeout window
    if (elapsedTime < WIFI_MANUAL_RECONNECT_TIMEOUT + 
                      (WIFI_AUTO_RECONNECT_CHECKS * WIFI_AUTO_RECONNECT_INTERVAL)) {
        // Try manual reconnect to primary network
        log_message("🔧 Попытка ручного переподключения...");
        
        if (connect_to_wifi()) {
            // SUCCESS!
            wifiRecoveryState = WIFI_OK;
            log_message("✅ WiFi восстановлен вручную!");
            reset_inactivity_timer();
            return;
        }
        
        // FAILED → Check fallback availability
        if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty() && !fallbackAttempted) {
            wifiRecoveryState = WIFI_FALLBACK_CONNECTING;  // ← Transition to fallback
            log_message("🔀 Основная сеть недоступна. Переключение на fallback...");
        } else {
            // No fallback or already attempted → Go to reboot
            wifiRecoveryState = WIFI_FAILED_REBOOTING;
            rebootCountdown = WIFI_REBOOT_COUNTDOWN;
            lastRebootCountdownUpdate = millis();
            log_message(formatString("❌ Ручное переподключение не удалось. Перезагрузка через %d сек...", 
                        rebootCountdown));
        }
    } else {
        // TIMEOUT → Check fallback availability
        if (wifiConfig.fallbackEnabled && !wifiConfig.fallbackSsid.isEmpty() && !fallbackAttempted) {
            wifiRecoveryState = WIFI_FALLBACK_CONNECTING;  // ← Transition to fallback
            log_message("🔀 Таймаут основной сети. Переключение на fallback...");
        } else {
            // No fallback or already attempted → Go to reboot
            wifiRecoveryState = WIFI_FAILED_REBOOTING;
            rebootCountdown = WIFI_REBOOT_COUNTDOWN;
            lastRebootCountdownUpdate = millis();
            log_message(formatString("❌ Таймаут ручного переподключения. Перезагрузка через %d сек...", 
                        rebootCountdown));
        }
    }
    return;
}
```

**Duration:** 40-90 seconds total
- Starts at 40s (after auto-reconnect phase)
- Timeout window: 50 seconds (`WIFI_MANUAL_RECONNECT_TIMEOUT`)
- Total elapsed time check: 40s + 50s = 90s

**Process:**
1. Calls `connect_to_wifi()` to reconnect to primary network (2 attempts × 8s each)
2. If successful → Return to `WIFI_OK`
3. If failed AND fallback configured → Transition to `WIFI_FALLBACK_CONNECTING`
4. If failed AND no fallback → Transition to `WIFI_FAILED_REBOOTING`

**Fallback Check Conditions:**
- `wifiConfig.fallbackEnabled` is `true`
- `wifiConfig.fallbackSsid` is not empty
- `fallbackAttempted` is `false` (not yet tried this cycle)

**Constants (config.h):**
```cpp
#define WIFI_MANUAL_RECONNECT_TIMEOUT 50000  // 50 seconds
```

**Timeout Calculation:**
```cpp
elapsedTime < 50000 + (4 * 10000)  // 50s + 40s = 90s total
```

---

## LEVEL 3: Fallback Network (Optional, ~90-98 seconds)

### Location: Lines 231-245

```cpp
if (wifiRecoveryState == WIFI_FALLBACK_CONNECTING) {
    fallbackAttempted = true;  // ← Mark as attempted to prevent loops
    
    if (connect_to_wifi_fallback()) {
        // Fallback connection successful!
        wifiRecoveryState = WIFI_OK;  // ← BACK TO LEVEL 0
        log_message("✅ WiFi восстановлен через fallback сеть!");
        show_ip_address(WiFi.localIP().toString(), 2000);
        reset_inactivity_timer();
    } else {
        // Fallback failed → Go to reboot
        wifiRecoveryState = WIFI_FAILED_REBOOTING;  // ← STATE TRANSITION
        rebootCountdown = WIFI_REBOOT_COUNTDOWN;    // ← Set countdown = 5
        lastRebootCountdownUpdate = millis();
        log_message(formatString("❌ Fallback подключение не удалось. Перезагрузка через %d сек...", 
                    rebootCountdown));
    }
    return;
}
```

**Duration:** ~8 seconds (single connection attempt with timeout)

**Process:**
1. Sets `fallbackAttempted = true` to prevent infinite loops
2. Calls `connect_to_wifi_fallback()` function
3. If successful → Return to `WIFI_OK` with fallback network
4. If failed → Transition to `WIFI_FAILED_REBOOTING`

**Success Path:**
- Device continues normal operation using fallback network
- All features work as if connected to primary network
- Logs indicate fallback network is active

**Failure Path:**
- Proceeds to reboot countdown (Level 4)

**Note:** Fallback is attempted only once per recovery cycle. If device reboots and reconnects, `fallbackAttempted` is reset to `false`.

---

## LEVEL 4: Reboot Countdown (5 seconds)

### Location: Lines 184-202

```cpp
if (wifiRecoveryState == WIFI_FAILED_REBOOTING) {
    // Countdown every second
    if (millis() - lastRebootCountdownUpdate >= 1000) {
        lastRebootCountdownUpdate = millis();
        rebootCountdown--;  // ← DECREMENT COUNTDOWN (5 → 0)
        
        // Update display
        show_message("WiFi Error!", "Rebooting in " + String(rebootCountdown) + "s");
        log_message(formatString("🔄 Перезагрузка через %d сек...", rebootCountdown));
        
        if (rebootCountdown <= 0) {
            log_message("🔄 ПЕРЕЗАГРУЗКА...");
            delay(500);
            ESP.restart();  // ← REBOOT TRIGGERED HERE (Line 198)
        }
    }
    return;
}
```

**Duration:** 5 seconds

**Counter:** `rebootCountdown` (5 → 0)

**Process:**
1. Countdown decrements every 1 second
2. Display shows: "Rebooting in Xs"
3. When countdown reaches 0 → `ESP.restart()` is called

**Constants (config.h):**
```cpp
#define WIFI_REBOOT_COUNTDOWN 5  // 5 seconds
```

---

## Complete Timeline

**Without Fallback:**
```
TIME    STATE                      COUNTER                ACTION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
0s      WIFI_OK                    -                      Normal operation
        ↓ WiFi lost detected
        
0s      WIFI_AUTO_RECONNECTING     autoReconnectChecks=0  Waiting for auto-reconnect
10s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=1  Check 1/4
20s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=2  Check 2/4
30s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=3  Check 3/4
40s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=4  Check 4/4
        ↓ Auto-reconnect failed
        
40s     WIFI_MANUAL_RECONNECTING   -                      Manual reconnect attempt
        ↓ Manual reconnect failed OR timeout at 90s
        
90s     WIFI_FAILED_REBOOTING      rebootCountdown=5      Countdown starts
91s     WIFI_FAILED_REBOOTING      rebootCountdown=4      4 seconds...
92s     WIFI_FAILED_REBOOTING      rebootCountdown=3      3 seconds...
93s     WIFI_FAILED_REBOOTING      rebootCountdown=2      2 seconds...
94s     WIFI_FAILED_REBOOTING      rebootCountdown=1      1 second...
95s     WIFI_FAILED_REBOOTING      rebootCountdown=0      ESP.restart() ← REBOOT!
```

**With Fallback Network:**
```
TIME    STATE                      COUNTER                ACTION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
0s      WIFI_OK                    -                      Normal operation (primary network)
        ↓ WiFi lost detected
        
0s      WIFI_AUTO_RECONNECTING     autoReconnectChecks=0  Waiting for auto-reconnect
10s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=1  Check 1/4
20s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=2  Check 2/4
30s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=3  Check 3/4
40s     WIFI_AUTO_RECONNECTING     autoReconnectChecks=4  Check 4/4
        ↓ Auto-reconnect failed
        
40s     WIFI_MANUAL_RECONNECTING   -                      Manual reconnect to primary
        ↓ Manual reconnect failed
        
90s     WIFI_FALLBACK_CONNECTING   fallbackAttempted=true Attempting fallback network
        ├─ SUCCESS → WIFI_OK (fallback network active)
        └─ FAILURE ↓
        
98s     WIFI_FAILED_REBOOTING      rebootCountdown=5      Countdown starts
99s     WIFI_FAILED_REBOOTING      rebootCountdown=4      4 seconds...
100s    WIFI_FAILED_REBOOTING      rebootCountdown=3      3 seconds...
101s    WIFI_FAILED_REBOOTING      rebootCountdown=2      2 seconds...
102s    WIFI_FAILED_REBOOTING      rebootCountdown=1      1 second...
103s    WIFI_FAILED_REBOOTING      rebootCountdown=0      ESP.restart() ← REBOOT!
```

**Total Time to Reboot:**
- Without fallback: ~95 seconds
- With fallback (if fails): ~103 seconds

---

## Success Paths (Return to WIFI_OK)

### Path 1: Auto-Reconnect Success
```
WIFI_OK → WIFI_AUTO_RECONNECTING → WIFI_OK
```
**Location:** Lines 122-129
**Condition:** WiFi auto-reconnects within 40 seconds
**Network:** Primary network

### Path 2: Manual Reconnect Success
```
WIFI_OK → WIFI_AUTO_RECONNECTING → WIFI_MANUAL_RECONNECTING → WIFI_OK
```
**Location:** Lines 161-166
**Condition:** Manual `connect_to_wifi()` succeeds
**Network:** Primary network

### Path 3: Fallback Network Success (NEW)
```
WIFI_OK → WIFI_AUTO_RECONNECTING → WIFI_MANUAL_RECONNECTING → WIFI_FALLBACK_CONNECTING → WIFI_OK
```
**Location:** Lines 231-240
**Condition:** Fallback configured and `connect_to_wifi_fallback()` succeeds
**Network:** Fallback network (backup)

---

## Failure Path (Reboot)

**Without Fallback:**
```
WIFI_OK → WIFI_AUTO_RECONNECTING → WIFI_MANUAL_RECONNECTING → WIFI_FAILED_REBOOTING → ESP.restart()
```

**With Fallback (if fallback also fails):**
```
WIFI_OK → WIFI_AUTO_RECONNECTING → WIFI_MANUAL_RECONNECTING → WIFI_FALLBACK_CONNECTING → WIFI_FAILED_REBOOTING → ESP.restart()
```

**Total Time to Reboot:**
- Without fallback: ~95 seconds (40s + 50s + 5s)
- With fallback: ~103 seconds (40s + 50s + 8s + 5s)

---

## AP Mode Trigger

**Location:** `src/main.cpp` (Lines 111-124)

**Trigger:** Initial boot when:
1. `/wifi.json` doesn't exist (`load_wifi_config()` returns false), OR
2. `connect_to_wifi()` fails (after 2 attempts)

**Code:**
```cpp
if (wifi_config_exists && connect_to_wifi()) {
    systemState = STATE_STA;
    // Normal operation
} else {
    systemState = STATE_AP;
    setup_ap_mode();  // ← AP MODE TRIGGERED
    start_web_server_ap();
}
```

**Important:** AP mode is ONLY triggered at boot time, NOT during runtime recovery!

---

## Key Differences: Boot vs Runtime

### Boot Time (main.cpp setup)
- Uses `connect_to_wifi()` directly
- 2 attempts with 8s timeout each (16s total)
- On failure → AP mode
- Counter: `wifiReconnectAttempts` (not actively used)

### Runtime (handle_wifi_recovery in loop)
- 4-level state machine
- Total recovery time: ~95 seconds
- On failure → Reboot (NOT AP mode)
- Counters: `autoReconnectChecks`, `rebootCountdown`

---

## Audio Behavior During Recovery

**Location:** `src/audio_manager.cpp` (Line 294)

```cpp
// Block new connections during WiFi recovery
if (wifiRecoveryState != WIFI_OK) {
    if (audioState != AUDIO_IDLE) {
        cleanup_audio();
    }
    return;
}
```

**Behavior:**
- Audio is stopped when `wifiRecoveryState != WIFI_OK`
- Prevents audio attempts during reconnection
- Explicitly stopped before manual reconnect (Line 146)

---

## Summary Table

| Level | State                      | Duration | Counter                | Exit Condition                    | Next State           |
|-------|----------------------------|----------|------------------------|-----------------------------------|----------------------|
| 0     | WIFI_OK                    | ∞        | -                      | WiFi lost                         | AUTO_RECONNECTING    |
| 1     | WIFI_AUTO_RECONNECTING     | 0-40s    | autoReconnectChecks    | Auto-reconnect OR 4 checks        | OK or MANUAL         |
| 2     | WIFI_MANUAL_RECONNECTING   | 40-90s   | -                      | Manual success OR timeout/failure | OK or FALLBACK/FAILED|
| 3     | WIFI_FALLBACK_CONNECTING   | ~8s      | fallbackAttempted      | Fallback success OR failure       | OK or FAILED         |
| 4     | WIFI_FAILED_REBOOTING      | 5s       | rebootCountdown        | Countdown reaches 0               | ESP.restart()        |

**Note:** Level 3 (WIFI_FALLBACK_CONNECTING) is optional and only executed when fallback credentials are configured.

---

## Constants Reference (config.h)

```cpp
// Initial connection
#define WIFI_CONNECTION_TIMEOUT     8000     // 8 seconds per attempt
#define WIFI_RETRY_DELAY            500      // 0.5 seconds between attempts

// Runtime recovery
#define WIFI_AUTO_RECONNECT_CHECKS  4        // 4 checks (40 seconds total)
#define WIFI_AUTO_RECONNECT_INTERVAL 10000   // 10 seconds per check
#define WIFI_MANUAL_RECONNECT_TIMEOUT 50000  // 50 seconds for manual attempts
#define WIFI_REBOOT_COUNTDOWN       5        // 5 seconds countdown before reboot
```

---

## End of Research Document

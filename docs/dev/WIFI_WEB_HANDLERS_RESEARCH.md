# WiFi Web Handlers Research

## Summary
This document details all web handlers and forms related to WiFi configuration in the ESP32 Radio project.

---

## Key Finding: NO WiFi Configuration in STA Mode

**Important:** The `index.html` (STA mode web interface) does **NOT** have any WiFi configuration UI.

- Users **CANNOT** change WiFi credentials from the normal web interface
- WiFi configuration is **ONLY** available in AP mode
- To change WiFi settings, users must:
  1. Factory reset the device, OR
  2. Manually edit `/wifi.json` in LittleFS

---

## AP Mode WiFi Handlers

### Location: `src/web_server_manager.cpp` (Lines 159-220)

All WiFi configuration handlers are in the `start_web_server_ap()` function.

---

## Handler 1: Root Page (/)

### Location: Lines 160-163

```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    web_loading_html.store(true, std::memory_order_relaxed);
    request->send(LittleFS, "/ap_mode.html", "text/html; charset=utf-8");
});
```

**Purpose:** Serves the WiFi setup page

**Method:** GET

**Response:** `data/ap_mode.html`

---

## Handler 2: Network Scan (/api/scan)

### Location: Lines 165-169

```cpp
server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    WiFi.scanNetworks(true);  // ← Async scan (true = async)
    request->send(200, "text/plain", "Scan Started");
});
```

**Purpose:** Initiates WiFi network scan

**Method:** GET

**Parameters:** None

**Process:**
1. Calls `WiFi.scanNetworks(true)` - starts async scan
2. Returns immediately with "Scan Started"
3. Client must poll `/api/scan-results` to get results

**Response:**
- Status: 200
- Body: "Scan Started"

---

## Handler 3: Scan Results (/api/scan-results)

### Location: Lines 171-188

```cpp
server.on("/api/scan-results", HTTP_GET, [](AsyncWebServerRequest *request){
    int n = WiFi.scanComplete();
    if (n == -1) {
        // Still scanning
        request->send(202, "text/plain", "Scanning...");
    } else {
        // Scan complete - build JSON response
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject net = array.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
        }
        WiFi.scanDelete();  // ← Clean up scan results
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json; charset=utf-8", response);
    }
});
```

**Purpose:** Returns WiFi scan results (polling endpoint)

**Method:** GET

**Parameters:** None

**Response States:**

### State 1: Scanning in Progress
- Status: 202 (Accepted)
- Body: "Scanning..."
- Client should poll again

### State 2: Scan Complete
- Status: 200
- Content-Type: application/json
- Body: JSON array of networks

**JSON Format:**
```json
[
  {
    "ssid": "NetworkName1",
    "rssi": -45
  },
  {
    "ssid": "NetworkName2",
    "rssi": -67
  }
]
```

**Process:**
1. Checks `WiFi.scanComplete()`:
   - Returns `-1` if still scanning
   - Returns count if complete
2. Builds JSON array with SSID and RSSI for each network
3. Calls `WiFi.scanDelete()` to free memory
4. Returns JSON response

---

## Handler 4: Save WiFi Config (/save)

### Location: Lines 189-220

```cpp
server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid;
    
    // Priority 1: Manual SSID entry
    if (request->hasParam("ssid_manual", true) && 
        !request->getParam("ssid_manual", true)->value().isEmpty()) {
        ssid = request->getParam("ssid_manual", true)->value();
    } 
    // Priority 2: Dropdown selection
    else if (request->hasParam("ssid_select", true)) {
        ssid = request->getParam("ssid_select", true)->value();
    }

    // Validate and save
    if (request->hasParam("password", true) && !ssid.isEmpty()) {
        String password = request->getParam("password", true)->value();
        show_message("Saving config...", ssid);
        
        // Update global config
        wifiConfig.ssid = ssid;
        wifiConfig.password = password;
        
        // Save to /wifi.json
        if (save_wifi_config()) {
            request->send(200, "text/plain", "WiFi settings saved. Rebooting...");
            delay(1000);
            ESP.restart();  // ← REBOOT AFTER SAVE
        } else {
            request->send(500, "text/plain", "Failed to save WiFi settings.");
        }
    } else {
        request->send(400, "text/plain", "Bad Request: SSID or password missing.");
    }
});
```

**Purpose:** Saves WiFi credentials and reboots

**Method:** POST

**Parameters (Form Data):**
- `ssid_manual` (optional) - Manually entered SSID
- `ssid_select` (optional) - Selected SSID from dropdown
- `password` (required) - WiFi password

**SSID Priority:**
1. Manual entry (`ssid_manual`) takes precedence
2. Dropdown selection (`ssid_select`) if manual is empty

**Process:**
1. Extract SSID (manual or dropdown)
2. Extract password
3. Validate both are present
4. Update global `wifiConfig` struct
5. Call `save_wifi_config()` to write `/wifi.json`
6. Send success response
7. Wait 1 second
8. **Reboot device** with `ESP.restart()`

**Response States:**

### Success:
- Status: 200
- Body: "WiFi settings saved. Rebooting..."
- Action: Device reboots after 1 second

### Save Failed:
- Status: 500
- Body: "Failed to save WiFi settings."

### Validation Failed:
- Status: 400
- Body: "Bad Request: SSID or password missing."

---

## AP Mode HTML Form

### File: `data/ap_mode.html`

### Form Structure (Lines 26-44)

```html
<form action="/save" method="post">
    <!-- Network Dropdown -->
    <label for="ssid-select">Choose a Network:</label>
    <select name="ssid_select" id="ssid-select"></select>
    
    <!-- Manual SSID Entry -->
    <label for="ssid-manual">Or Enter SSID Manually:</label>
    <input type="text" name="ssid_manual" id="ssid-manual" placeholder="Manual SSID">

    <!-- Password -->
    <label for="password">Password:</label>
    <div class="password-wrapper">
        <input type="password" name="password" id="password" placeholder="Password" required>
        <svg class="eye-icon" onclick="togglePassword('password')" ...>
            <!-- Eye icon for show/hide password -->
        </svg>
    </div>
    
    <input type="submit" value="Save & Reboot">
</form>
```

**Form Fields:**
1. `ssid_select` - Dropdown populated by scan results
2. `ssid_manual` - Text input for manual SSID entry
3. `password` - Password input (required)

**Features:**
- Password visibility toggle (eye icon)
- Either dropdown OR manual SSID (manual takes priority)
- Submit triggers `/save` POST handler

---

## JavaScript Scan Flow

### Location: `data/ap_mode.html` (Lines 47-100)

### Scan Button Click (Lines 79-91)

```javascript
scanBtn.addEventListener('click', async () => {
    statusEl.textContent = 'Starting scan...';
    scanBtn.disabled = true;
    
    try {
        const response = await fetch('/api/scan');
        if (response.ok) {
            statusEl.textContent = 'Scanning in progress...';
            setTimeout(pollScanResults, 1500); // Start polling after 1.5s
        } else {
            throw new Error('Failed to start scan');
        }
    } catch (e) {
        statusEl.textContent = 'Could not start scan.';
        scanBtn.disabled = false;
    }
});
```

**Process:**
1. Disable scan button
2. Call `/api/scan` to start scan
3. Wait 1.5 seconds
4. Start polling for results

### Poll Scan Results (Lines 52-78)

```javascript
async function pollScanResults() {
    try {
        const response = await fetch('/api/scan-results');
        
        if (response.status === 202) {
            // Still scanning - poll again
            setTimeout(pollScanResults, 1500);
        } 
        else if (response.ok) {
            // Scan complete
            const networks = await response.json();
            statusEl.textContent = `Scan complete. Found ${networks.length} networks.`;
            
            // Populate dropdown
            ssidSelect.innerHTML = '<option value="">-- Select from list --</option>';
            if (networks.length > 0) {
                networks.forEach(net => {
                    const option = document.createElement('option');
                    option.value = net.ssid;
                    option.textContent = `${net.ssid} (${net.rssi} dBm)`;
                    ssidSelect.appendChild(option);
                });
            }
            scanBtn.disabled = false;
        } 
        else {
            throw new Error('Scan result error');
        }
    } catch (e) {
        statusEl.textContent = 'Error getting scan results.';
        scanBtn.disabled = false;
    }
}
```

**Polling Logic:**
1. Call `/api/scan-results`
2. If status 202 → Still scanning, poll again in 1.5s
3. If status 200 → Parse JSON and populate dropdown
4. Display network count and RSSI values
5. Re-enable scan button

**Dropdown Format:**
```
-- Select from list --
NetworkName1 (-45 dBm)
NetworkName2 (-67 dBm)
NetworkName3 (-80 dBm)
```

---

## SSID Input Priority Logic

### Location: `data/ap_mode.html` (Lines 93-100)

```javascript
// Clear manual input when dropdown selected
ssidSelect.addEventListener('change', () => {
    if (ssidSelect.value) manualSsidInput.value = '';
});

// Clear dropdown when manual input used
manualSsidInput.addEventListener('input', () => {
    if (manualSsidInput.value) ssidSelect.value = '';
});
```

**Behavior:**
- Selecting from dropdown → Clears manual input
- Typing in manual input → Clears dropdown selection
- Server-side: Manual input takes priority if both present

---

## Password Visibility Toggle

### Location: `data/ap_mode.html` (Lines 102-112)

```javascript
function togglePassword(inputId) {
    const input = document.getElementById(inputId);
    const icon = event.currentTarget;
    
    if (input.type === 'password') {
        input.type = 'text';
        icon.innerHTML = '<path d="...crossed eye icon..."></path>';
    } else {
        input.type = 'password';
        icon.innerHTML = '<path d="...open eye icon..."></path>';
    }
}
```

**Feature:** Click eye icon to show/hide password

---

## STA Mode (Normal Operation)

### File: `data/index.html`

**WiFi Configuration:** ❌ NOT AVAILABLE

**Findings:**
- No WiFi settings section
- No SSID/password inputs
- No network scan functionality
- Only mentions WiFi in Factory Reset warning:
  - Line 186: "WiFi настройки" (will be deleted)

**Available Settings in index.html:**
- Radio station management
- Volume control
- Visualizer style
- Display rotation
- Language selection
- Factory reset

---

## How to Change WiFi in STA Mode

Since there's no WiFi configuration UI in STA mode, users have 3 options:

### Option 1: Factory Reset (Recommended)
1. Access web interface
2. Go to Settings → Danger Zone
3. Click "Factory Reset"
4. Enter password
5. Device reboots into AP mode
6. Configure new WiFi credentials

### Option 2: Forgot Password Reset
1. Access `/forgot-password` page
2. Confirm factory reset (no password needed)
3. Device reboots into AP mode
4. Configure new WiFi credentials

### Option 3: Manual File Edit (Advanced)
1. Connect to device via serial/USB
2. Access LittleFS filesystem
3. Edit `/wifi.json` directly
4. Reboot device

---

## Complete WiFi Configuration Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    DEVICE BOOT                              │
└─────────────────────────────────────────────────────────────┘
                            ↓
                    load_wifi_config()
                            ↓
                ┌───────────┴───────────┐
                │                       │
         /wifi.json exists       /wifi.json missing
                │                       │
                ↓                       ↓
        connect_to_wifi()          AP MODE
                │                       │
        ┌───────┴───────┐              ↓
        │               │         192.168.4.1
    SUCCESS         FAILURE          ↓
        │               │         ap_mode.html
        ↓               ↓              ↓
    STA MODE        AP MODE      ┌────────────┐
        │               │         │ User Actions│
        ↓               ↓         └────────────┘
   index.html      ap_mode.html        ↓
        │               │         1. Click "Scan"
        │               │              ↓
        │               │         GET /api/scan
        │               │              ↓
        │               │         WiFi.scanNetworks(true)
        │               │              ↓
        │               │         Poll /api/scan-results
        │               │              ↓
        │               │         Populate dropdown
        │               │              ↓
        │               │         2. Select network OR
        │               │            Enter manual SSID
        │               │              ↓
        │               │         3. Enter password
        │               │              ↓
        │               │         4. Click "Save & Reboot"
        │               │              ↓
        │               │         POST /save
        │               │              ↓
        │               │         save_wifi_config()
        │               │              ↓
        │               │         Write /wifi.json
        │               │              ↓
        │               │         ESP.restart()
        │               │              ↓
        │               └──────────────┘
        │                              │
        └──────────────────────────────┘
                       ↓
              Device reboots with
              new WiFi credentials
```

---

## Security Considerations

### AP Mode Security:
- ❌ No password on AP ("ESP32-Radio-Setup")
- ❌ Anyone can connect and configure WiFi
- ⚠️ WiFi password transmitted in plain text (HTTP POST)
- ⚠️ No HTTPS in AP mode

### Recommendations:
1. Add AP password protection
2. Implement HTTPS (difficult on ESP32)
3. Add WPS support as alternative
4. Timeout AP mode after X minutes

---

## Related Files

### Backend:
- `src/web_server_manager.cpp` - All WiFi handlers (lines 159-220)
- `src/config.cpp` - `save_wifi_config()` function
- `src/wifi_manager.cpp` - `connect_to_wifi()` function

### Frontend:
- `data/ap_mode.html` - WiFi setup page (AP mode)
- `data/index.html` - Main interface (NO WiFi config)

### Storage:
- `/wifi.json` - WiFi credentials storage (LittleFS)

---

## API Endpoints Summary

| Endpoint           | Method | Mode | Purpose                    | Response                  |
|--------------------|--------|------|----------------------------|---------------------------|
| /                  | GET    | AP   | Serve ap_mode.html         | HTML page                 |
| /api/scan          | GET    | AP   | Start network scan         | "Scan Started"            |
| /api/scan-results  | GET    | AP   | Get scan results (polling) | JSON array or 202 status  |
| /save              | POST   | AP   | Save WiFi & reboot         | Success/error message     |

**Note:** All endpoints are ONLY available in AP mode. STA mode has NO WiFi configuration endpoints.

---

## End of Research Document

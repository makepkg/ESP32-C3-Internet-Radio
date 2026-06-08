# API Endpoints Reference

Complete API documentation for ESP32-C3 Internet Radio web server.

**Base URL:** `http://[device-ip]/`  
**Port:** 80 (HTTP)  
**Server:** AsyncWebServer (ESPAsyncWebServer)

---

## Authentication

### Session-Based Auth

Two methods:
1. **Cookie:** `session=[token]` (set after login)
2. **Header:** `X-Session-Token: [token]`

**Token lifetime:** Until logout or 25 reboots (auto-clear)

**401 Unauthorized:** All protected endpoints return 401 if not authenticated

---

## AP Mode Endpoints

Active when device creates WiFi network `ESP32-Radio-Setup` (IP: 192.168.4.1)

### GET /

Serve WiFi setup page

**Response:** HTML page (`ap_mode.html`)

---

### GET /api/scan

Start WiFi network scan

**Auth:** None  
**Response:** 
```
200 OK
"Scan Started"
```

**Note:** Async operation, use `/api/scan-results` to get results

---

### GET /api/scan-results

Get WiFi scan results (polling endpoint)

**Auth:** None  
**Response:**

**202 Accepted** (still scanning):
```
"Scanning..."
```

**200 OK** (scan complete):
```json
[
  {
    "ssid": "NetworkName",
    "rssi": -45
  }
]
```

---

### POST /save

Save WiFi credentials and reboot

**Auth:** None  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `ssid_manual` (optional) — Manually entered SSID
- `ssid_select` (optional) — Selected SSID from scan
- `password` (required) — WiFi password

**Priority:** `ssid_manual` > `ssid_select`

**Response:**
- `200 OK` — WiFi settings saved, device reboots
- `400 Bad Request` — Missing parameters
- `500 Internal Server Error` — Save failed

---

## STA Mode Endpoints

Active when device connected to WiFi network

---

## Web Pages

### GET /

Main application page

**Auth:** Session token (auto-login) or redirect to login  
**Response:** 
- `index.html` if authenticated
- Redirect to `/login` if not authenticated
- Redirect to `/register` if not registered

**Headers:**
- `Cache-Control: no-cache`
- `Content-Type: text/html; charset=utf-8`

---

### GET /register

User registration page (first launch only)

**Auth:** None  
**Response:**
- `register.html` if not registered
- Redirect to `/login` if already registered

---

### GET /login

Login page

**Auth:** None  
**Response:**
- HTML login form if not authenticated
- Redirect to `/` if already authenticated
- Redirect to `/register` if not registered

---

### GET /forgot-password

Factory reset warning page

**Auth:** None  
**Response:** HTML page with factory reset button

---

## Authentication API

### POST /register

Register new user (first launch only)

**Auth:** None  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `username` (required, min 5 chars)
- `password` (required, min 5 chars)

**Response:**
- `200 OK` — Registration successful
- `400 Bad Request` — Already registered or validation failed
- `500 Internal Server Error` — Save failed

---

### POST /login

Authenticate user

**Auth:** None  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `username` (required)
- `password` (required)

**Response:**
- `302 Redirect` to `/` + `Set-Cookie: session=[token]`
- `401 Unauthorized` — Invalid credentials
- `400 Bad Request` — Missing parameters

**Cookie:** `session=[token]; Path=/; Max-Age=31536000` (1 year)

---

### GET /logout

Logout current user

**Auth:** Required  
**Response:** `302 Redirect` to `/login` + clear session cookie

**Side effects:**
- `sessionToken` cleared
- State saved

---

### POST /api/forgot-password-reset

Factory reset without password (for forgotten password)

**Auth:** None (by design)  
**Response:** `200 OK` + device reboots

**⚠️ Warning:** Deletes ALL data:
- Credentials
- WiFi config
- Stations
- State
- Logs

---

## Station Management API

### GET /api/stations/info

Get station count and limit

**Auth:** Required  
**Response:**
```json
{
  "current": 5,
  "max": 25,
  "available": 20
}
```

---

### GET /api/stations

Get all radio stations

**Auth:** Required  
**Response:**
```json
[
  {
    "name": "Radio Paradise",
    "url": "http://stream.radioparadise.com/mp3-320"
  }
]
```

**Order:** As saved (user-defined order)

---

### POST /api/add

Add new radio station

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `name` (required) — Station display name
- `url` (required) — MP3 stream URL (HTTP/HTTPS)

**URL Validation:**
- Must start with `http://` or `https://`
- Max length: 512 chars
- Domain must be valid
- Port allowed

**Response:**
- `200 OK` — Station added
- `400 Bad Request` — Limit reached (25) or invalid URL
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves `stations.json` via command queue

---

### POST /api/delete

Delete radio station

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `name` (required) — Station name to delete

**Response:**
- `200 OK` — Station deleted
- `400 Bad Request` — Missing parameter
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves `stations.json` via command queue

---

### POST /api/update

Update radio station

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `originalName` (required) — Current station name
- `name` (required) — New station name
- `url` (required) — New URL

**URL Validation:** Same as `/api/add`

**Response:**
- `200 OK` — Station updated
- `400 Bad Request` — Invalid URL or missing parameters
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves `stations.json` via command queue

---

### POST /api/stations/order

Reorder stations (drag-and-drop)

**Auth:** Required  
**Content-Type:** `application/json`

**Body:** Array of station names in new order
```json
[
  "Station 1",
  "Station 2",
  "Station 3"
]
```

**Response:**
- `200 OK` — Order saved
- `400 Bad Request` — Invalid data
- `401 Unauthorized` — Not authenticated

**Side effects:**
- Saves `stations.json` via command queue
- Resets audio to station #0
- Prints new order to Serial Monitor

---

### GET /api/stations/export

Export stations as JSON file

**Auth:** Required  
**Response:** File download (`stations.json`)

**Content-Type:** `application/json; charset=utf-8`  
**Content-Disposition:** `attachment`

---

### POST /api/stations/import

Import stations from JSON file

**Auth:** Required  
**Content-Type:** `multipart/form-data`

**Body:** File upload (field: `file`)

**File format:** Same as `stations.json`
```json
[
  {
    "name": "Station Name",
    "url": "http://example.com/stream.mp3"
  }
]
```

**Response:**
- `200 OK` — Import successful
- `401 Unauthorized` — Not authenticated

**Side effects:**
- Overwrites `/stations.json`
- Reloads station config

---

## Player Control API

### POST /api/player/next

Switch to next station

**Auth:** Required  
**Response:**
- `200 OK` — Command sent
- `503 Service Unavailable` — Command queue full
- `401 Unauthorized` — Not authenticated

**Async:** Command sent via FreeRTOS queue

---

### POST /api/player/previous

Switch to previous station

**Auth:** Required  
**Response:**
- `200 OK` — Command sent
- `503 Service Unavailable` — Command queue full
- `401 Unauthorized` — Not authenticated

**Async:** Command sent via FreeRTOS queue

---

### POST /api/player/volume

Set volume level

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `volume` (required, float 0.0-1.0)

**Response:**
- `200 OK` — Volume set
- `400 Bad Request` — Missing parameter
- `503 Service Unavailable` — Command queue full
- `401 Unauthorized` — Not authenticated

**Async:** Command sent via FreeRTOS queue

---

## Display Control API

### GET /api/display/rotation

Get current display rotation

**Auth:** Required  
**Response:**
```json
{
  "rotation": 0
}
```

**Values:** `0` (normal) or `2` (180° flipped)

---

### POST /api/display/rotation

Set display rotation

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `rotation` (required, int: 0 or 2)

**Response:**
- `200 OK` — Rotation set
- `400 Bad Request` — Invalid value (not 0 or 2)
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves state

---

## Visualizer API

### GET /api/visualizer/style

Get current visualizer style

**Auth:** Required  
**Response:**
```json
{
  "style": 3,
  "name": "Hexagon"
}
```

**Style IDs:** 0-11 (see `/api/visualizer/styles`)

---

### POST /api/visualizer/style

Set visualizer style

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `style` (required, int 0-11)

**Response:**
- `200 OK` — Style set
- `400 Bad Request` — Invalid style ID
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves state

---

### GET /api/visualizer/styles

Get all available visualizer styles

**Auth:** Required  
**Response:**
```json
[
  {"id": 0, "name": "Bars"},
  {"id": 1, "name": "Wave"},
  {"id": 2, "name": "Circle"},
  {"id": 3, "name": "Hexagon"},
  {"id": 4, "name": "Stars"},
  {"id": 5, "name": "Mirror"},
  {"id": 6, "name": "Lightning"},
  {"id": 7, "name": "Tesseract"},
  {"id": 8, "name": "Plasma"},
  {"id": 9, "name": "Holo Grid"},
  {"id": 10, "name": "Omni Scan"},
  {"id": 11, "name": "Goo"}
]
```

---

## WiFi Management API

### GET /api/wifi/fallback

Get fallback WiFi configuration

**Auth:** Required  
**Response:**
```json
{
  "fallback_enabled": true,
  "fallback_ssid": "Backup_Network",
  "fallback_has_password": true
}
```

**Note:** Password not returned (security)

---

### POST /api/wifi/fallback

Update fallback WiFi configuration

**Auth:** Required  
**Content-Type:** `application/json`

**Body:**
```json
{
  "fallback_enabled": true,
  "fallback_ssid": "Backup_Network",
  "fallback_password": "backup_password"
}
```

**All fields optional** (only updates provided fields)

**Response:**
```json
{"status": "ok"}
```

**Errors:**
- `400 Bad Request` — Invalid JSON
- `500 Internal Server Error` — Save failed
- `401 Unauthorized` — Not authenticated

**Side effects:** Saves `/wifi.json`

---

## System API

### GET /api/logs

Get system logs

**Auth:** Required  
**Response:** Plain text log (last 20KB)

**Content-Type:** `text/plain`

**Format:**
```
[2025-01-08 12:34:56] Log message 1
[2025-01-08 12:35:01] Log message 2
```

---

### POST /api/system/reboot

Reboot device

**Auth:** Required  
**Response:**
- `200 OK` — "Rebooting..."
- `503 Service Unavailable` — Command queue full
- `401 Unauthorized` — Not authenticated

**Async:** Command sent via FreeRTOS queue

---

### POST /api/factory-reset

Factory reset (requires password)

**Auth:** Required  
**Content-Type:** `application/x-www-form-urlencoded`

**Parameters:**
- `password` (required) — User password for verification

**Response:**
- `200 OK` — Factory reset complete, device reboots
- `401 Unauthorized` — Wrong password or not authenticated
- `400 Bad Request` — Missing parameter

**⚠️ Warning:** Deletes ALL data (see `/api/forgot-password-reset`)

---

## Static Files

### GET /[path]

Serve static files from LittleFS

**Auth:** None (files)  
**Caching:** `max-age=3600` (1 hour)

**Files:**
- `/index.html` — Main app
- `/register.html` — Registration page
- `/ap_mode.html` — WiFi setup page (AP mode)
- `/styles.css` — Stylesheets
- `/script.js` — JavaScript
- Other assets

---

## Error Handling

### HTTP Status Codes

| Code | Meaning |
|------|---------|
| 200 | OK — Request successful |
| 302 | Redirect — See Location header |
| 400 | Bad Request — Invalid parameters or data |
| 401 | Unauthorized — Not authenticated |
| 404 | Not Found — Endpoint does not exist |
| 500 | Internal Server Error — Server-side error |
| 503 | Service Unavailable — Command queue full |

### 404 Not Found Handler

**Behavior:**
- Not registered → Redirect to `/register`
- Not authenticated → Redirect to `/login`
- Authenticated → `404 Not found`

---

## Security Features

### Session Token

**Generation:** 32-character random string (alphanumeric)  
**Storage:** Saved in `/state.json`  
**Lifetime:** Until logout or 25 reboots  
**Transport:** HTTP cookie or header

### Password Hashing

**Algorithm:** SHA-256  
**Storage:** `/credentials.json` (hash only, not plaintext)

### CORS

**Not implemented** (same-origin only)

### Rate Limiting

**Not implemented** (no brute-force protection)

### HTTPS

**Not available** (HTTP only)

---

## Performance Notes

### Async Operations

Commands sent to main loop via FreeRTOS queues:
- Volume change
- Station switch
- Reboot
- Save stations

**Queue size:** 10 commands  
**503 Error:** Queue full (wait and retry)

### File Operations

Large file serving (HTML pages) sets `web_loading_html` flag:
- Pauses audio processing
- Cleared after 100ms
- Ensures fast page load

### Race Conditions

Station list protected by mutex (`stationsMutex`):
- All read/write operations locked
- Safe for async web server + main loop access

---

## Complete Endpoint List

### AP Mode (3 endpoints)

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | WiFi setup page |
| GET | `/api/scan` | Start WiFi scan |
| GET | `/api/scan-results` | Get scan results |
| POST | `/save` | Save WiFi credentials |

### STA Mode (28 endpoints)

#### Pages (4)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Main app page |
| GET | `/register` | Registration page |
| GET | `/login` | Login page |
| GET | `/forgot-password` | Factory reset page |

#### Auth (4)
| Method | Path | Description |
|--------|------|-------------|
| POST | `/register` | Register user |
| POST | `/login` | Login |
| GET | `/logout` | Logout |
| POST | `/api/forgot-password-reset` | Factory reset (no auth) |

#### Stations (8)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/stations/info` | Station count/limit |
| GET | `/api/stations` | List all stations |
| POST | `/api/add` | Add station |
| POST | `/api/delete` | Delete station |
| POST | `/api/update` | Update station |
| POST | `/api/stations/order` | Reorder stations |
| GET | `/api/stations/export` | Export JSON |
| POST | `/api/stations/import` | Import JSON |

#### Player (3)
| Method | Path | Description |
|--------|------|-------------|
| POST | `/api/player/next` | Next station |
| POST | `/api/player/previous` | Previous station |
| POST | `/api/player/volume` | Set volume |

#### Display (2)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/display/rotation` | Get rotation |
| POST | `/api/display/rotation` | Set rotation |

#### Visualizer (3)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/visualizer/style` | Get current style |
| POST | `/api/visualizer/style` | Set style |
| GET | `/api/visualizer/styles` | List all styles |

#### WiFi (2)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/wifi/fallback` | Get fallback config |
| POST | `/api/wifi/fallback` | Set fallback config |

#### System (3)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/logs` | Get system logs |
| POST | `/api/system/reboot` | Reboot device |
| POST | `/api/factory-reset` | Factory reset (with password) |

#### Static Files (1)
| Method | Path | Description |
|--------|------|-------------|
| GET | `/[path]` | Serve files from LittleFS |

**Total Endpoints:** 31 (3 AP mode + 28 STA mode)

---

## Testing Examples

### cURL

**Login:**
```bash
curl -X POST http://192.168.1.100/login \
  -d "username=admin&password=secret" \
  -c cookies.txt
```

**Add Station:**
```bash
curl -X POST http://192.168.1.100/api/add \
  -b cookies.txt \
  -d "name=Jazz FM&url=http://example.com/stream.mp3"
```

**Set Volume:**
```bash
curl -X POST http://192.168.1.100/api/player/volume \
  -b cookies.txt \
  -d "volume=0.75"
```

**Get Logs:**
```bash
curl http://192.168.1.100/api/logs -b cookies.txt
```

---

**Version:** v4.5  
**Last Updated:** 2025-01-08  
**Protocol:** HTTP/1.1  
**Server:** ESPAsyncWebServer 3.2.2

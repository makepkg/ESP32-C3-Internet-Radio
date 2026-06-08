# Changelog

All notable changes to this project are documented here.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [4.6.0] — 2026-06-08

### Added
- **GitHub Actions CI/CD**: Automatic build and code formatting checks.
- **Project Structure**: Organized `.github` directory with templates and Dependabot.

### Fixed
- **Build System**: Fixed compilation errors related to I2S and MIDI (downgraded to stable `espressif32@6.10.0` and `ESP8266Audio@1.9.7`).
- **CI Compatibility**: Resolved Node.js 20 warnings in GitHub Actions.
- **Code Style**: Standardized code formatting using `clang-format`.

---

## [4.5] — 2025-10-15

### Added
- Interactive IP display with pause/resume control on OLED
- 9th visualizer mode: **Plasma** (liquid plasma animation)

### Changed
- Default language switched to English
- Settings menu reordered for better usability

### Fixed
- Encoder jitter on fast rotation causing double-click misdetection
- Station list not saving after reorder in some edge cases

---

## [4.4] — 2025-09-xx

### Added
- **Tesseract** visualizer (4D hypercube animation)
- **Lightning** visualizer (lightning between spectrum peaks)
- Web API: `GET /api/visualizer/styles` — list all available styles
- Web API: `POST /api/stations/order` — drag-and-drop reorder endpoint

### Changed
- OLED frame skip logic improved — audio gets priority during MP3 decode
- Session tokens now auto-clear every 25 reboots

---

## [4.3] — 2025-08-xx

### Added
- **Stars** visualizer (audio-reactive starfield)
- **Mirror** visualizer (mirrored vertical bars)
- Station import/export as JSON (`/api/stations/import`, `/api/stations/export`)
- SHA-256 password hashing for web credentials

### Changed
- Buffer increased from 64 KB to 128 KB
- WiFi recovery upgraded to 3-level system (auto → manual → reboot)

---

## [4.0] — 2025-07-xx

### Added
- **Hexagon** visualizer (cyberpunk hexagons)
- **Circle** visualizer (circular equalizer)
- Deep sleep mode (long hold 5 s) — ~10 µA sleep current
- Brownout detection with automatic reboot and logging
- Web interface: real-time system monitoring (RAM, RSSI, uptime)
- Session-based authentication

### Changed
- I2C speed increased to 400 kHz (Fast Mode)
- FreeRTOS task priorities: Audio > Input > Display > WiFi > System

---

## [3.x] — Earlier

- Initial public release with **Bars** and **Wave** visualizers
- Basic web interface (add/delete stations, volume, rotation)
- LittleFS filesystem for configuration persistence
- KY-040 encoder: click / double-click / long-press

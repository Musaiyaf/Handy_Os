my discord 
https://discord.gg/X226EBqcc



<blockquote>
  <h3>⚠️ Distribution Warning</h3>
  <p>
    <strong>This project is NOT open source yet.</strong><br>
    The code is currently public for <em>viewing and auditing purposes only</em>. 
    All rights are reserved by the author. No license is granted for modification or redistribution at this time.
  </p>
</blockquote>



# 📟 HandyOS v11.2.0 — *The Refined Edition*

> A fully-featured handheld operating system for the ESP32, running on a 1.44" ST7735 128×128 TFT display with a 5-way joystick. Built from scratch by **musaiyaf**.

---

## 🌟 What is HandyOS?

HandyOS is a custom bare-metal firmware that turns a bare ESP32 + tiny TFT display into a capable pocket computer. It packs **9 games**, **19 tools**, **8 Islamic/MuslimOS features**, a full **WiFi suite**, **OTA updates**, **NVS-persistent storage**, and a polished Marauder-inspired UI — all controlled with a 7-button joystick and no touchscreen required.

---

## 🖥️ Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 (any 38-pin board) |
| Display | 1.44" ST7735 TFT — 128×128 pixels |
| Input | 5-Way Joystick (UP / DOWN / LEFT / RIGHT / MID) + SET + RST |
| Audio | Optional passive buzzer on GPIO 26 |

### Pin Map

```
TFT_CS=5   TFT_RST=4   TFT_DC=2
TFT_MOSI=23  TFT_CLK=18  TFT_BL=15

BTN_UP=13   BTN_DWN=12  BTN_LFT=14
BTN_RHT=27  BTN_MID=33  BTN_SET=32
BTN_RST=25  BUZZER=26
```

---

## 📦 Dependencies

Install these via the Arduino Library Manager before flashing:

- `Adafruit ST7735 and ST7789 Library`
- `Adafruit GFX Library`
- `ESP32 Arduino Core` (includes WiFi, Preferences, ESPmDNS, Update, WebServer)

---

## ⚡ Quick Flash

1. Open `HandyOS_v11_2_0.ino` in Arduino IDE (or ArduinoDroid).
2. Select board: **ESP32 Dev Module**.
3. Set Partition Scheme to **Minimal SPIFFS** (for OTA headroom).
4. Upload. On first boot the logo splash appears for ~1.8 s, then NVS settings are loaded and saved WiFi is auto-reconnected.

---

## 🗂️ Main Menu

Six top-level sections, navigated with **UP / DOWN**, launched with **MID**:

| # | Section | Description |
|---|---------|-------------|
| 1 | 🎮 Games | 9 arcade & puzzle games |
| 2 | 🔧 Tools | 19 productivity & utility tools |
| 3 | 📶 WiFi | Scan, connect, web apps & network tools |
| 4 | ☪️ Islamic | Full MuslimOS prayer & Islamic suite |
| 5 | ⚙️ Settings | 10-row scrollable settings panel |
| 6 | ℹ️ About | Firmware info & credits |

> **App Shortcut:** Hold **SET** on any menu row for ~800 ms to pin it. On the next boot, hold **SET** while powering on to jump directly to that app.

---

## 🎮 Games (9 total)

### 1. 🐍 Snake
Classic snake — eat food, grow longer, don't hit walls or yourself.  
High score is **NVS-persistent** (survives reboot).  
Controls: joystick direction to steer · RST = exit.

### 2. 🏓 Pong
Single-player Pong against a CPU paddle.  
Controls: UP/DOWN to move · RST = exit.

### 3. 🧱 Brick Break
Breakout-style brick demolition with a bouncing ball.  
Controls: LEFT/RIGHT to move paddle · RST = exit.

### 4. ⚡ Reaction Test
A randomly timed "flash" appears — tap MID as fast as possible. Your reaction time is displayed in milliseconds.  
Controls: MID = react · RST = exit.

### 5. 🧠 Memory Match
Card-flip memory game. Find matching pairs on a hidden grid.  
Controls: joystick to move cursor · MID = flip · RST = exit.

### 6. 🟦 Tetris
Full classic Tetris with falling tetrominoes, line clears, and level speed scaling.  
Controls: LEFT/RIGHT to move · UP to rotate · DOWN to soft-drop · RST = exit.

### 7. 🔢 2048
The 4×4 sliding tile puzzle. Merge tiles to reach 2048.  
Controls: joystick direction to slide · RST = exit.

### 8. 🐦 Flappy Bird
Side-scrolling pipe-dodge. Tap to flap, survive as long as possible.  
Controls: MID = flap · RST = exit.

### 9. 🎵 Simon Says
Colour-and-sound pattern memory game. Watch the 4-direction sequence grow, then repeat it exactly. Uses the buzzer for distinct tones per direction.  
Controls: joystick directions to repeat pattern · RST = exit.

---

## 🔧 Tools (19 total)

### ⏱️ Timer
Countdown timer. Set hours / minutes / seconds, start/pause/reset, with buzzer alert on completion.

### ⏱️ Stopwatch
Lap-capable stopwatch with centisecond precision. MID = start/stop · SET = lap · RST = exit.

### 🧮 Calculator
Full 4-function calculator (+, −, ×, ÷) with decimal support. State is **NVS-persistent** (remembers last result after reboot).

### 🎲 Dice Roller
Roll any standard die (D4, D6, D8, D10, D12, D20). Animated roll, result displayed large.

### 📐 Unit Converter
Multi-category converter: **Length** (m/km/mi/ft/in), **Weight** (kg/lb/g/oz), **Temperature** (°C/°F/K), and more.

### 📡 Morse Code Encoder
Type a message with the on-screen keyboard and watch it rendered as dots and dashes in real time, with optional buzzer output.

### 🧭 Digital Compass
Bearing in degrees (0–360°) with an 8-point compass rose (N, NE, E, SE, S, SW, W, NW). Uses ESP32 internal magnetometer or manual heading input.

### 📝 Notes
NVS-persistent notepad. Stores up to **4 notes** of 40 characters each — survives reboot and deep sleep.

### 🔢 Tally Counter
Simple increment/decrement counter. NVS-persistent daily tally — count survives reboot.

### 🔑 Password Generator
Generate random passwords with customisable character sets (uppercase, lowercase, digits, symbols) and length.

### 🍅 Pomodoro Timer
25-minute focus + 5-minute break cycles. Buzzer sounds at each transition. Visual countdown with cycle count.

### 🔀 Base Converter
Live conversion between **Decimal**, **Hexadecimal**, **Binary**, and **Octal**. Enter a value in any base and all others update instantly.

### 🔊 Tone Generator
Play specific frequencies through the buzzer. Useful for tuning, testing, or fun. Joystick adjusts frequency.

### ⬛ QR Code Generator
Type any text (URL, message, Wi-Fi credentials) and a QR code is rendered on-screen using a full Reed-Solomon encoder. Scannable with most phone cameras.

### 🔍 Port Scanner
Enter a target IP address and scan a range of TCP ports. Displays open ports live as scanning proceeds.

### 🆔 MAC Info
Displays the ESP32's current MAC address and lets you generate and apply a **random MAC** for privacy.

### ✅ Habit Tracker
Track **7 daily habits** (Prayer, Study, Exercise, Read, Journal, Hydrate, Sleep). Each habit is a checkbox — completions are stored as a bitmask in NVS, persistent per day.

### 🎨 RGB Color Mixer
Live 58×60 pixel colour preview on the left, R/G/B sliders on the right. Displays both the `#RRGGBB` hex code and the 16-bit 565 value. MID = fullscreen colour flash with contrasting hex text overlay.  
Controls: LEFT/RIGHT = select channel · UP/DOWN = adjust value (step 8) · RST = exit.

### 📡 OTA Update
Hosts a web server at `http://handyos.local` (via mDNS). Navigate to it from any browser on the same network and upload a new `.bin` firmware file — no USB cable needed.

---

## 📶 WiFi Suite

### Scan & Connect
Scan for nearby access points, select one, enter the password via on-screen keyboard. Credentials are **saved to NVS** and auto-reconnected on every boot.

### 🌤️ Weather
Fetches current weather for any city from `wttr.in` (no API key required). Displays condition, temperature, humidity, and wind. Weather summary also appears in the status bar.

### 🌍 IP Info
Fetches your public IP, city, country, and ISP from `ip-api.com` (no API key required).

### 📰 News Ticker
Scrolling news headlines fetched live from a public RSS/API endpoint.

### 📈 Markets
Live price ticker for configurable assets (crypto or stocks) via public APIs.

### 🔎 AP Scanner
Scans all nearby Wi-Fi access points and lists them with SSID, RSSI, channel, and encryption type.

### 📡 Ping Tester
Ping a target IP or hostname and display round-trip time and packet loss.

### 🌡️ WiFi Signal Monitor *(v11 new)*
Scrolling RSSI history graph with a 100-point ring buffer. Live dBm reading, quality label (Excellent / Good / Fair / Weak), and a percentage bar. Reference lines at −55 dBm and −70 dBm. Auto-updates every 1.5 seconds with flicker-free partial redraw.

### 🕵️ Deauth Detector
Passive 802.11 deauth frame scanner — monitors the air for deauthentication packets without connecting to any network.

---

## ☪️ Islamic / MuslimOS Suite (8 features)

> Configurable for any city worldwide. Supports 5 calculation methods: **MWL, ISNA, Egypt, Makkah, Karachi**.

### 🕌 Prayer Times
Calculates Fajr, Sunrise, Dhuhr, Asr, Maghrib, and Isha for your configured coordinates and GMT offset. Times are computed locally — no internet required after NTP sync.

### 🧭 Qibla Direction
Calculates the great-circle bearing to the Kaaba in Mecca from your set latitude/longitude, displayed as degrees and compass direction.

### 📖 Quran
Scrollable Quran verses display (selected Surahs and Ayahs stored in flash).

### 📿 Dhikr Counter
Finger-tasbih counter. MID = increment. Resets and vibrates (buzzer) at milestones.

### 🔢 Tasbeeh Counter
Dedicated dhikr counter — a cleaner single-purpose version for SubhanAllah / Alhamdulillah / Allahu Akbar cycling.

### 📅 Hijri Calendar
Displays today's Hijri (Islamic lunar) date alongside the Gregorian date. Includes **Lunar Phase** indicator (new moon, crescent, half, full, etc.).

### 🔔 Adhan Alarm
Automatic adhan alert — fires the buzzer at each calculated prayer time. Enable/disable per prayer. Fires silently in the background regardless of which app is active.

### 🤲 Duas & 99 Names
Scrollable collection of common Duas (supplications) and the 99 Names of Allah (Asma ul-Husna) with meanings.

---

## ⚙️ Settings (10 rows, scrollable)

| Setting | Options |
|---------|---------|
| Speed | 1–10 (UI animation speed) |
| Brightness | 0–10 (PWM backlight control) |
| Rotation | 0° / 90° / 180° / 270° |
| Deep Sleep | Off / 1–30 min timeout |
| Screensaver | Off / 1–10 min timeout |
| **Accent Theme** *(v11 new)* | Orange / Cyan / Green / Pink / Yellow / Red / White |
| **Buzzer Volume** *(v11 new)* | 0 (Mute) – 5, with live preview tap |
| **Weather City** *(v11 new)* | Change wttr.in city via on-screen keyboard |
| **Lock Screen** *(v11 new)* | Enable / change 4-digit PIN |
| WiFi Credentials | Saved SSID (view / clear) |

All settings are **persisted to NVS** (ESP32 flash) and restored on every boot.

---

## 🔒 Security Features

### Lock Screen
Optional 4-digit PIN lock. If enabled, the PIN entry screen appears on every boot (and after deep-sleep wake). Configure from Settings.

### Screensaver
After a configurable idle timeout, a clock/animation screensaver activates. Any button press wakes it. Timeout configurable in Settings.

---

## 💾 System Features

### Deep Sleep with RTC Resume
Press **SET + RST** to enter deep sleep. The last active app state is stored in RTC memory. On wake, HandyOS resumes directly into the app you were using (Calculator, Timer, Notes, or Tally).

### NVS Persistence
The following data survives reboots and deep sleep via the ESP32 Non-Volatile Storage (NVS / Preferences library):
- All Settings (brightness, speed, rotation, accent, volume, etc.)
- WiFi credentials (auto-reconnect on boot)
- Snake high score
- Tally counter value
- Up to 4 notepad entries
- Habit tracker daily state
- Islamic configuration (city, coordinates, GMT offset, method)
- Lock PIN and screensaver timeout

### mDNS
When connected to WiFi, HandyOS registers as `handyos.local`. The OTA update page is reachable at `http://handyos.local` from any device on the same network.

### Status Bar
Always-visible top bar showing:
- Live clock (HH:MM:SS, synced via NTP)
- 4-bar animated WiFi signal indicator
- Weather temperature **or** RSSI in dBm

### NTP Time Sync
Syncs with `pool.ntp.org` on boot (if WiFi connected). Feeds the real-time clock, prayer time calculations, and the status bar.

---

## 🖼️ UI Design

HandyOS uses a **Marauder firmware-inspired** aesthetic:
- Pure black background
- White text, accent-coloured headers and selections
- `[TITLE]` header style with yellow brackets
- Compact 1px hint bar at the bottom of every screen
- All menus use boxed rows with a `>` selection arrow
- Scrollable menus with a sidebar scroll indicator

### Accent Colours
The accent colour (used for headers, selections, and highlights) is user-selectable from 7 options: **Orange** (default), Cyan, Green, Pink, Yellow, Red, White. Applied globally throughout the entire OS.

---

## 🕹️ Button Reference

| Button | Primary Function |
|--------|----------------|
| UP | Navigate up / increase value |
| DOWN | Navigate down / decrease value |
| LEFT | Move left / previous option |
| RIGHT | Move right / next option |
| MID | Confirm / Select / Action |
| SET | Secondary action / Hold to pin shortcut |
| RST | Back / Exit (on every single screen) |

> **RST = Exit** is a hard design rule. Every screen, every app, every menu — RST always takes you back.

---

## 📜 Version History (Summary)

| Version | Highlights |
|---------|-----------|
| **v11.2** | 10-row Settings, Accent themes, Buzzer volume, Weather city in settings, Lock screen toggle |
| v11.1 | Simon Says, Habit Tracker, RGB Color Mixer, WiFi Signal Monitor, NVS Snake high score, WiFi signal bars in status bar |
| v10.0 | Deep sleep RTC resume, Deauth Detector, Port Scanner, MAC Randomizer, mDNS OTA, QR Code Generator, Pomodoro, Base Converter, Tone Generator, Tasbeeh, Lunar Phase, Tetris, 2048, Flappy Bird, App Shortcut, Lock Screen, Screensaver |
| v9.0 | NVS Notes, Morse Code, Digital Compass, Password Generator, Tally Counter, Deep Sleep, NVS WiFi save/load |
| v8.0 | Full Islamic / MuslimOS suite (Prayer, Qibla, Quran, Dhikr, Hijri, Adhan, Duas, 99 Names) |

---

## 🤝 Contributing

This is a personal hobby project. Feel free to fork, adapt, and build upon it. If you add cool features, a pull request is always welcome.

---

## 📄 License

No formal licence. Free to use, modify, and distribute for personal and educational purposes. Credit appreciated.

---

*Bismillah Ar-Rahman Ar-Raheem*  
*Built with ❤️ by musaiyaf*

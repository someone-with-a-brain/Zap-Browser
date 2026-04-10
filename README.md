# ⚡ ZAP Browser

A lightweight, privacy-first Chromium-based browser built in C++20.

> **Fast. Private. Smart. No accounts. No tracking. Local AI.**

---

## ✨ Features

| Feature | Detail |
|---|---|
| ⚡ Fast startup | Minimal background services |
| 🪶 Low RAM | Tab sleeping, shared CEF processes |
| 🔐 No accounts | Zero sign-in required |
| 🕵️ No telemetry | All Google services disabled at CEF level |
| 🌐 Tor mode | One-click anonymous routing |
| 🚫 Ad blocking | EasyList-based, O(1) lookup per request |
| 🍪 Cookie isolation | Per-tab isolated cookie stores |
| 🔐 DNS over HTTPS | Cloudflare/Quad9/Mullvad |
| 🧠 AI sidebar | Ollama-powered offline AI assistant |
| 📄 Page summarizer | Summarize any webpage locally |
| 📜 Local history | SQLite, never leaves your device |
| 🎨 Dark/light themes | Hot-swappable Qt stylesheets |

---

## 🏗️ Architecture

```
ZAP Browser (C++20)
├── UI Layer (Qt 6)
│   ├── MainWindow      — top-level shell
│   ├── TabBar          — custom tab manager
│   ├── AddressBar      — smart URL/search input
│   ├── AISidebar       — streaming AI chat panel
│   └── SettingsPage    — full settings dialog
│
├── Web Engine Layer (CEF)
│   ├── ZapApp          — CefApp (process/CLI config)
│   └── ZapClient       — CefClient (all event handlers)
│
├── Privacy Layer
│   ├── TorManager      — launches bundled tor.exe
│   ├── DohResolver     — DNS over HTTPS config
│   ├── BlockList       — ad/tracker domain blocking
│   └── CookieIsolator  — per-tab cookie isolation
│
├── AI Layer
│   ├── OllamaBridge    — async HTTP → Ollama API
│   └── PageSummarizer  — DOM text → Ollama prompt
│
├── Data Layer
│   ├── HistoryDB       — SQLite3 local history
│   └── BookmarkStore   — JSON local bookmarks
│
└── Update Layer
    └── AutoUpdater     — GitHub Releases version check
```

---

## 🧰 Prerequisites

| Tool | Version | Link |
|---|---|---|
| Visual Studio | 2022 (MSVC) | https://visualstudio.microsoft.com |
| CMake | 3.22+ | https://cmake.org |
| Qt 6 | 6.6+ | https://www.qt.io/download |
| CEF | Latest stable | https://cef-builds.spotifycdn.com |
| Git | Any | https://git-scm.com |
| Ollama | Any | https://ollama.com *(runtime only)* |
| Tor | 0.4.x | https://www.torproject.org *(bundled)* |

---

## 🚀 Build Instructions (Windows)

### Step 1 — Clone

```powershell
git clone https://github.com/someone-with-a-brain/zap-browser.git
cd zap-browser
```

### Step 2 — Download CEF

1. Go to: https://cef-builds.spotifycdn.com/index.html
2. Select: **Windows 64-bit** → **Standard Distribution** → latest stable
3. Extract into `third_party/cef/`

Directory should look like:
```
third_party/cef/
├── cmake/
├── include/
├── Release/
│   ├── libcef.dll
│   ├── libcef.lib
│   └── ...
└── Resources/
```

### Step 3 — Download SQLite (optional)

If SQLite3 is not system-installed:
1. Download from https://www.sqlite.org/download.html
2. Place `sqlite3.c` and `sqlite3.h` into `third_party/sqlite/`

### Step 4 — Download Tor

1. Download the **Expert Bundle** from https://www.torproject.org/download/tor/
2. Extract into `assets/tor/`:
```
assets/tor/
├── tor.exe
├── geoip
└── geoip6
```

### Step 5 — Configure

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2022_64" `
  -DCEF_ROOT="./third_party/cef"
```

### Step 6 — Build

```powershell
cmake --build build --config Release
```

### Step 7 — Run

```powershell
.\build\bin\Release\zap.exe
```

---

## 🧠 AI Setup

1. Install Ollama: https://ollama.com
2. Pull a model:
   ```
   ollama pull llama3:8b
   ```
3. Ollama starts automatically on boot. ZAP connects to `localhost:11434`.

If Ollama is not running, ZAP gracefully disables AI features — no crash.

---

## 🕵️ Tor Setup

Tor is bundled in `assets/tor/`. When you click **Tor: Off** in the toolbar:

1. ZAP launches `tor.exe` with the SOCKS5 port 9050
2. Bootstrap progress is shown in the status bar
3. Once 100% bootstrapped, all browser traffic routes through Tor
4. Verify at: https://check.torproject.org

---

## ⚙️ Settings

| Section | Option |
|---|---|
| 🔐 Privacy | Tor, DoH, ad blocking, clear-on-exit |
| 🧠 AI | Ollama enable, model selector |
| 🌐 Network | Custom proxy, Tor bridges |
| 🎨 Appearance | Dark/light theme, compact mode, font size |
| ⚡ Performance | Tab sleeping delay, hardware acceleration |

Settings are persisted via `QSettings` to the Windows Registry or `.ini`.

---

## 🗺️ Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close current tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+R` | Reload page |
| `Alt+←` | Go back |
| `Alt+→` | Go forward |
| `F12` | Toggle AI sidebar |

---

## 📁 Data Storage

All data is stored locally — never synced, never uploaded:

| Data | Location |
|---|---|
| History | `%APPDATA%/ZAP/ZapBrowser/history.db` |
| Bookmarks | `%APPDATA%/ZAP/ZapBrowser/bookmarks.json` |
| Settings | Windows Registry: `HKCU\Software\ZAP\ZapBrowser` |
| CEF cache | `%APPDATA%/ZAP/ZapBrowser/cache/` |
| Tor data | `%APPDATA%/ZAP/ZapBrowser/tor_data/` |

---

## 🔧 Development Notes

### Building without CEF (stub mode)
If CEF is not present, ZAP compiles in stub mode where tabs show placeholder labels. All other features (AI, Tor, history) still work.

### Adding a new block list
Drop any EasyList/hosts-format `.txt` file into `assets/blocklists/` and call `BlockList::LoadFile()` at startup.

### Changing the default search engine
In `AddressBar::resolveInput()`, change the DuckDuckGo URL to your preferred engine.

### Tab sleeping threshold
In `SettingsPage`, adjust the "Sleep after" spinner. `TabBar::SetTabSleeping()` is called by `MainWindow`'s inactivity timer.

---

## 📜 License

MIT License — do whatever you want, just don't claim you made Chromium.

---

## 🙏 Credits

- [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef) — web engine
- [Qt](https://www.qt.io) — UI framework
- [Tor Project](https://www.torproject.org) — anonymous routing
- [Ollama](https://ollama.com) — local AI inference
- [EasyList](https://easylist.to) — ad block lists

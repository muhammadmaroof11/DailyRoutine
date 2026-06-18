# Daily Routine Dashboard - C++ Edition

This repository contains the high-performance, native C++ translation of the ADHD-friendly **Daily Routine Dashboard Widget**.

By rewriting the platform in native C++ using **Qt6**, memory footprint and startup times are optimized to absolute minimums, and system calls run directly at CPU speed.

---

## 🎨 Visual Aesthetics & UX Practices
- **Frameless Windowing**: Clean, overlay-style widget layout with smooth custom Title-less dragging.
- **Translucent Layering**: Incorporates glassmorphic drop shadows (`QGraphicsDropShadowEffect`) and alpha blending for dark backgrounds.
- **Custom Vector Graphics**: Complete hand-crafted graphics utilizing `QPainter` for high-DPI weekly column charts and GitHub-style consistency density grids.
- **Native Sound Cues**: Plays WAV cues via Win32 `PlaySoundW` to bypass heavy multimedia frameworks and guarantee sound delivery.

---

## 📁 File Structure
- `CMakeLists.txt`: Configures standard compile steps, automatically searches for Qt6 packages, and disables terminal console boot flags on Windows.
- `main.cpp`: Orchestrates startup lifecycle, initiates the Onboarding Stacked Wizard, handles system tray icon, and binds rollover clocks.
- `storage.h` / `storage.cpp`: Fast JSON parsing and state management with legacy schema migration using Qt Core libraries.
- `logic.h` / `logic.cpp`: Waveform synthesizers and registry run key edits.
- `analytics.h` / `analytics.cpp`: Vector chart canvas widgets and historical summaries.
- `ui.h` / `ui.cpp`: Full UI classes including list checkboxes, settings dialogue, onboarding wizard page-deck, and countdown timer.

---

## 🛠️ Compilation & Setup

### Prerequisites
To build the application, make sure you have the following installed on your machine:
1. **CMake** (v3.16 or newer)
2. **C++ Compiler** supporting C++17 (MSVC 2019+, GCC 9+, or Clang)
3. **Qt6 SDK** (specifically the `Core`, `Gui`, and `Widgets` packages)

### Building on Windows
Open your terminal inside this `routine-widget-cpp` directory and run:

```powershell
# Create build directory
mkdir build
cd build

# Configure build with CMake (adjust path to Qt if not in default location)
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x/msvc2019_64" ..

# Compile executable
cmake --build . --config Release
```

The compiled `DailyRoutineDashboard.exe` will be generated in `build/Release/`.
To run it, copy the required Qt dll dependencies to the directory (e.g. by running `windeployqt DailyRoutineDashboard.exe`).

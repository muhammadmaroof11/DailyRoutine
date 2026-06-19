# Daily Routine Dashboard

A premium, lightweight, open-source C++ and Qt6 daily habit organizer and focus grounding widget designed to combat distraction, build routines, and keep you on track.

---

## 🌟 Key Features

*   **Customized Onboarding:** Tailored setup requesting name, age, and main productivity focus.
*   **Time-Based Task Recognition:** Simple checklist items run alongside time-effective tasks (e.g. `(1h)`, `(30m)`). Time-based tasks display a play button to launch focus sessions.
*   **Focus Grounding Beeps:** Adjustable interval chime reminders (1-120 mins) to gently redirect attention during tasks. Features a custom pop-up dialog with modern outside `-` and `+` adjustment controls.
*   **Streak Tracking & Analytics:** Real-time analytics charts monitoring your daily routine completion rates and habits over time.
*   **System Tray Integration & Startup Support:** Minimizes to the Windows System Tray notifications area rather than cluttering the taskbar. Options to launch automatically on Windows boot.
*   **Harmonious Color Palettes:** Dynamic themes support: *Catppuccin*, *Nord*, *Cyberpunk*, and *Sakura*.
*   **Offline First & Safe:** All configurations, logs, and stats are saved locally on your device (`%LOCALAPPDATA%\DailyRoutineDashboard`).

---

## 🛠️ Build & Installation

### Prerequisites
*   **OS:** Windows 10/11
*   **Compiler:** MSVC 2022 (Visual Studio 17+)
*   **Framework:** Qt 6.8+ (MSVC 2022 64-bit SDK)
*   **Build Tool:** CMake 3.20+
*   **Installer Compiler:** Inno Setup 6

### Compilation Steps
1. Configure and build via CMake:
   ```cmd
   cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\path\to\Qt\6.8.0\msvc2022_64"
   cmake --build build --config Release
   ```
2. Deploy Qt library dependencies:
   ```cmd
   windeployqt.exe build\Release\DailyRoutineDashboard.exe
   ```
3. Compile the offline installer using Inno Setup:
   ```cmd
   ISCC.exe setup.iss
   ```

---

## 👤 Developer Profile

**Muhammad Maroof**
*   **GitHub:** [@muhammadmaroof11](https://github.com/muhammadmaroof11)
*   **LinkedIn:** [mmaroof11](https://www.linkedin.com/in/mmaroof11/)
*   **WhatsApp:** +923051526463

---

## 📄 License

This project is open-source and licensed under the **MIT License**. For more information, please read the [LICENSE](LICENSE) file.

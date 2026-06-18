#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QTimer>
#include "storage.h"
#include "logic.h"
#include "ui.h"

int main(int argc, char *argv[]) {
    // 1. Initialize Qt Application
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // 2. Ensure synthesized wav audio files are pre-generated
    Logic::generateAllSounds();

    // 3. Load configurations and current state
    Storage::TasksConfig tasksDict = Storage::loadTasks();
    Storage::AppState state = Storage::loadState(tasksDict);

    // 4. Initialize onboarding deck if user is new
    if (!state.onboarded) {
        OnboardingDialog wizard(state, state.themeSelection);
        if (wizard.exec() != QDialog::Accepted) {
            return 0; // Terminate if user aborted onboarding
        }
        // Reload saved state after onboarding completions
        state = Storage::loadState(tasksDict);
    }

    // 5. Instaniate main dashboard window
    MainWindow w(state, tasksDict);
    w.show();

    // 6. Handle initial startup day rollover checks
    auto rollover = Logic::checkDailyReset(state, tasksDict);
    if (rollover.first) {
        w.showResetBanner(rollover.second);
        w.buildTaskItems();
    }

    // 7. Setup system tray controls
    QSystemTrayIcon tray(QIcon(":/icon.png"), &app);
    // Note: If resource icon is missing, Qt defaults to blank, which is safe.
    // Let's create a standard active icon representation or text tray tooltip.
    tray.setToolTip("Daily Routine Dashboard");

    QMenu trayMenu;
    QAction *actShow = trayMenu.addAction("Show Dashboard");
    QAction *actHide = trayMenu.addAction("Hide Dashboard");
    trayMenu.addSeparator();
    QAction *actSettings = trayMenu.addAction("Settings");
    QAction *actAnalytics = trayMenu.addAction("Analytics");
    trayMenu.addSeparator();
    QAction *actExit = trayMenu.addAction("Exit");

    QObject::connect(actShow, &QAction::triggered, &w, [&w]() {
        w.show();
        w.raise();
        w.activateWindow();
    });

    QObject::connect(actHide, &QAction::triggered, &w, &MainWindow::hide);

    QObject::connect(actSettings, &QAction::triggered, &w, &MainWindow::openSettings);

    QObject::connect(actAnalytics, &QAction::triggered, &w, &MainWindow::openAnalytics);

    QObject::connect(actExit, &QAction::triggered, &app, [&app, &w]() {
        w.saveWidgetState();
        app.quit();
    });

    tray.setContextMenu(&trayMenu);
    tray.show();

    // Toggle visibility on tray double click
    QObject::connect(&tray, &QSystemTrayIcon::activated, &w, [&w](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
            if (w.isVisible()) {
                w.hide();
            } else {
                w.show();
                w.raise();
                w.activateWindow();
            }
        }
    });

    // 8. Connect background rollover timer (checks for midnight shifts every 15 minutes)
    QTimer rolloverTimer;
    QObject::connect(&rolloverTimer, &QTimer::timeout, &w, [&state, &tasksDict, &w]() {
        auto check = Logic::checkDailyReset(state, tasksDict);
        if (check.first) {
            w.showResetBanner(check.second);
            w.buildTaskItems();
        }
    });
    rolloverTimer.start(15 * 60 * 1000); // 15 mins

    // 9. Run the main execution event loop
    return app.exec();
}

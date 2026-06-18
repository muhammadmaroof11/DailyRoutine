#ifndef LOGIC_H
#define LOGIC_H

#include "storage.h"

namespace Logic {
    // Check and process midnight date transition resets
    QPair<bool, QString> checkDailyReset(Storage::AppState &state, const Storage::TasksConfig &tasksConfig);

    // Audio Wave generation utilities
    QString generatePopSound(const QString &filename = "pop.wav");
    QString generateSparkleSound(const QString &filename = "sparkle.wav");
    QString generateChimeSound(const QString &filename = "chime.wav");
    QString generateAlertSound(const QString &filename = "alert.wav");
    void generateAllSounds();

    // Windows Startup Registry manager
    bool setStartup(bool enabled);
    bool isStartupEnabled();
}

#endif // LOGIC_H

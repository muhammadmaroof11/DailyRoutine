#ifndef STORAGE_H
#define STORAGE_H

#include <QString>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QList>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDate>

namespace Storage {
    // Config files paths resolver
    QString getPath(const QString &filename);

    // Tasks structure structures
    struct TasksConfig {
        QString selectedProfile;
        QMap<QString, QList<QString>> profiles;
    };

    // State structures
    struct AppState {
        QString currentDate;
        QList<bool> completed;
        int streak = 0;
        int longestStreak = 0;
        int windowX = 200;
        int windowY = 200;
        bool pinned = false;
        bool darkMode = true;
        QString soundSelection = "pop";
        QString themeSelection = "catppuccin";
        int customAlertMinutes = 5;
        bool onboarded = false;
        QString userName = "Friend";
        QMap<QString, QList<bool>> profileStates;
    };

    TasksConfig loadTasks();
    void saveTasks(const TasksConfig &config);
    
    AppState loadState(const TasksConfig &tasksConfig);
    void saveState(const AppState &state);
    
    void appendToLog(const QString &dateStr, int completed, int total, int streak);
}

#endif // STORAGE_H

#include "storage.h"
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>

namespace Storage {

QString getPath(const QString &filename) {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath(filename);
}

TasksConfig loadTasks() {
    TasksConfig config;
    QString path = getPath("tasks.json");
    QFile file(path);

    // Default configuration if file is missing
    if (!file.exists()) {
        config.selectedProfile = "Daily Habits";
        config.profiles["Daily Habits"] = {
            "Exercise (1h)", 
            "Study (1h)", 
            "Coding (2h)", 
            "Reading (30m)", 
            "Walk (30m)"
        };
        config.profiles["Work Focus"] = {
            "Email triaging (15m)",
            "Deep Work (2h)",
            "System Review (30m)"
        };
        saveTasks(config);
        return config;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        
        // Handle migration from legacy single list structures
        if (obj.contains("profiles") && obj["profiles"].isObject()) {
            config.selectedProfile = obj["selected_profile"].toString("Daily Habits");
            QJsonObject profsObj = obj["profiles"].toObject();
            for (auto it = profsObj.begin(); it != profsObj.end(); ++it) {
                QJsonArray arr = it.value().toArray();
                QList<QString> list;
                for (const auto &val : arr) {
                    list.append(val.toString());
                }
                config.profiles[it.key()] = list;
            }
        } else {
            // Legacy single tasks list detected - migrate to profiles structure
            config.selectedProfile = "Daily Habits";
            QList<QString> list;
            QJsonArray arr = obj["tasks"].toArray();
            if (arr.isEmpty() && doc.isArray()) {
                // Was it a raw array?
                arr = doc.array();
            }
            for (const auto &val : arr) {
                if (val.isString()) list.append(val.toString());
                else if (val.isObject()) list.append(val.toObject()["text"].toString());
            }
            if (list.isEmpty()) {
                list = {"Exercise (1h)", "Study (1h)", "Coding (2h)"};
            }
            config.profiles["Daily Habits"] = list;
            saveTasks(config);
        }
    }
    return config;
}

void saveTasks(const TasksConfig &config) {
    QJsonObject root;
    root["selected_profile"] = config.selectedProfile;
    QJsonObject profs;
    for (auto it = config.profiles.begin(); it != config.profiles.end(); ++it) {
        QJsonArray arr;
        for (const auto &task : it.value()) {
            arr.append(task);
        }
        profs[it.key()] = arr;
    }
    root["profiles"] = profs;

    QFile file(getPath("tasks.json"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

AppState loadState(const TasksConfig &tasksConfig) {
    AppState state;
    QString path = getPath("state.json");
    QFile file(path);

    QString activeProfile = tasksConfig.selectedProfile;
    int numTasks = tasksConfig.profiles.value(activeProfile).size();

    // Default initialization
    state.currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    state.completed = QList<bool>(numTasks, false);
    for (auto it = tasksConfig.profiles.begin(); it != tasksConfig.profiles.end(); ++it) {
        state.profileStates[it.key()] = QList<bool>(it.value().size(), false);
    }

    if (!file.exists()) {
        saveState(state);
        return state;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        state.currentDate = obj["current_date"].toString(state.currentDate);
        state.streak = obj["streak"].toInt(0);
        state.longestStreak = obj["longest_streak"].toInt(0);
        state.windowX = obj["window_x"].toInt(200);
        state.windowY = obj["window_y"].toInt(200);
        state.pinned = obj["pinned"].toBool(false);
        state.darkMode = obj["dark_mode"].toBool(true);
        state.soundSelection = obj["sound_selection"].toString("pop");
        state.themeSelection = obj["theme_selection"].toString("catppuccin");
        state.customAlertMinutes = obj["custom_alert_minutes"].toInt(5);
        state.onboarded = obj["onboarded"].toBool(false);
        state.userName = obj["user_name"].toString("Friend");
        state.userAge = obj["user_age"].toInt(25);
        state.focusGoal = obj["focus_goal"].toString("Productivity");

        // Parse profile_states map
        QJsonObject statesObj = obj["profile_states"].toObject();
        for (auto it = tasksConfig.profiles.begin(); it != tasksConfig.profiles.end(); ++it) {
            QString profName = it.key();
            QList<bool> list(it.value().size(), false);
            if (statesObj.contains(profName)) {
                QJsonArray arr = statesObj[profName].toArray();
                for (int i = 0; i < qMin(arr.size(), list.size()); ++i) {
                    list[i] = arr[i].toBool();
                }
            }
            state.profileStates[profName] = list;
        }

        // Active profile checkboxes
        if (obj.contains("completed")) {
            QJsonArray arr = obj["completed"].toArray();
            QList<bool> activeList(numTasks, false);
            for (int i = 0; i < qMin(arr.size(), activeList.size()); ++i) {
                activeList[i] = arr[i].toBool();
            }
            state.completed = activeList;
            state.profileStates[activeProfile] = activeList;
        } else {
            state.completed = state.profileStates[activeProfile];
        }
    }
    return state;
}

void saveState(const AppState &state) {
    QJsonObject root;
    root["current_date"] = state.currentDate;
    root["streak"] = state.streak;
    root["longest_streak"] = state.longestStreak;
    root["window_x"] = state.windowX;
    root["window_y"] = state.windowY;
    root["pinned"] = state.pinned;
    root["dark_mode"] = state.darkMode;
    root["sound_selection"] = state.soundSelection;
    root["theme_selection"] = state.themeSelection;
    root["custom_alert_minutes"] = state.customAlertMinutes;
    root["onboarded"] = state.onboarded;
    root["user_name"] = state.userName;
    root["user_age"] = state.userAge;
    root["focus_goal"] = state.focusGoal;

    QJsonArray compArr;
    for (bool val : state.completed) {
        compArr.append(val);
    }
    root["completed"] = compArr;

    QJsonObject statesObj;
    for (auto it = state.profileStates.begin(); it != state.profileStates.end(); ++it) {
        QJsonArray arr;
        for (bool val : it.value()) {
            arr.append(val);
        }
        statesObj[it.key()] = arr;
    }
    root["profile_states"] = statesObj;

    QFile file(getPath("state.json"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void appendToLog(const QString &dateStr, int completed, int total, int streak) {
    QFile file(getPath("log.csv"));
    bool isNew = !file.exists();
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        if (isNew) {
            out << "date,completed,total,streak\n";
        }
        out << dateStr << "," << completed << "," << total << "," << streak << "\n";
        file.close();
    }
}

}

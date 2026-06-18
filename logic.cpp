#include "logic.h"
#include <QCoreApplication>
#include <QSettings>
#include <cmath>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Logic {

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    int32_t chunkSize;
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1Id[4] = {'f', 'm', 't', ' '};
    int32_t subchunk1Size = 16;
    int16_t audioFormat = 1; // PCM
    int16_t numChannels = 1;
    int32_t sampleRate = 44100;
    int32_t byteRate;
    int16_t blockAlign;
    int16_t bitsPerSample = 16;
    char subchunk2Id[4] = {'d', 'a', 't', 'a'};
    int32_t subchunk2Size;
};
#pragma pack(pop)

QPair<bool, QString> checkDailyReset(Storage::AppState &state, const Storage::TasksConfig &tasksConfig) {
    if (state.currentDate.isEmpty()) {
        return qMakePair(false, QString());
    }

    QDate lastDate = QDate::fromString(state.currentDate, "yyyy-MM-dd");
    if (!lastDate.isValid()) {
        state.currentDate = QDate::currentDate().toString("yyyy-MM-dd");
        Storage::saveState(state);
        return qMakePair(false, QString());
    }

    QDate today = QDate::currentDate();
    int delta = lastDate.daysTo(today);

    if (delta > 0) {
        int completedCount = 0;
        for (bool val : state.completed) {
            if (val) completedCount++;
        }
        int totalCount = state.completed.size();

        // 1. Log previous day's results
        Storage::appendToLog(state.currentDate, completedCount, totalCount, state.streak);

        // 2. Evaluate streak
        bool allDone = (completedCount == totalCount && totalCount > 0);
        int streakWas = state.streak;
        QString brokenMessage;

        if (delta == 1) {
            if (!allDone) {
                state.streak = 0;
                brokenMessage = "It's a new day! Let's start fresh today, no guilt. ☀️";
            } else {
                brokenMessage = QString("Keep the momentum going! 🔥 Streak is active at %1 days.").arg(streakWas);
            }
        } else {
            state.streak = 0;
            brokenMessage = "Welcome back! Let's start a new streak today. ☀️";
        }

        // 3. Reset checklists for all profiles
        for (auto it = state.profileStates.begin(); it != state.profileStates.end(); ++it) {
            QList<bool> list(it.value().size(), false);
            state.profileStates[it.key()] = list;
        }

        // Sync current active completed array
        QString selected = tasksConfig.selectedProfile;
        state.completed = state.profileStates.value(selected);
        state.currentDate = today.toString("yyyy-MM-dd");

        // Sync longest streak
        if (state.streak > state.longestStreak) {
            state.longestStreak = state.streak;
        }

        Storage::saveState(state);
        return qMakePair(true, brokenMessage);
    }

    return qMakePair(false, QString());
}

QString generatePopSound(const QString &filename) {
    QString filepath = Storage::getPath(filename);
    QFile file(filepath);
    if (file.exists()) {
        return filepath;
    }

    int sampleRate = 44100;
    double duration = 0.12;
    int numSamples = static_cast<int>(sampleRate * duration);

    if (file.open(QIODevice::WriteOnly)) {
        WAVHeader header;
        header.subchunk2Size = numSamples * sizeof(int16_t);
        header.chunkSize = 36 + header.subchunk2Size;
        header.byteRate = sampleRate * sizeof(int16_t);
        header.blockAlign = sizeof(int16_t);

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double freq = 350.0 + 450.0 * (t / duration);
            double envelope = std::exp(-7.0 * (t / duration));
            double val = std::sin(2.0 * M_PI * freq * t) * envelope;
            int16_t sample = static_cast<int16_t>(val * 14000);
            file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        }
        file.close();
    }
    return filepath;
}

QString generateSparkleSound(const QString &filename) {
    QString filepath = Storage::getPath(filename);
    QFile file(filepath);
    if (file.exists()) {
        return filepath;
    }

    int sampleRate = 44100;
    double duration = 0.25;
    int numSamples = static_cast<int>(sampleRate * duration);

    if (file.open(QIODevice::WriteOnly)) {
        WAVHeader header;
        header.subchunk2Size = numSamples * sizeof(int16_t);
        header.chunkSize = 36 + header.subchunk2Size;
        header.byteRate = sampleRate * sizeof(int16_t);
        header.blockAlign = sizeof(int16_t);

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double env1 = std::exp(-15.0 * t);
            double f1 = 1600.0 + 300.0 * std::sin(35.0 * t);
            double val1 = std::sin(2.0 * M_PI * f1 * t) * env1;

            double val2 = 0.0;
            if (t > 0.07) {
                double t2 = t - 0.07;
                double env2 = std::exp(-12.0 * t2);
                double f2 = 1900.0 + 400.0 * std::sin(25.0 * t2);
                val2 = std::sin(2.0 * M_PI * f2 * t2) * env2;
            }

            double val = val1 + val2 * 0.7;
            int16_t sample = static_cast<int16_t>(val * 11000);
            file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        }
        file.close();
    }
    return filepath;
}

QString generateChimeSound(const QString &filename) {
    QString filepath = Storage::getPath(filename);
    QFile file(filepath);
    if (file.exists()) {
        return filepath;
    }

    int sampleRate = 44100;
    double duration = 0.65;
    int numSamples = static_cast<int>(sampleRate * duration);

    if (file.open(QIODevice::WriteOnly)) {
        WAVHeader header;
        header.subchunk2Size = numSamples * sizeof(int16_t);
        header.chunkSize = 36 + header.subchunk2Size;
        header.byteRate = sampleRate * sizeof(int16_t);
        header.blockAlign = sizeof(int16_t);

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        QList<double> freqs = {523.25, 659.25, 783.99, 1046.50};

        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double envelope = std::exp(-4.2 * t);
            double signal = 0.0;
            for (double f : freqs) {
                signal += std::sin(2.0 * M_PI * f * t);
            }
            signal = (signal / freqs.size()) * envelope;
            int16_t sample = static_cast<int16_t>(signal * 13000);
            file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        }
        file.close();
    }
    return filepath;
}

QString generateAlertSound(const QString &filename) {
    QString filepath = Storage::getPath(filename);
    QFile file(filepath);
    if (file.exists()) {
        return filepath;
    }

    int sampleRate = 44100;
    double duration = 0.3;
    int numSamples = static_cast<int>(sampleRate * duration);

    if (file.open(QIODevice::WriteOnly)) {
        WAVHeader header;
        header.subchunk2Size = numSamples * sizeof(int16_t);
        header.chunkSize = 36 + header.subchunk2Size;
        header.byteRate = sampleRate * sizeof(int16_t);
        header.blockAlign = sizeof(int16_t);

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double val = 0.0;
            if (t >= 0.0 && t < 0.05) {
                double env = std::sin(M_PI * (t / 0.05));
                val = std::sin(2.0 * M_PI * 580.0 * t) * env;
            } else if (t >= 0.09 && t < 0.14) {
                double t2 = t - 0.09;
                double env = std::sin(M_PI * (t2 / 0.05));
                val = std::sin(2.0 * M_PI * 580.0 * t2) * env;
            }
            int16_t sample = static_cast<int16_t>(val * 9000);
            file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        }
        file.close();
    }
    return filepath;
}

void generateAllSounds() {
    generatePopSound();
    generateSparkleSound();
    generateChimeSound();
    generateAlertSound();
}

bool setStartup(bool enabled) {
    // Utilize Qt's cross-platform registry manipulator (handles Windows run registry paths automatically)
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) {
        QString path = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        bootSettings.setValue("DailyRoutineDashboard", "\"" + path + "\"");
    } else {
        bootSettings.remove("DailyRoutineDashboard");
    }
    return true;
}

bool isStartupEnabled() {
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return bootSettings.contains("DailyRoutineDashboard");
}

}

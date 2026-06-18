#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QDate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

class WeeklyChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit WeeklyChartWidget(const QString &themeName = "catppuccin", QWidget *parent = nullptr);
    void setTheme(const QString &themeName);
    void reloadData();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_themeName;
    QList<QPair<QString, double>> m_weeklyRates;
};

class HeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeatmapWidget(const QString &themeName = "catppuccin", QWidget *parent = nullptr);
    void setTheme(const QString &themeName);
    void reloadData();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_themeName;
    QMap<QString, double> m_history;
};

class AnalyticsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AnalyticsDialog(const QString &themeName = "catppuccin", int currentStreak = 0, int longestStreak = 0, QWidget *parent = nullptr);

private:
    void applyTheme();

    QString m_themeName;
    int m_currentStreak;
    int m_longestStreak;

    QFrame *m_outerFrame;
    QLabel *m_lblTitle;
    QLabel *m_lblCurrentStreak;
    QLabel *m_lblLongestStreak;
    WeeklyChartWidget *m_weeklyChart;
    HeatmapWidget *m_heatmap;
    QPushButton *m_btnClose;
};

#endif // ANALYTICS_H

#include "analytics.h"
#include "storage.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMap>

// Unified Theme Registry
struct ThemeColors {
    QString bg;
    QString fg;
    QString accent;
    QString border;
    QString card;
    QString success;
};

static const QMap<QString, ThemeColors> THEMES = {
    {"catppuccin", {"#1e1e2e", "#cdd6f4", "#cba6f7", "#585b70", "#313244", "#a6e3a1"}},
    {"nord",       {"#2e3440", "#d8dee9", "#88c0d0", "#4c566a", "#3b4252", "#a3be8c"}},
    {"cyberpunk",  {"#0b0813", "#00f0ff", "#ff007f", "#3d1b5c", "#1a103c", "#39ff14"}},
    {"sakura",     {"#2d1b24", "#fff0f5", "#ffb7c5", "#5c3d4a", "#422835", "#ff69b4"}}
};

// --- Weekly Chart Widget ---
WeeklyChartWidget::WeeklyChartWidget(const QString &themeName, QWidget *parent)
    : QWidget(parent), m_themeName(themeName) {
    setMinimumSize(240, 110);
    reloadData();
}

void WeeklyChartWidget::setTheme(const QString &themeName) {
    m_themeName = themeName;
    update();
}

void WeeklyChartWidget::reloadData() {
    m_weeklyRates.clear();
    QList<QPair<QString, double>> rawLogs;

    QFile file(Storage::getPath("log.csv"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString header = in.readLine(); // skip header
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(',');
            if (parts.size() >= 3) {
                QString dateStr = parts[0];
                int completed = parts[1].toInt();
                int total = parts[2].toInt();
                double rate = (total > 0) ? (static_cast<double>(completed) / total) : 0.0;
                rawLogs.append(qMakePair(dateStr, rate));
            }
        }
        file.close();
    }

    // Keep only the last 7 entries
    int startIdx = qMax(0, rawLogs.size() - 7);
    for (int i = startIdx; i < rawLogs.size(); ++i) {
        m_weeklyRates.append(rawLogs[i]);
    }

    // Pad with placeholders if less than 7 days of records exist
    while (m_weeklyRates.size() < 7) {
        QDate pastDate = QDate::currentDate().addDays(-(7 - m_weeklyRates.size()));
        m_weeklyRates.prepend(qMakePair(pastDate.toString("MM-dd"), 0.0));
    }
}

void WeeklyChartWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);
    
    int w = width();
    int h = height();
    int paddingLeft = 10;
    int paddingRight = 10;
    int paddingTop = 15;
    int paddingBottom = 20;

    int chartWidth = w - paddingLeft - paddingRight;
    int chartHeight = h - paddingTop - paddingBottom;
    int numBars = m_weeklyRates.size();
    
    double colWidth = static_cast<double>(chartWidth) / numBars;
    double barWidth = colWidth * 0.6;
    double barSpacing = colWidth * 0.4;

    // Draw reference grid lines
    painter.setPen(QPen(QColor(colors.border), 1, Qt::DashLine));
    painter.drawLine(paddingLeft, paddingTop, w - paddingRight, paddingTop);
    painter.drawLine(paddingLeft, paddingTop + chartHeight / 2, w - paddingRight, paddingTop + chartHeight / 2);
    painter.drawLine(paddingLeft, paddingTop + chartHeight, w - paddingRight, paddingTop + chartHeight);

    for (int i = 0; i < numBars; ++i) {
        double rate = m_weeklyRates[i].second;
        QString dateLabel = m_weeklyRates[i].first;
        if (dateLabel.length() > 5) {
            // Reformat yyyy-MM-dd -> MM-dd
            QDate date = QDate::fromString(dateLabel, "yyyy-MM-dd");
            if (date.isValid()) {
                dateLabel = date.toString("MM-dd");
            }
        }

        double barHeight = chartHeight * rate;
        double x = paddingLeft + i * colWidth + barSpacing / 2.0;
        double y = paddingTop + chartHeight - barHeight;

        // Draw bar
        QRectF rect(x, y, barWidth, qMax(barHeight, 2.0));
        painter.setPen(Qt::NoPen);
        QColor barColor = QColor(colors.accent);
        if (rate >= 1.0) {
            barColor = QColor(colors.success);
        }
        painter.setBrush(barColor);
        painter.drawRoundedRect(rect, 4, 4);

        // Draw labels below
        painter.setPen(QColor(colors.fg));
        painter.setFont(QFont("Segoe UI", 8));
        QRectF textRect(x - barSpacing/2.0, paddingTop + chartHeight + 2, colWidth, 15);
        painter.drawText(textRect, Qt::AlignCenter, dateLabel);
    }
}


// --- Heatmap Widget ---
HeatmapWidget::HeatmapWidget(const QString &themeName, QWidget *parent)
    : QWidget(parent), m_themeName(themeName) {
    setMinimumSize(240, 60);
    reloadData();
}

void HeatmapWidget::setTheme(const QString &themeName) {
    m_themeName = themeName;
    update();
}

void HeatmapWidget::reloadData() {
    m_history.clear();
    QFile file(Storage::getPath("log.csv"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString header = in.readLine();
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(',');
            if (parts.size() >= 3) {
                QString dateStr = parts[0];
                int completed = parts[1].toInt();
                int total = parts[2].toInt();
                double rate = (total > 0) ? (static_cast<double>(completed) / total) : 0.0;
                m_history[dateStr] = rate;
            }
        }
        file.close();
    }
}

void HeatmapWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);
    
    int boxes = 30;
    int cols = 10;
    int rows = 3;
    int boxSize = 16;
    int spacing = 4;

    int xOffset = (width() - (cols * (boxSize + spacing) - spacing)) / 2;
    int yOffset = (height() - (rows * (boxSize + spacing) - spacing)) / 2;

    QDate today = QDate::currentDate();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int dayOffset = -((rows - 1 - r) * cols + (cols - 1 - c));
            QDate date = today.addDays(dayOffset);
            QString dateStr = date.toString("yyyy-MM-dd");

            double rate = m_history.value(dateStr, -1.0);

            QColor boxColor(colors.card);
            if (rate > 0.0) {
                QColor accentColor(colors.accent);
                if (rate >= 0.99) {
                    boxColor = QColor(colors.success);
                } else if (rate >= 0.5) {
                    boxColor = accentColor;
                } else {
                    boxColor = accentColor;
                    boxColor.setAlphaF(0.4); // lighter transparency
                }
            } else if (rate == 0.0) {
                boxColor = QColor(colors.border);
            }

            int x = xOffset + c * (boxSize + spacing);
            int y = yOffset + r * (boxSize + spacing);

            painter.setPen(Qt::NoPen);
            painter.setBrush(boxColor);
            painter.drawRoundedRect(x, y, boxSize, boxSize, 3, 3);
        }
    }
}


// --- Analytics Dialog ---
AnalyticsDialog::AnalyticsDialog(const QString &themeName, int currentStreak, int longestStreak, QWidget *parent)
    : QDialog(parent), m_themeName(themeName), m_currentStreak(currentStreak), m_longestStreak(longestStreak) {
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 390);

    // Main frame wrapper
    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("OuterFrame");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 95));
    m_outerFrame->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_outerFrame);

    QVBoxLayout *innerLayout = new QVBoxLayout(m_outerFrame);
    innerLayout->setContentsMargins(15, 15, 15, 15);
    innerLayout->setSpacing(10);

    // Title label
    m_lblTitle = new QLabel("WEEKLY STATISTICS", m_outerFrame);
    m_lblTitle->setFont(QFont("Segoe UI", 11, QFont::Bold));
    m_lblTitle->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(m_lblTitle);

    // Streaks summary card
    QFrame *streakCard = new QFrame(m_outerFrame);
    streakCard->setObjectName("StreakCard");
    QHBoxLayout *streakLay = new QHBoxLayout(streakCard);
    streakLay->setContentsMargins(8, 8, 8, 8);

    m_lblCurrentStreak = new QLabel(QString("🔥 Current: %1").arg(m_currentStreak), streakCard);
    m_lblCurrentStreak->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_lblCurrentStreak->setAlignment(Qt::AlignCenter);

    m_lblLongestStreak = new QLabel(QString("🏆 Best: %1").arg(m_longestStreak), streakCard);
    m_lblLongestStreak->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_lblLongestStreak->setAlignment(Qt::AlignCenter);

    streakLay->addWidget(m_lblCurrentStreak);
    streakLay->addWidget(m_lblLongestStreak);
    innerLayout->addWidget(streakCard);

    // Chart component
    m_weeklyChart = new WeeklyChartWidget(m_themeName, m_outerFrame);
    innerLayout->addWidget(m_weeklyChart);

    // Section title
    QLabel *heatmapTitle = new QLabel("Consistency (30 Days)", m_outerFrame);
    heatmapTitle->setFont(QFont("Segoe UI", 9, QFont::Bold));
    heatmapTitle->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(heatmapTitle);

    // Heatmap component
    m_heatmap = new HeatmapWidget(m_themeName, m_outerFrame);
    innerLayout->addWidget(m_heatmap);

    innerLayout->addStretch();

    // Footer actions
    m_btnClose = new QPushButton("Close", m_outerFrame);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    innerLayout->addWidget(m_btnClose);

    applyTheme();
}

void AnalyticsDialog::applyTheme() {
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    m_outerFrame->setStyleSheet(QString(
        "QFrame#OuterFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 12px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QFrame#StreakCard {"
        "  background-color: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  color: %3;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
    ).arg(colors.bg, colors.border, colors.fg, colors.card));
}

#include "ui.h"
#include "analytics.h"
#include "logic.h"
#include <QPainter>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QPropertyAnimation>
#include <QDir>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <windows.h>
#include <mmsystem.h>
#include <cmath>

#pragma comment(lib, "winmm.lib")

// Theme declarations match analytics.cpp
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

int parseDuration(const QString &text) {
    QRegularExpression re("\\(([^)]+)\\)");
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        QString content = match.captured(1).toLower();
        int totalSecs = 0;
        QRegularExpression hrRe("(\\d+)h");
        QRegularExpressionMatch hrMatch = hrRe.match(content);
        if (hrMatch.hasMatch()) {
            totalSecs += hrMatch.captured(1).toInt() * 3600;
        }
        QRegularExpression minRe("(\\d+)m");
        QRegularExpressionMatch minMatch = minRe.match(content);
        if (minMatch.hasMatch()) {
            totalSecs += minMatch.captured(1).toInt() * 60;
        }
        if (totalSecs > 0) return totalSecs;
    }
    return 25 * 60; // Default: 25 minutes
}

bool isTimeBasedTask(const QString &text) {
    QRegularExpression re("\\(([^)]+)\\)");
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        QString content = match.captured(1).toLower();
        return content.contains("h") || content.contains("m");
    }
    return false;
}

void playWavSound(const QString &filename) {
    QString path = QDir::toNativeSeparators(Storage::getPath(filename));
    PlaySoundW(reinterpret_cast<LPCWSTR>(path.utf16()), NULL, SND_FILENAME | SND_ASYNC);
}

// --- EditableLabel ---
EditableLabel::EditableLabel(const QString &text, QWidget *parent) : QLabel(text, parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}
void EditableLabel::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    emit doubleClicked();
}

// --- IconButton ---
IconButton::IconButton(IconType type, const QString &themeName, QWidget *parent)
    : QPushButton(parent), m_type(type), m_themeName(themeName) {
    setFlat(true);
    setFixedSize(28, 28);
}

void IconButton::setTheme(const QString &themeName) {
    m_themeName = themeName;
    update();
}

void IconButton::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void IconButton::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

void IconButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);
    QColor iconColor = QColor(colors.fg);
    
    if (m_hovered) {
        painter.setPen(Qt::NoPen);
        QColor hoverBg = QColor(colors.accent);
        hoverBg.setAlphaF(0.15);
        painter.setBrush(hoverBg);
        painter.drawRoundedRect(rect(), 8, 8);
        iconColor = QColor(colors.accent);
    } else {
        if (m_type == Trash || m_type == Play) {
            iconColor.setAlphaF(0.65);
        }
    }

    if (m_type == Trash && m_hovered) {
        iconColor = QColor("#ff5555");
    }

    int size = qMin(width(), height());
    QRect rect = this->rect();

    if (m_type == Settings) {
        QPen pen(iconColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(rect.center(), size / 5, size / 5);
        for (int i = 0; i < 8; ++i) {
            double angle = i * 45.0 * 3.14159265 / 180.0;
            QPointF p1 = rect.center() + QPointF(cos(angle) * (size/5.0), sin(angle) * (size/5.0));
            QPointF p2 = rect.center() + QPointF(cos(angle) * (size/3.3), sin(angle) * (size/3.3));
            painter.drawLine(p1, p2);
        }
    } else if (m_type == Analytics) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(iconColor);
        int barW = size / 7;
        int gap = size / 12;
        int h1 = size / 3.5;
        int h2 = size / 2.2;
        int h3 = (2 * size) / 3.2;
        int xStart = rect.center().x() - (3 * barW + 2 * gap) / 2;
        int yBase = rect.center().y() + size/3.2;
        painter.drawRoundedRect(xStart, yBase - h1, barW, h1, 1.5, 1.5);
        painter.drawRoundedRect(xStart + barW + gap, yBase - h2, barW, h2, 1.5, 1.5);
        painter.drawRoundedRect(xStart + 2 * (barW + gap), yBase - h3, barW, h3, 1.5, 1.5);
    } else if (m_type == Play) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(iconColor);
        QPainterPath path;
        path.moveTo(rect.center().x() - size/8, rect.center().y() - size/4);
        path.lineTo(rect.center().x() + size/3.5, rect.center().y());
        path.lineTo(rect.center().x() - size/8, rect.center().y() + size/4);
        path.closeSubpath();
        painter.drawPath(path);
    } else if (m_type == Pause) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(iconColor);
        int barW = size / 8;
        int barH = size / 2.2;
        painter.drawRoundedRect(rect.center().x() - barW - barW/2, rect.center().y() - barH/2, barW, barH, 1.5, 1.5);
        painter.drawRoundedRect(rect.center().x() + barW/2, rect.center().y() - barH/2, barW, barH, 1.5, 1.5);
    } else if (m_type == Trash) {
        QPen pen(iconColor, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(rect.center().x() - size/3.2, rect.center().y() - size/4, rect.center().x() + size/3.2, rect.center().y() - size/4);
        painter.drawLine(rect.center().x() - size/7, rect.center().y() - size/4, rect.center().x() - size/7, rect.center().y() - size/3);
        painter.drawLine(rect.center().x() - size/7, rect.center().y() - size/3, rect.center().x() + size/7, rect.center().y() - size/3);
        painter.drawLine(rect.center().x() + size/7, rect.center().y() - size/3, rect.center().x() + size/7, rect.center().y() - size/4);
        painter.drawRect(rect.center().x() - size/4.5, rect.center().y() - size/4 + 2, size/2.25, size/2);
        painter.drawLine(rect.center().x() - size/10, rect.center().y() - size/10, rect.center().x() - size/10, rect.center().y() + size/7);
        painter.drawLine(rect.center().x() + size/10, rect.center().y() - size/10, rect.center().x() + size/10, rect.center().y() + size/7);
    } else if (m_type == Close) {
        QPen pen(iconColor, 2, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        int d = size / 4.5;
        painter.drawLine(rect.center().x() - d, rect.center().y() - d, rect.center().x() + d, rect.center().y() + d);
        painter.drawLine(rect.center().x() + d, rect.center().y() - d, rect.center().x() - d, rect.center().y() + d);
    } else if (m_type == Plus) {
        QPen pen(iconColor, 2.2, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        int d = size / 4.5;
        painter.drawLine(rect.center().x() - d, rect.center().y(), rect.center().x() + d, rect.center().y());
        painter.drawLine(rect.center().x(), rect.center().y() - d, rect.center().x(), rect.center().y() + d);
    } else if (m_type == Minus) {
        QPen pen(iconColor, 2.2, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        int d = size / 4.5;
        painter.drawLine(rect.center().x() - d, rect.center().y(), rect.center().x() + d, rect.center().y());
    } else if (m_type == Check) {
        QPen pen(iconColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawLine(rect.center().x() - size/4, rect.center().y(), rect.center().x() - size/15, rect.center().y() + size/5);
        painter.drawLine(rect.center().x() - size/15, rect.center().y() + size/5, rect.center().x() + size/3.2, rect.center().y() - size/4.5);
    } else if (m_type == Add5 || m_type == Sub5) {
        painter.setPen(iconColor);
        painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter.drawText(rect, Qt::AlignCenter, m_type == Add5 ? "+5m" : "-5m");
    } else if (m_type == Minimize) {
        QPen pen(iconColor, 2.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        int d = size / 4.5;
        painter.drawLine(rect.center().x() - d, rect.center().y() + d/2, rect.center().x() + d, rect.center().y() + d/2);
    }
}

// --- CustomCheckBox ---
CustomCheckBox::CustomCheckBox(const QString &themeName, QWidget *parent)
    : QAbstractButton(parent), m_themeName(themeName) {
    setCheckable(true);
    setFixedSize(22, 22);
    setCursor(Qt::PointingHandCursor);
}

void CustomCheckBox::setTheme(const QString &themeName) {
    m_themeName = themeName;
    update();
}

void CustomCheckBox::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void CustomCheckBox::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

void CustomCheckBox::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);
    QRect r = rect().adjusted(1, 1, -1, -1);
    
    QColor borderCol = isChecked() ? QColor(colors.accent) : QColor(colors.border);
    QColor bgCol = isChecked() ? QColor(colors.accent) : Qt::transparent;
    
    if (m_hovered) {
        borderCol = QColor(colors.accent);
    }

    painter.setPen(QPen(borderCol, 2));
    painter.setBrush(bgCol);
    painter.drawRoundedRect(r, 6, 6);

    if (isChecked()) {
        QColor tickColor = QColor(colors.bg);
        if (m_themeName == "sakura") tickColor = QColor("#2d1b24");
        painter.setPen(QPen(tickColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(r.left() + 5, r.center().y() + 1, r.center().x() - 1, r.bottom() - 5);
        painter.drawLine(r.center().x() - 1, r.bottom() - 5, r.right() - 5, r.top() + 5);
    }
}

// --- TaskItemWidget ---
TaskItemWidget::TaskItemWidget(int index, const QString &text, bool completed, const QString &themeName, QWidget *parent)
    : QWidget(parent), m_index(index), m_text(text), m_themeName(themeName) {
    
    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(2, 2, 2, 2);
    outerLayout->setSpacing(0);

    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("TaskCard");
    outerLayout->addWidget(m_cardFrame);

    QHBoxLayout *layout = new QHBoxLayout(m_cardFrame);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    m_chkDone = new CustomCheckBox(m_themeName, m_cardFrame);
    m_chkDone->setChecked(completed);
    connect(m_chkDone, &QAbstractButton::toggled, this, &TaskItemWidget::onCheckBoxToggled);
    layout->addWidget(m_chkDone);

    m_lblText = new EditableLabel(m_text, m_cardFrame);
    m_lblText->setFont(QFont("Segoe UI", 9.5));
    m_lblText->setWordWrap(true);
    connect(m_lblText, &EditableLabel::doubleClicked, this, &TaskItemWidget::triggerRename);
    layout->addWidget(m_lblText, 1);

    m_lineEdit = new QLineEdit(m_text, m_cardFrame);
    m_lineEdit->setVisible(false);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &TaskItemWidget::saveRename);
    layout->addWidget(m_lineEdit, 1);

    m_btnPlay = new IconButton(IconButton::Play, m_themeName, m_cardFrame);
    m_btnPlay->setVisible(isTimeBasedTask(m_text));
    connect(m_btnPlay, &QPushButton::clicked, this, [this]() { emit startTimerRequested(m_index); });
    layout->addWidget(m_btnPlay);

    m_btnDelete = new IconButton(IconButton::Trash, m_themeName, m_cardFrame);
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() { emit taskDeleted(m_index); });
    layout->addWidget(m_btnDelete);

    applyTheme(m_themeName);
}

void TaskItemWidget::onCheckBoxToggled(bool checked) {
    emit taskToggled(m_index, checked);
}

void TaskItemWidget::triggerRename() {
    m_lblText->setVisible(false);
    m_lineEdit->setText(m_text);
    m_lineEdit->setVisible(true);
    m_lineEdit->setFocus();
    m_lineEdit->selectAll();
}

void TaskItemWidget::saveRename() {
    QString newText = m_lineEdit->text().trimmed();
    if (!newText.isEmpty()) {
        m_text = newText;
        m_lblText->setText(newText);
        m_btnPlay->setVisible(isTimeBasedTask(newText));
        emit taskEdited(m_index, newText);
    }
    m_lineEdit->setVisible(false);
    m_lblText->setVisible(true);
}

void TaskItemWidget::applyTheme(const QString &themeName) {
    m_themeName = themeName;
    m_chkDone->setTheme(themeName);
    m_btnPlay->setTheme(themeName);
    m_btnDelete->setTheme(themeName);
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    setStyleSheet(QString(
        "QFrame#TaskCard {"
        "  background: %1;"
        "  border: 1px solid rgba(255, 255, 255, 0.04);"
        "  border-radius: 10px;"
        "}"
        "QFrame#TaskCard:hover {"
        "  border: 1px solid %2;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QLineEdit {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
    ).arg(colors.card, colors.accent, colors.fg, colors.bg));
}

// --- FocusTimerWidget ---
FocusTimerWidget::FocusTimerWidget(const QString &taskName, int durationSeconds, int alertIntervalMins, const QString &themeName, QWidget *parent)
    : QWidget(parent), m_taskName(taskName), m_duration(durationSeconds), m_timeLeft(durationSeconds),
      m_alertInterval(alertIntervalMins), m_themeName(themeName) {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::SubWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(220, 290);

    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("TimerFrame");

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
    innerLayout->setContentsMargins(14, 14, 14, 14);
    innerLayout->setSpacing(10);

    m_lblTask = new QLabel(m_taskName, m_outerFrame);
    m_lblTask->setFont(QFont("Segoe UI", 10, QFont::Bold));
    m_lblTask->setAlignment(Qt::AlignCenter);
    m_lblTask->setWordWrap(true);
    innerLayout->addWidget(m_lblTask);

    m_circleProgress = new QWidget(m_outerFrame);
    m_circleProgress->setFixedSize(130, 130);
    innerLayout->addWidget(m_circleProgress, 0, Qt::AlignCenter);

    QHBoxLayout *adjustLay = new QHBoxLayout();
    m_btnSub5 = new IconButton(IconButton::Sub5, m_themeName, m_outerFrame);
    connect(m_btnSub5, &QPushButton::clicked, this, [this]() { adjustTime(-300); });

    m_btnAdd5 = new IconButton(IconButton::Add5, m_themeName, m_outerFrame);
    connect(m_btnAdd5, &QPushButton::clicked, this, [this]() { adjustTime(300); });

    adjustLay->addWidget(m_btnSub5);
    adjustLay->addWidget(m_btnAdd5);
    innerLayout->addLayout(adjustLay);

    QHBoxLayout *ctrlLay = new QHBoxLayout();
    m_btnPause = new QPushButton("Pause", m_outerFrame);
    m_btnPause->setCursor(Qt::PointingHandCursor);
    connect(m_btnPause, &QPushButton::clicked, this, &FocusTimerWidget::togglePause);

    m_btnCancel = new QPushButton("Cancel", m_outerFrame);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    connect(m_btnCancel, &QPushButton::clicked, this, &QWidget::close);

    ctrlLay->addWidget(m_btnPause);
    ctrlLay->addWidget(m_btnCancel);
    innerLayout->addLayout(ctrlLay);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FocusTimerWidget::onTick);
    m_timer->start(1000);

    applyThemeStyles();
}

void FocusTimerWidget::setTheme(const QString &themeName) {
    m_themeName = themeName;
    m_btnSub5->setTheme(themeName);
    m_btnAdd5->setTheme(themeName);
    applyThemeStyles();
}

void FocusTimerWidget::adjustTime(int deltaSecs) {
    m_timeLeft = qMax(0, m_timeLeft + deltaSecs);
    m_duration = qMax(1, m_duration + deltaSecs);
    m_circleProgress->update();
}

void FocusTimerWidget::togglePause() {
    m_isPaused = !m_isPaused;
    m_btnPause->setText(m_isPaused ? "Resume" : "Pause");
}

void FocusTimerWidget::onTick() {
    if (m_isPaused) return;

    m_timeLeft--;
    m_circleProgress->update();

    int elapsed = m_duration - m_timeLeft;
    int alertSecs = m_alertInterval * 60;
    if (m_timeLeft > 0 && elapsed % alertSecs == 0) {
        playWavSound("alert.wav");
    }

    if (m_timeLeft <= 0) {
        m_timer->stop();
        emit timerFinished();
    }
}

void FocusTimerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPoint circlePos = m_circleProgress->mapTo(this, QPoint(0, 0));
    QRectF rect(circlePos.x() + 6, circlePos.y() + 6, 118, 118);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    // Inner Glass circle
    QColor innerBg = QColor(colors.bg);
    innerBg.setAlphaF(0.3);
    painter.setBrush(innerBg);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect.adjusted(2, 2, -2, -2));

    // Track Background
    QPen trackPen(QColor(255, 255, 255, 12), 8);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect);

    // Active segments
    QPen progressPen(QColor(colors.accent), 8, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(progressPen);
    double fraction = static_cast<double>(m_timeLeft) / m_duration;
    int spanAngle = static_cast<int>(fraction * 360 * 16);
    painter.drawArc(rect, 90 * 16, spanAngle);

    // Centered countdown Text
    int mins = m_timeLeft / 60;
    int secs = m_timeLeft % 60;
    QString timeText = QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));

    painter.setPen(QColor(colors.fg));
    painter.setFont(QFont("Segoe UI", 18, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, timeText);
}

void FocusTimerWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void FocusTimerWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void FocusTimerWidget::applyThemeStyles() {
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    m_outerFrame->setStyleSheet(QString(
        "QFrame#TimerFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 6px 14px;"
        "  color: %3;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
    ).arg(colors.bg, colors.border, colors.fg, colors.card));
}

// --- OnboardingDialog ---
OnboardingDialog::OnboardingDialog(Storage::AppState &state, Storage::TasksConfig &tasksDict, const QString &themeName, QWidget *parent)
    : QDialog(parent), m_state(state), m_tasksDict(tasksDict), m_themeName(themeName) {
    initUI();
}

void OnboardingDialog::initUI() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(360, 430);

    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("OnboardFrame");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setXOffset(0);
    shadow->setYOffset(5);
    shadow->setColor(QColor(0, 0, 0, 90));
    m_outerFrame->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_outerFrame);

    QVBoxLayout *innerLayout = new QVBoxLayout(m_outerFrame);
    innerLayout->setContentsMargins(20, 20, 20, 20);
    innerLayout->setSpacing(10);

    m_stack = new QStackedWidget(m_outerFrame);
    m_stack->addWidget(createSlide1());
    m_stack->addWidget(createSlide2());
    m_stack->addWidget(createSlide3TaskBuilder());
    m_stack->addWidget(createSlide4Guide());
    m_stack->addWidget(createSlide5AllSet());
    innerLayout->addWidget(m_stack);

    QHBoxLayout *btnLay = new QHBoxLayout();
    m_btnPrev = new QPushButton("Back", m_outerFrame);
    m_btnPrev->setCursor(Qt::PointingHandCursor);
    m_btnPrev->setEnabled(false);
    connect(m_btnPrev, &QPushButton::clicked, this, &OnboardingDialog::prevPage);

    m_btnNext = new QPushButton("Next", m_outerFrame);
    m_btnNext->setCursor(Qt::PointingHandCursor);
    connect(m_btnNext, &QPushButton::clicked, this, &OnboardingDialog::nextPage);

    btnLay->addWidget(m_btnPrev);
    btnLay->addWidget(m_btnNext);
    innerLayout->addLayout(btnLay);

    applyThemeStyles();
}

QWidget* OnboardingDialog::createSlide1() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(12);

    QLabel *title = new QLabel("WELCOME TO ROUTINE", w);
    title->setFont(QFont("Segoe UI", 12, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);

    QLabel *body = new QLabel("This is not a productivity tracker. It is a visual accountability companion designed for ADHD focus.\n\nEverything runs entirely locally. Your streaks, focus logs, and daily lists are private.", w);
    body->setFont(QFont("Segoe UI", 9.5));
    body->setWordWrap(true);
    body->setAlignment(Qt::AlignCenter);

    lay->addWidget(title);
    lay->addWidget(body);
    lay->addStretch();
    return w;
}

QWidget* OnboardingDialog::createSlide2() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    QLabel *title = new QLabel("PERSONALIZATION", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    QHBoxLayout *nameLay = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("Name:", w);
    lbl1->setMinimumWidth(80);
    m_txtUserName = new QLineEdit(m_state.userName, w);
    m_txtUserName->setPlaceholderText("Friend");
    nameLay->addWidget(lbl1);
    nameLay->addWidget(m_txtUserName, 1);
    lay->addLayout(nameLay);

    QHBoxLayout *ageLay = new QHBoxLayout();
    QLabel *lblAge = new QLabel("Age:", w);
    lblAge->setMinimumWidth(80);
    m_spnAge = new QSpinBox(w);
    m_spnAge->setRange(1, 120);
    m_spnAge->setValue(m_state.userAge);
    m_spnAge->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spnAge->setAlignment(Qt::AlignCenter);
    
    // Custom age buttons right after the spinbox
    IconButton *btnDecAge = new IconButton(IconButton::Minus, m_themeName, w);
    IconButton *btnIncAge = new IconButton(IconButton::Plus, m_themeName, w);
    connect(btnDecAge, &QPushButton::clicked, this, [this]() { m_spnAge->setValue(m_spnAge->value() - 1); });
    connect(btnIncAge, &QPushButton::clicked, this, [this]() { m_spnAge->setValue(m_spnAge->value() + 1); });

    ageLay->addWidget(lblAge);
    ageLay->addWidget(m_spnAge, 1);
    ageLay->addWidget(btnDecAge);
    ageLay->addWidget(btnIncAge);
    lay->addLayout(ageLay);

    QHBoxLayout *goalLay = new QHBoxLayout();
    QLabel *lblGoal = new QLabel("Main Focus:", w);
    lblGoal->setMinimumWidth(80);
    m_txtFocusGoal = new QLineEdit(m_state.focusGoal, w);
    m_txtFocusGoal->setPlaceholderText("e.g. Coding, Studies, ADHD Focus");
    goalLay->addWidget(lblGoal);
    goalLay->addWidget(m_txtFocusGoal, 1);
    lay->addLayout(goalLay);

    QHBoxLayout *themeLay = new QHBoxLayout();
    QLabel *lbl2 = new QLabel("Theme Preset:", w);
    lbl2->setMinimumWidth(80);
    m_cmbTheme = new QComboBox(w);
    m_cmbTheme->addItems(THEMES.keys());
    m_cmbTheme->setCurrentText(m_themeName);
    connect(m_cmbTheme, &QComboBox::currentTextChanged, this, &OnboardingDialog::onThemeSelected);
    themeLay->addWidget(lbl2);
    themeLay->addWidget(m_cmbTheme, 1);
    lay->addLayout(themeLay);

    lay->addStretch();
    return w;
}

QWidget* OnboardingDialog::createSlide3TaskBuilder() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(8);

    QLabel *title = new QLabel("DAILY ROUTINE BUILDER", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    QLabel *sub = new QLabel("Add tasks you want to do every day (double-click to remove):", w);
    sub->setFont(QFont("Segoe UI", 8.5));
    sub->setWordWrap(true);
    lay->addWidget(sub);

    QHBoxLayout *inputLay = new QHBoxLayout();
    m_txtTaskInput = new QLineEdit(w);
    m_txtTaskInput->setPlaceholderText("e.g. Coding (2h) or Walk (30m)");
    m_btnAddTask = new IconButton(IconButton::Plus, m_themeName, w);
    connect(m_btnAddTask, &QPushButton::clicked, this, &OnboardingDialog::addOnboardTask);
    connect(m_txtTaskInput, &QLineEdit::returnPressed, this, &OnboardingDialog::addOnboardTask);
    inputLay->addWidget(m_txtTaskInput, 1);
    inputLay->addWidget(m_btnAddTask);
    lay->addLayout(inputLay);

    m_lstTasks = new QListWidget(w);
    connect(m_lstTasks, &QListWidget::itemDoubleClicked, this, &OnboardingDialog::removeOnboardTask);
    lay->addWidget(m_lstTasks, 1);

    QLabel *chipsLbl = new QLabel("Quick Suggestion Templates:", w);
    chipsLbl->setFont(QFont("Segoe UI", 8, QFont::Bold));
    lay->addWidget(chipsLbl);

    QHBoxLayout *chipsLay = new QHBoxLayout();
    QStringList suggestions = {"Coding (2h)", "Exercise (30m)", "Reading (30m)", "Study (1h)"};
    for (const auto &s : suggestions) {
        QPushButton *chip = new QPushButton(s, w);
        chip->setCursor(Qt::PointingHandCursor);
        chip->setStyleSheet(
            "QPushButton {"
            "  background: rgba(255, 255, 255, 0.05);"
            "  border: 1px solid rgba(255, 255, 255, 0.1);"
            "  border-radius: 10px;"
            "  padding: 3px 8px;"
            "  font-size: 10px;"
            "  color: #cdd6f4;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(255, 255, 255, 0.12);"
            "}"
        );
        connect(chip, &QPushButton::clicked, this, [this, s]() {
            m_lstTasks->addItem(s);
        });
        chipsLay->addWidget(chip);
    }
    lay->addLayout(chipsLay);

    return w;
}

void OnboardingDialog::addOnboardTask() {
    QString task = m_txtTaskInput->text().trimmed();
    if (!task.isEmpty()) {
        m_lstTasks->addItem(task);
        m_txtTaskInput->clear();
    }
}

void OnboardingDialog::removeOnboardTask() {
    QListWidgetItem *item = m_lstTasks->currentItem();
    if (item) {
        delete item;
    }
}

QWidget* OnboardingDialog::createSlide4Guide() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    QLabel *title = new QLabel("PLATFORM GUIDE", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);

    QLabel *body = new QLabel("- Drag Widget: Grab header and slide anywhere.\n- Start Session: Click play next to tasks.\n- Rename: Double-click task text directly.\n- Check-in Chirps: Plays alert beeps periodically to redirect floating focus.", w);
    body->setFont(QFont("Segoe UI", 9));
    body->setWordWrap(true);

    lay->addWidget(title);
    lay->addWidget(body);
    lay->addStretch();
    return w;
}

QWidget* OnboardingDialog::createSlide5AllSet() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(12);

    QLabel *title = new QLabel("YOU'RE ALL SET!", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);

    QLabel *body = new QLabel("Consistency builds momentum one small step at a time.\n\nLet's build a streak starting today!", w);
    body->setFont(QFont("Segoe UI", 10));
    body->setWordWrap(true);
    body->setAlignment(Qt::AlignCenter);

    lay->addWidget(title);
    lay->addWidget(body);
    lay->addStretch();
    return w;
}

void OnboardingDialog::onThemeSelected(const QString &theme) {
    m_themeName = theme;
    m_btnAddTask->setTheme(theme);
    applyThemeStyles();
    emit themeChanged(theme);
}

void OnboardingDialog::nextPage() {
    if (m_currentSlide < 4) {
        m_currentSlide++;
        m_stack->setCurrentIndex(m_currentSlide);
        m_btnPrev->setEnabled(true);
        if (m_currentSlide == 4) {
            m_btnNext->setText("Get Started");
        }
    } else {
        m_state.userName = m_txtUserName->text().trimmed().isEmpty() ? "Friend" : m_txtUserName->text().trimmed();
        m_state.themeSelection = m_cmbTheme->currentText();
        m_state.userAge = m_spnAge->value();
        m_state.focusGoal = m_txtFocusGoal->text().trimmed().isEmpty() ? "Productivity" : m_txtFocusGoal->text().trimmed();
        m_state.onboarded = true;

        m_tasksDict.selectedProfile = "Daily Habits";
        m_tasksDict.profiles.clear();

        QList<QString> tasks;
        for (int i = 0; i < m_lstTasks->count(); ++i) {
            tasks.append(m_lstTasks->item(i)->text());
        }
        if (tasks.isEmpty()) {
            tasks = {"Exercise (1h)", "Study (1h)", "Coding (2h)"};
        }
        m_tasksDict.profiles["Daily Habits"] = tasks;
        m_tasksDict.profiles["Work Focus"] = {
            "Email triaging (15m)",
            "Deep Work (2h)",
            "System Review (30m)"
        };

        Storage::saveTasks(m_tasksDict);

        m_state.profileStates.clear();
        for (const auto &prof : m_tasksDict.profiles.keys()) {
            m_state.profileStates[prof] = QList<bool>(m_tasksDict.profiles[prof].size(), false);
        }
        m_state.completed = m_state.profileStates["Daily Habits"];
        
        Storage::saveState(m_state);
        accept();
    }
}

void OnboardingDialog::prevPage() {
    if (m_currentSlide > 0) {
        m_currentSlide--;
        m_stack->setCurrentIndex(m_currentSlide);
        m_btnNext->setText("Next");
        if (m_currentSlide == 0) {
            m_btnPrev->setEnabled(false);
        }
    }
}

void OnboardingDialog::applyThemeStyles() {
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    m_outerFrame->setStyleSheet(QString(
        "QFrame#OnboardFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QLineEdit {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "  color: %3;"
        "}"
        "QComboBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 4px 8px;"
        "  color: %3;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %4;"
        "  border: 1px solid %2;"
        "  color: %3;"
        "  selection-background-color: %2;"
        "}"
        "QSpinBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
        "QListWidget {"
        "  background: rgba(0, 0, 0, 0.2);"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 8px 14px;"
        "  color: %3;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
    ).arg(colors.bg, colors.border, colors.fg, colors.card));
}

// --- SettingsDialog ---
SettingsDialog::SettingsDialog(Storage::AppState &state, Storage::TasksConfig &tasksDict, const QString &themeName, QWidget *parent)
    : QDialog(parent), m_state(state), m_tasksDict(tasksDict), m_themeName(themeName) {
    initUI();
}

void SettingsDialog::initUI() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(340, 620);

    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("SettingsFrame");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 80));
    m_outerFrame->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_outerFrame);

    QVBoxLayout *innerLayout = new QVBoxLayout(m_outerFrame);
    innerLayout->setContentsMargins(20, 20, 20, 20);
    innerLayout->setSpacing(10);

    QLabel *title = new QLabel("SETTINGS", m_outerFrame);
    title->setFont(QFont("Segoe UI", 12, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(title);

    // Username
    QHBoxLayout *nameLay = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("User Name:", m_outerFrame);
    lbl1->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_txtUserName = new QLineEdit(m_state.userName, m_outerFrame);
    nameLay->addWidget(lbl1);
    nameLay->addWidget(m_txtUserName, 1);
    innerLayout->addLayout(nameLay);

    // Theme preset
    QHBoxLayout *themeLay = new QHBoxLayout();
    QLabel *lbl2 = new QLabel("Theme Preset:", m_outerFrame);
    lbl2->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_cmbTheme = new QComboBox(m_outerFrame);
    m_cmbTheme->addItems(THEMES.keys());
    m_cmbTheme->setCurrentText(m_state.themeSelection);
    themeLay->addWidget(lbl2);
    themeLay->addWidget(m_cmbTheme, 1);
    innerLayout->addLayout(themeLay);

    // Checkboxes
    QHBoxLayout *chkLay = new QHBoxLayout();
    m_chkPinned = new QCheckBox("Pinned", m_outerFrame);
    m_chkPinned->setChecked(m_state.pinned);
    
    m_chkStartup = new QCheckBox("Start on Boot", m_outerFrame);
    m_chkStartup->setChecked(Logic::isStartupEnabled());
    
    chkLay->addWidget(m_chkPinned);
    chkLay->addWidget(m_chkStartup);
    innerLayout->addLayout(chkLay);

    // Sound selection
    QHBoxLayout *soundLay = new QHBoxLayout();
    QLabel *lbl3 = new QLabel("Sound Profile:", m_outerFrame);
    lbl3->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_cmbSound = new QComboBox(m_outerFrame);
    m_cmbSound->addItems({"pop", "sparkle", "chime", "silent"});
    m_cmbSound->setCurrentText(m_state.soundSelection);
    soundLay->addWidget(lbl3);
    soundLay->addWidget(m_cmbSound, 1);
    innerLayout->addLayout(soundLay);

    // Alert spinbox
    QHBoxLayout *alertLay = new QHBoxLayout();
    QLabel *lbl4 = new QLabel("Focus Beep Reminder:", m_outerFrame);
    lbl4->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_spnAlert = new QSpinBox(m_outerFrame);
    m_spnAlert->setRange(1, 120);
    m_spnAlert->setSuffix(" min");
    m_spnAlert->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spnAlert->setValue(m_state.customAlertMinutes);
    m_spnAlert->setAlignment(Qt::AlignCenter);
    m_spnAlert->setFixedWidth(60);

    IconButton *btnDec = new IconButton(IconButton::Minus, m_themeName, m_outerFrame);
    IconButton *btnInc = new IconButton(IconButton::Plus, m_themeName, m_outerFrame);
    connect(btnDec, &QPushButton::clicked, this, [this]() { m_spnAlert->setValue(m_spnAlert->value() - 1); });
    connect(btnInc, &QPushButton::clicked, this, [this]() { m_spnAlert->setValue(m_spnAlert->value() + 1); });

    alertLay->addWidget(lbl4);
    alertLay->addStretch();
    alertLay->addWidget(m_spnAlert);
    alertLay->addWidget(btnDec);
    alertLay->addWidget(btnInc);
    innerLayout->addLayout(alertLay);

    // Separator line
    QFrame *line = new QFrame(m_outerFrame);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    innerLayout->addWidget(line);

    // Profile Management Section
    QLabel *profTitle = new QLabel("MANAGE PROFILES", m_outerFrame);
    profTitle->setFont(QFont("Segoe UI", 10, QFont::Bold));
    profTitle->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(profTitle);

    QHBoxLayout *profLay = new QHBoxLayout();
    m_txtProfInput = new QLineEdit(m_outerFrame);
    m_txtProfInput->setPlaceholderText("Create new profile name");
    m_btnCreateProf = new QPushButton("Create", m_outerFrame);
    m_btnCreateProf->setCursor(Qt::PointingHandCursor);
    connect(m_btnCreateProf, &QPushButton::clicked, this, &SettingsDialog::createNewProfile);
    profLay->addWidget(m_txtProfInput, 1);
    profLay->addWidget(m_btnCreateProf);
    innerLayout->addLayout(profLay);

    QHBoxLayout *profDelLay = new QHBoxLayout();
    m_cmbDelProfile = new QComboBox(m_outerFrame);
    for (const auto &p : m_tasksDict.profiles.keys()) {
        if (p != m_tasksDict.selectedProfile) m_cmbDelProfile->addItem(p);
    }
    m_btnDeleteProf = new QPushButton("Delete", m_outerFrame);
    m_btnDeleteProf->setCursor(Qt::PointingHandCursor);
    connect(m_btnDeleteProf, &QPushButton::clicked, this, &SettingsDialog::deleteSelectedProfile);
    profDelLay->addWidget(m_cmbDelProfile, 1);
    profDelLay->addWidget(m_btnDeleteProf);
    innerLayout->addLayout(profDelLay);

    // Add Task to selected profile
    QLabel *taskLbl = new QLabel("Add task to selected profile:", m_outerFrame);
    taskLbl->setFont(QFont("Segoe UI", 9, QFont::Bold));
    innerLayout->addWidget(taskLbl);

    QHBoxLayout *taskLay = new QHBoxLayout();
    m_txtTaskInput = new QLineEdit(m_outerFrame);
    m_txtTaskInput->setPlaceholderText("e.g. Coding (2h)");
    m_btnAddTask = new QPushButton("Add Task", m_outerFrame);
    m_btnAddTask->setCursor(Qt::PointingHandCursor);
    connect(m_btnAddTask, &QPushButton::clicked, this, &SettingsDialog::addCustomTask);
    taskLay->addWidget(m_txtTaskInput, 1);
    taskLay->addWidget(m_btnAddTask);
    innerLayout->addLayout(taskLay);

    innerLayout->addStretch();

    // Footer actions
    QHBoxLayout *btnLay = new QHBoxLayout();
    m_btnCancel = new QPushButton("Cancel", m_outerFrame);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton("Save", m_outerFrame);
    m_btnSave->setCursor(Qt::PointingHandCursor);
    m_btnSave->setObjectName("SaveButton");
    connect(m_btnSave, &QPushButton::clicked, this, &SettingsDialog::saveSettings);

    btnLay->addWidget(m_btnCancel);
    btnLay->addWidget(m_btnSave);
    innerLayout->addLayout(btnLay);

    applyThemeStyles();
}

void SettingsDialog::createNewProfile() {
    QString name = m_txtProfInput->text().trimmed();
    if (!name.isEmpty() && !m_tasksDict.profiles.contains(name)) {
        m_tasksDict.profiles[name] = QList<QString>();
        m_state.profileStates[name] = QList<bool>();
        m_txtProfInput->clear();
        m_txtProfInput->setPlaceholderText("Created!");

        m_cmbDelProfile->clear();
        for (const auto &p : m_tasksDict.profiles.keys()) {
            if (p != m_tasksDict.selectedProfile) m_cmbDelProfile->addItem(p);
        }
    }
}

void SettingsDialog::deleteSelectedProfile() {
    QString toDel = m_cmbDelProfile->currentText();
    if (!toDel.isEmpty() && m_tasksDict.profiles.contains(toDel)) {
        m_tasksDict.profiles.remove(toDel);
        m_state.profileStates.remove(toDel);

        Storage::saveTasks(m_tasksDict);
        Storage::saveState(m_state);

        m_cmbDelProfile->clear();
        for (const auto &p : m_tasksDict.profiles.keys()) {
            if (p != m_tasksDict.selectedProfile) m_cmbDelProfile->addItem(p);
        }
    }
}

void SettingsDialog::addCustomTask() {
    QString task = m_txtTaskInput->text().trimmed();
    if (!task.isEmpty()) {
        QString active = m_tasksDict.selectedProfile;
        m_tasksDict.profiles[active].append(task);
        m_state.profileStates[active].append(false);
        m_state.completed = m_state.profileStates[active];

        m_txtTaskInput->clear();
        m_txtTaskInput->setPlaceholderText("Added!");
    }
}

void SettingsDialog::saveSettings() {
    m_state.themeSelection = m_cmbTheme->currentText();
    m_state.pinned = m_chkPinned->isChecked();
    m_state.soundSelection = m_cmbSound->currentText();
    m_state.customAlertMinutes = m_spnAlert->value();
    m_state.userName = m_txtUserName->text().trimmed().isEmpty() ? "Friend" : m_txtUserName->text().trimmed();

    Logic::setStartup(m_chkStartup->isChecked());

    emit settingsSaved();
    accept();
}

void SettingsDialog::applyThemeStyles() {
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    QString comboStyle = QString(
        "QComboBox {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 4px 8px;"
        "  color: %3;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  selection-background-color: %2;"
        "  color: %3;"
        "}"
    ).arg(colors.card, colors.border, colors.fg);

    m_cmbTheme->setStyleSheet(comboStyle);
    m_cmbSound->setStyleSheet(comboStyle);
    m_cmbDelProfile->setStyleSheet(comboStyle);

    m_outerFrame->setStyleSheet(QString(
        "QFrame#SettingsFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QCheckBox {"
        "  color: %3;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 5px;"
        "  border: 1px solid %2;"
        "  background: %4;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %5;"
        "  border-color: %5;"
        "}"
        "QLineEdit {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "  color: %3;"
        "  font-size: 12px;"
        "}"
        "QSpinBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  color: %3;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
        "QPushButton#SaveButton {"
        "  background: %5;"
        "  color: #11111b;"
        "  border-color: %5;"
        "}"
    ).arg(colors.bg, colors.border, colors.fg, colors.card, colors.accent));
}

// --- MainWindow ---
MainWindow::MainWindow(Storage::AppState &state, Storage::TasksConfig &tasksDict, QWidget *parent)
    : QWidget(parent), m_state(state), m_tasksDict(tasksDict) {

    m_selectedProfile = m_tasksDict.selectedProfile;
    m_themeName = m_state.themeSelection;

    initUI();
    updateProgress(false);
}

MainWindow::~MainWindow() {
    if (m_activeTimer) {
        m_activeTimer->close();
    }
}

void MainWindow::initUI() {
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(":/icon.png"));
    setMinimumSize(280, 450);
    setMaximumSize(600, 900);
    resize(300, 520);

    RECT desktopRect;
    GetWindowRect(GetDesktopWindow(), &desktopRect);
    int scrW = desktopRect.right - desktopRect.left;
    int scrH = desktopRect.bottom - desktopRect.top;
    
    int x = m_state.windowX;
    int y = m_state.windowY;
    if (x < 0 || x > scrW - 100) x = 200;
    if (y < 0 || y > scrH - 100) y = 200;
    move(x, y);

    setPinnedMode(m_state.pinned);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 10, 10, 10);

    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("MainFrame");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setXOffset(0);
    shadow->setYOffset(6);
    shadow->setColor(QColor(0, 0, 0, 95));
    m_outerFrame->setGraphicsEffect(shadow);
    rootLayout->addWidget(m_outerFrame);

    QVBoxLayout *innerLayout = new QVBoxLayout(m_outerFrame);
    innerLayout->setContentsMargins(16, 16, 16, 16);
    innerLayout->setSpacing(10);

    // Header segment
    QHBoxLayout *header = new QHBoxLayout();
    m_lblGreeting = new QLabel(QString("👋 Hi %1!").arg(m_state.userName), m_outerFrame);
    m_lblGreeting->setFont(QFont("Segoe UI", 11, QFont::Bold));
    header->addWidget(m_lblGreeting, 1);

    m_btnSettings = new IconButton(IconButton::Settings, m_themeName, m_outerFrame);
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::openSettings);

    m_btnAnalytics = new IconButton(IconButton::Analytics, m_themeName, m_outerFrame);
    connect(m_btnAnalytics, &QPushButton::clicked, this, &MainWindow::openAnalytics);

    m_btnMinimize = new IconButton(IconButton::Minimize, m_themeName, m_outerFrame);
    connect(m_btnMinimize, &QPushButton::clicked, this, &QWidget::hide);

    m_btnClose = new IconButton(IconButton::Close, m_themeName, m_outerFrame);
    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        saveWidgetState();
        QCoreApplication::quit();
    });

    header->addWidget(m_btnAnalytics);
    header->addWidget(m_btnSettings);
    header->addWidget(m_btnMinimize);
    header->addWidget(m_btnClose);
    innerLayout->addLayout(header);

    m_cmbProfile = new QComboBox(m_outerFrame);
    m_cmbProfile->addItems(m_tasksDict.profiles.keys());
    m_cmbProfile->setCurrentText(m_selectedProfile);
    connect(m_cmbProfile, &QComboBox::currentTextChanged, this, &MainWindow::onProfileSwitched);
    innerLayout->addWidget(m_cmbProfile);

    m_listWidget = new QListWidget(m_outerFrame);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setSpacing(6);
    m_listWidget->setStyleSheet("background: transparent;");
    innerLayout->addWidget(m_listWidget, 1);

    m_progressBarContainer = new QWidget(m_outerFrame);
    m_progressBarContainer->setFixedHeight(8);
    m_progressBarContainer->setObjectName("ProgressContainer");
    
    m_progressBarFill = new QWidget(m_progressBarContainer);
    m_progressBarFill->setFixedHeight(8);
    m_progressBarFill->setObjectName("ProgressFill");
    innerLayout->addWidget(m_progressBarContainer);

    QHBoxLayout *progressLabels = new QHBoxLayout();
    m_lblProgressText = new QLabel("0/0 done", m_outerFrame);
    m_lblProgressText->setFont(QFont("Segoe UI", 9));
    m_lblProgressText->setStyleSheet("opacity: 0.8;");

    m_lblStreak = new QLabel(QString("🔥 Streak: %1 days").arg(m_state.streak), m_outerFrame);
    m_lblStreak->setFont(QFont("Segoe UI", 9, QFont::Bold));
    m_lblStreak->setAlignment(Qt::AlignRight);

    progressLabels->addWidget(m_lblProgressText);
    progressLabels->addWidget(m_lblStreak);
    innerLayout->addLayout(progressLabels);

    QHBoxLayout *footerLay = new QHBoxLayout();
    m_btnReset = new QPushButton("Reset Day", m_outerFrame);
    m_btnReset->setCursor(Qt::PointingHandCursor);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::resetDay);

    m_btnSave = new QPushButton("Save", m_outerFrame);
    m_btnSave->setCursor(Qt::PointingHandCursor);
    m_btnSave->setObjectName("SaveButton");
    connect(m_btnSave, &QPushButton::clicked, this, &MainWindow::saveWidgetState);

    footerLay->addWidget(m_btnReset);
    footerLay->addWidget(m_btnSave);
    innerLayout->addLayout(footerLay);

    buildTaskItems();
    applyThemeStyles();
}

void MainWindow::buildTaskItems() {
    m_listWidget->clear();

    QList<QString> tasks = m_tasksDict.profiles.value(m_selectedProfile);
    QList<bool> compList = m_state.profileStates.value(m_selectedProfile);

    if (compList.size() != tasks.size()) {
        compList = QList<bool>(tasks.size(), false);
        m_state.profileStates[m_selectedProfile] = compList;
    }
    m_state.completed = compList;

    for (int i = 0; i < tasks.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(100, 52));

        TaskItemWidget *widget = new TaskItemWidget(i, tasks[i], compList[i], m_themeName, m_listWidget);
        connect(widget, &TaskItemWidget::taskToggled, this, &MainWindow::onTaskToggled);
        connect(widget, &TaskItemWidget::taskEdited, this, &MainWindow::onTaskEdited);
        connect(widget, &TaskItemWidget::taskDeleted, this, &MainWindow::onTaskDeleted);
        connect(widget, &TaskItemWidget::startTimerRequested, this, &MainWindow::onStartTimer);

        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, widget);
    }
}

void MainWindow::onProfileSwitched(const QString &profileName) {
    m_state.profileStates[m_selectedProfile] = m_state.completed;
    m_selectedProfile = profileName;
    m_tasksDict.selectedProfile = profileName;

    buildTaskItems();
    updateProgress(false);
}

void MainWindow::onTaskToggled(int index, bool checked) {
    if (index < m_state.completed.size()) {
        m_state.completed[index] = checked;
        m_state.profileStates[m_selectedProfile] = m_state.completed;

        if (checked && m_state.soundSelection != "silent") {
            playWavSound(m_state.soundSelection + ".wav");
        }

        updateProgress();
    }
}

void MainWindow::onTaskEdited(int index, const QString &newText) {
    if (index < m_tasksDict.profiles[m_selectedProfile].size()) {
        m_tasksDict.profiles[m_selectedProfile][index] = newText;
        Storage::saveTasks(m_tasksDict);
    }
}

void MainWindow::onTaskDeleted(int index) {
    if (index < m_tasksDict.profiles[m_selectedProfile].size()) {
        m_tasksDict.profiles[m_selectedProfile].removeAt(index);
        m_state.completed.removeAt(index);
        m_state.profileStates[m_selectedProfile] = m_state.completed;

        Storage::saveTasks(m_tasksDict);
        Storage::saveState(m_state);

        buildTaskItems();
        updateProgress();
    }
}

class BeepIntervalDialog : public QDialog {
public:
    explicit BeepIntervalDialog(int defaultValue, const QString &themeName, QWidget *parent = nullptr)
        : QDialog(parent), m_themeName(themeName) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(280, 150);

        QFrame *frame = new QFrame(this);
        frame->setObjectName("DialogFrame");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setXOffset(0);
        shadow->setYOffset(4);
        shadow->setColor(QColor(0, 0, 0, 80));
        frame->setGraphicsEffect(shadow);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->addWidget(frame);

        QVBoxLayout *inner = new QVBoxLayout(frame);
        inner->setContentsMargins(15, 15, 15, 15);
        inner->setSpacing(10);

        QLabel *lbl = new QLabel("Focus Grounding Beep", frame);
        lbl->setFont(QFont("Segoe UI", 10.5, QFont::Bold));
        lbl->setAlignment(Qt::AlignCenter);
        inner->addWidget(lbl);

        QHBoxLayout *ctrls = new QHBoxLayout();
        
        m_spn = new QSpinBox(frame);
        m_spn->setRange(1, 120);
        m_spn->setSuffix(" min");
        m_spn->setValue(defaultValue);
        m_spn->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spn->setAlignment(Qt::AlignCenter);
        m_spn->setFixedWidth(70);

        IconButton *btnDec = new IconButton(IconButton::Minus, m_themeName, frame);
        IconButton *btnInc = new IconButton(IconButton::Plus, m_themeName, frame);

        connect(btnDec, &QPushButton::clicked, this, [this]() { m_spn->setValue(m_spn->value() - 1); });
        connect(btnInc, &QPushButton::clicked, this, [this]() { m_spn->setValue(m_spn->value() + 1); });

        ctrls->addStretch();
        ctrls->addWidget(m_spn);
        ctrls->addWidget(btnDec);
        ctrls->addWidget(btnInc);
        ctrls->addStretch();
        inner->addLayout(ctrls);

        QHBoxLayout *buttons = new QHBoxLayout();
        QPushButton *btnOk = new QPushButton("Start", frame);
        btnOk->setCursor(Qt::PointingHandCursor);
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

        QPushButton *btnCancel = new QPushButton("Cancel", frame);
        btnCancel->setCursor(Qt::PointingHandCursor);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

        buttons->addWidget(btnOk);
        buttons->addWidget(btnCancel);
        inner->addLayout(buttons);

        // Apply stylesheet
        ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);
        frame->setStyleSheet(QString(
            "QFrame#DialogFrame {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 12px;"
            "}"
            "QLabel {"
            "  color: %3;"
            "}"
            "QSpinBox {"
            "  background-color: %4;"
            "  border: 1px solid %2;"
            "  border-radius: 6px;"
            "  color: %3;"
            "  padding: 4px;"
            "  font-size: 13px;"
            "}"
            "QPushButton {"
            "  background-color: %2;"
            "  border: none;"
            "  border-radius: 6px;"
            "  color: %3;"
            "  padding: 6px 12px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: %5;"
            "}"
        ).arg(colors.bg, colors.border, colors.fg, colors.card, colors.accent));
    }

    int value() const { return m_spn->value(); }

private:
    QString m_themeName;
    QSpinBox *m_spn;
};

void MainWindow::onStartTimer(int index) {
    if (m_activeTimer) {
        m_activeTimer->close();
    }

    QString taskText = m_tasksDict.profiles[m_selectedProfile][index];
    int secs = parseDuration(taskText);

    BeepIntervalDialog dlg(m_state.customAlertMinutes, m_themeName, this);
    if (dlg.exec() != QDialog::Accepted) {
        return; // User cancelled starting the task timer
    }
    int intervalMins = dlg.value();

    m_activeTimer = new FocusTimerWidget(taskText, secs, intervalMins, m_themeName);
    connect(m_activeTimer, &FocusTimerWidget::timerFinished, this, &MainWindow::onTimerFinished);
    
    m_activeTimer->move(x() + width() + 10, y());
    m_activeTimer->show();
}

void MainWindow::onTimerFinished() {
    if (m_activeTimer) {
        m_activeTimer->close();
        m_activeTimer = nullptr;
    }

    playWavSound("chime.wav");

    for (int i = 0; i < m_state.completed.size(); ++i) {
        if (!m_state.completed[i]) {
            m_state.completed[i] = true;
            m_state.profileStates[m_selectedProfile] = m_state.completed;
            break;
        }
    }

    buildTaskItems();
    updateProgress();
}

void MainWindow::updateProgress(bool animate) {
    int total = m_state.completed.size();
    int completed = 0;
    for (bool val : m_state.completed) {
        if (val) completed++;
    }

    m_lblProgressText->setText(QString("%1/%2 done").arg(completed).arg(total));

    double percent = (total > 0) ? (static_cast<double>(completed) / total) : 0.0;
    int containerW = 268;
    int fillW = static_cast<int>(percent * containerW);

    if (animate) {
        QPropertyAnimation *anim = new QPropertyAnimation(m_progressBarFill, "geometry", this);
        anim->setDuration(250);
        anim->setStartValue(QRect(m_progressBarFill->x(), m_progressBarFill->y(), m_progressBarFill->width(), m_progressBarFill->height()));
        anim->setEndValue(QRect(0, 0, fillW, 8));
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        m_progressBarFill->setFixedSize(fillW, 8);
    }

    if (completed == total && total > 0) {
        m_lblGreeting->setText("🎉 All Done!");
    } else {
        m_lblGreeting->setText(QString("👋 Hi %1!").arg(m_state.userName));
    }
}

void MainWindow::openSettings() {
    SettingsDialog dlg(m_state, m_tasksDict, m_themeName, this);
    connect(&dlg, &SettingsDialog::settingsSaved, this, [this]() {
        m_themeName = m_state.themeSelection;
        m_lblGreeting->setText(QString("👋 Hi %1!").arg(m_state.userName));
        setPinnedMode(m_state.pinned);
        
        buildTaskItems();
        applyThemeStyles();
        Storage::saveState(m_state);
        Storage::saveTasks(m_tasksDict);
    });
    dlg.exec();
}

void MainWindow::openAnalytics() {
    AnalyticsDialog dlg(m_themeName, m_state.streak, m_state.longestStreak, this);
    dlg.exec();
}

void MainWindow::showResetBanner(const QString &message) {
    QMessageBox::information(this, "Daily Rollover Reset", message);
}

void MainWindow::resetDay() {
    for (int i = 0; i < m_state.completed.size(); ++i) {
        m_state.completed[i] = false;
    }
    m_state.profileStates[m_selectedProfile] = m_state.completed;
    buildTaskItems();
    updateProgress();
}

void MainWindow::saveWidgetState() {
    m_state.windowX = x();
    m_state.windowY = y();
    m_state.profileStates[m_selectedProfile] = m_state.completed;
    Storage::saveState(m_state);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG*>(message);
    if (msg->message == WM_NCHITTEST) {
        QPoint globalPos = QCursor::pos();
        QPoint localPos = mapFromGlobal(globalPos);
        
        int w = width();
        int h = height();
        
        // Center the grab border around the visible outer frame boundary (which is inset by 10px)
        bool left = localPos.x() >= 5 && localPos.x() <= 15;
        bool right = localPos.x() >= w - 15 && localPos.x() <= w - 5;
        bool top = localPos.y() >= 5 && localPos.y() <= 15;
        bool bottom = localPos.y() >= h - 15 && localPos.y() <= h - 5;
        
        if (left && top) { *result = HTTOPLEFT; return true; }
        if (right && top) { *result = HTTOPRIGHT; return true; }
        if (left && bottom) { *result = HTBOTTOMLEFT; return true; }
        if (right && bottom) { *result = HTBOTTOMRIGHT; return true; }
        if (left) { *result = HTLEFT; return true; }
        if (right) { *result = HTRIGHT; return true; }
        if (top) { *result = HTTOP; return true; }
        if (bottom) { *result = HTBOTTOM; return true; }
    }
#endif
    return QWidget::nativeEvent(eventType, message, result);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateProgress(false);
}

void MainWindow::setPinnedMode(bool pinned) {
    HWND hwnd = (HWND)winId();
    if (pinned) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void MainWindow::applyThemeStyles() {
    m_btnSettings->setTheme(m_themeName);
    m_btnAnalytics->setTheme(m_themeName);
    m_btnMinimize->setTheme(m_themeName);
    m_btnClose->setTheme(m_themeName);
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    // Refresh all child item styles
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(m_listWidget->itemWidget(item));
        if (widget) {
            widget->applyTheme(m_themeName);
        }
    }

    m_outerFrame->setStyleSheet(QString(
        "QFrame#MainFrame {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QComboBox {"
        "  background: %4;"
        "  border: 1px solid %5;"
        "  border-radius: 8px;"
        "  padding: 5px 10px;"
        "  color: %3;"
        "  font-weight: bold;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %4;"
        "  border: 1px solid %5;"
        "  selection-background-color: %5;"
        "  color: %3;"
        "}"
        "QWidget#ProgressContainer {"
        "  background: rgba(255, 255, 255, 0.05);"
        "  border-radius: 4px;"
        "}"
        "QWidget#ProgressFill {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %6, stop:1 %7);"
        "  border-radius: 4px;"
        "}"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: transparent;"
        "  width: 6px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255, 255, 255, 0.15);"
        "  border-radius: 3px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: %6;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %5;"
        "  border-radius: 8px;"
        "  padding: 8px 16px;"
        "  color: %3;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %5;"
        "}"
        "QPushButton#SaveButton {"
        "  background: %6;"
        "  color: #11111b;"
        "  border-color: %6;"
        "}"
        "QPushButton#SaveButton:hover {"
        "  background-color: %7;"
        "  border-color: %7;"
        "}"
    ).arg(colors.bg, colors.card, colors.fg, colors.card, colors.border, colors.accent, colors.success));
}

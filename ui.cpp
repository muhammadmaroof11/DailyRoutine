#include "ui.h"
#include "analytics.h"
#include "logic.h"
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QApplication>
#include <QSoundEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <windows.h>
#include <mmsystem.h>

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

// Utility to parse durations, e.g. "Coding (1h 15m)" -> 4500 seconds
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

// Utility to play synthesized WAV files natively on Windows without extra Qt Multimedia dependencies
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


// --- TaskItemWidget ---
TaskItemWidget::TaskItemWidget(int index, const QString &text, bool completed, const QString &themeName, QWidget *parent)
    : QWidget(parent), m_index(index), m_text(text), m_themeName(themeName) {
    
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(8);

    m_chkDone = new QCheckBox(this);
    m_chkDone->setChecked(completed);
    connect(m_chkDone, &QCheckBox::toggled, this, &TaskItemWidget::onCheckBoxToggled);
    layout->addWidget(m_chkDone);

    m_lblText = new EditableLabel(m_text, this);
    m_lblText->setFont(QFont("Segoe UI", 10));
    m_lblText->setWordWrap(true);
    connect(m_lblText, &EditableLabel::doubleClicked, this, &TaskItemWidget::triggerRename);
    layout->addWidget(m_lblText, 1);

    m_lineEdit = new QLineEdit(m_text, this);
    m_lineEdit->setVisible(false);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &TaskItemWidget::saveRename);
    layout->addWidget(m_lineEdit, 1);

    m_btnPlay = new QPushButton("▶", this);
    m_btnPlay->setFixedSize(24, 24);
    m_btnPlay->setCursor(Qt::CursorShape.PointingHandCursor);
    connect(m_btnPlay, &QPushButton::clicked, this, [this]() { emit startTimerRequested(m_index); });
    layout->addWidget(m_btnPlay);

    m_btnDelete = new QPushButton("🗑️", this);
    m_btnDelete->setObjectName("DeleteButton");
    m_btnDelete->setFixedSize(24, 24);
    m_btnDelete->setCursor(Qt::CursorShape.PointingHandCursor);
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
    QString newText = m_lineEdit->text().strip();
    if (!newText.isEmpty()) {
        m_text = newText;
        m_lblText->setText(newText);
        emit taskEdited(m_index, newText);
    }
    m_lineEdit->setVisible(false);
    m_lblText->setVisible(true);
}

void TaskItemWidget::applyTheme(const QString &themeName) {
    m_themeName = themeName;
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    setStyleSheet(QString(
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 5px;"
        "  border: 1px solid %1;"
        "  background: %2;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %3;"
        "  border-color: %3;"
        "}"
        "QLabel {"
        "  color: %4;"
        "}"
        "QLineEdit {"
        "  background: %2;"
        "  border: 1px solid %1;"
        "  border-radius: 4px;"
        "  padding: 2px;"
        "  color: %4;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: %1;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  color: %3;"
        "}"
        "QPushButton#DeleteButton {"
        "  color: %1;"
        "  opacity: 0.5;"
        "}"
        "QPushButton#DeleteButton:hover {"
        "  color: #ff5555;"
        "}"
    ).arg(colors.border, colors.card, colors.accent, colors.fg));
}


// --- FocusTimerWidget ---
FocusTimerWidget::FocusTimerWidget(const QString &taskName, int durationSeconds, int alertIntervalMins, const QString &themeName, QWidget *parent)
    : QWidget(parent), m_taskName(taskName), m_duration(durationSeconds), m_timeLeft(durationSeconds),
      m_alertInterval(alertIntervalMins), m_themeName(themeName) {

    setWindowFlags(Qt::WindowType.FramelessWindowHint | Qt.WindowType.WindowStaysOnTopHint | Qt.WindowType.SubWindow);
    setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground);
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
    innerLayout->setContentsMargins(12, 12, 12, 12);
    innerLayout->setSpacing(8);

    m_lblTask = new QLabel(m_taskName, m_outerFrame);
    m_lblTask->setFont(QFont("Segoe UI", 10, QFont::Weight.Bold));
    m_lblTask->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_lblTask->setWordWrap(true);
    innerLayout->addWidget(m_lblTask);

    m_circleProgress = new QWidget(m_outerFrame);
    m_circleProgress->setFixedSize(130, 130);
    // Draw via subclass/paint event helper below
    innerLayout->addWidget(m_circleProgress, 0, Qt::AlignmentFlag::AlignCenter);

    // +/- 5 minutes row
    QHBoxLayout *adjustLay = new QHBoxLayout();
    m_btnSub5 = new QPushButton("-5 Min", m_outerFrame);
    m_btnSub5->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnSub5, &QPushButton::clicked, this, [this]() { adjustTime(-300); });

    m_btnAdd5 = new QPushButton("+5 Min", m_outerFrame);
    m_btnAdd5->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnAdd5, &QPushButton::clicked, this, [this]() { adjustTime(300); });

    adjustLay->addWidget(m_btnSub5);
    adjustLay->addWidget(m_btnAdd5);
    innerLayout->addLayout(adjustLay);

    // Pause/Cancel controls row
    QHBoxLayout *ctrlLay = new QHBoxLayout();
    m_btnPause = new QPushButton("Pause", m_outerFrame);
    m_btnPause->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnPause, &QPushButton::clicked, this, &FocusTimerWidget::togglePause);

    m_btnCancel = new QPushButton("Cancel", m_outerFrame);
    m_btnCancel.setCursor(Qt.CursorShape.PointingHandCursor);
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

    // Trigger intermittent ADHD alerts
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
    // Draw circular countdown graphic inside the circle placeholder widget
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Map coordinate relative to child container
    QPoint circlePos = m_circleProgress->mapTo(this, QPoint(0, 0));
    QRectF rect(circlePos.x() + 5, circlePos.y() + 5, 120, 120);

    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    // 1. Draw track background
    painter.setPen(QPen(QColor(colors.border), 8));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect);

    // 2. Draw active progress segment
    painter.setPen(QPen(QColor(colors.accent), 8, Qt::SolidLine, Qt::RoundCap));
    double fraction = static_cast<double>(m_timeLeft) / m_duration;
    int spanAngle = static_cast<int>(fraction * 360 * 16);
    painter.drawArc(rect, 90 * 16, spanAngle); // start from top (90 deg)

    // 3. Draw text label centered inside
    int mins = m_timeLeft / 60;
    int secs = m_timeLeft % 60;
    QString timeText = QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));

    painter.setPen(QColor(colors.fg));
    painter.setFont(QFont("Segoe UI", 16, QFont::Weight.Bold));
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
        "  border-radius: 12px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
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
OnboardingDialog::OnboardingDialog(Storage::AppState &state, const QString &themeName, QWidget *parent)
    : QDialog(parent), m_state(state), m_themeName(themeName) {
    initUI();
}

void OnboardingDialog::initUI() {
    setWindowFlags(Qt::WindowType.FramelessWindowHint | Qt.WindowType.Dialog);
    setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground);
    setFixedSize(340, 320);

    m_outerFrame = new QFrame(this);
    m_outerFrame->setObjectName("OnboardFrame");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 85));
    m_outerFrame->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_outerFrame);

    QVBoxLayout *innerLayout = new QVBoxLayout(m_outerFrame);
    innerLayout->setContentsMargins(20, 20, 20, 20);

    m_stack = new QStackedWidget(m_outerFrame);
    m_stack->addWidget(createSlide1());
    m_stack->addWidget(createSlide2());
    m_stack->addWidget(createSlide3());
    m_stack->addWidget(createSlide4());
    innerLayout->addWidget(m_stack);

    // Wizard actions bottom row
    QHBoxLayout *btnLay = new QHBoxLayout();
    m_btnPrev = new QPushButton("Back", m_outerFrame);
    m_btnPrev->setCursor(Qt.CursorShape.PointingHandCursor);
    m_btnPrev->setEnabled(false);
    connect(m_btnPrev, &QPushButton::clicked, this, &OnboardingDialog::prevPage);

    m_btnNext = new QPushButton("Next", m_outerFrame);
    m_btnNext->setCursor(Qt.CursorShape.PointingHandCursor);
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
    lay->setSpacing(10);

    QLabel *title = new QLabel("WELCOME TO ROUTINE", w);
    title->setFont(QFont("Segoe UI", 12, QFont::Weight.Bold));
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
    lay->setSpacing(8);

    QLabel *title = new QLabel("PERSONALIZATION", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Weight.Bold));
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    // Name
    QHBoxLayout *nameLay = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("Name:", w);
    m_txtUserName = new QLineEdit(m_state.userName, w);
    m_txtUserName->setPlaceholderText("Friend");
    nameLay->addWidget(lbl1);
    nameLay->addWidget(m_txtUserName, 1);
    lay->addLayout(nameLay);

    // Theme preset
    QHBoxLayout *themeLay = new QHBoxLayout();
    QLabel *lbl2 = new QLabel("Theme:", w);
    m_cmbTheme = new QComboBox(w);
    m_cmbTheme->addItems(list(THEMES.keys()));
    m_cmbTheme->setCurrentText(m_themeName);
    connect(m_cmbTheme, &QComboBox::currentTextChanged, this, &OnboardingDialog::onThemeSelected);
    themeLay->addWidget(lbl2);
    themeLay->addWidget(m_cmbTheme, 1);
    lay->addLayout(themeLay);

    // Alert minutes
    QHBoxLayout *alertLay = new QHBoxLayout();
    QLabel *lbl3 = new QLabel("Beep Reminder:", w);
    m_spnAlert = new QSpinBox(w);
    m_spnAlert->setRange(1, 30);
    m_spnAlert->setSuffix(" min");
    m_spnAlert->setValue(m_state.customAlertMinutes);
    alertLay->addWidget(lbl3);
    alertLay->addWidget(m_spnAlert);
    lay->addLayout(alertLay);

    lay->addStretch();
    return w;
}

QWidget* OnboardingDialog::createSlide3() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    QLabel *title = new QLabel("PLATFORM GUIDE", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Weight.Bold));
    title->setAlignment(Qt::AlignCenter);

    QLabel *body = new QLabel("- Drag Widget: Grab header and slide anywhere.\n- Start Session: Click play next to tasks.\n- Rename: Double-click task text directly.\n- Check-in Chirps: Plays alert beeps periodically to redirect floating focus.", w);
    body->setFont(QFont("Segoe UI", 9));
    body->setWordWrap(true);

    lay->addWidget(title);
    lay->addWidget(body);
    lay->addStretch();
    return w;
}

QWidget* OnboardingDialog::createSlide4() {
    QWidget *w = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(12);

    QLabel *title = new QLabel("YOU'RE ALL SET!", w);
    title->setFont(QFont("Segoe UI", 11, QFont::Weight.Bold));
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
    applyThemeStyles();
    emit themeChanged(theme);
}

void OnboardingDialog::nextPage() {
    if (m_currentSlide < 3) {
        m_currentSlide++;
        m_stack->setCurrentIndex(m_currentSlide);
        m_btnPrev->setEnabled(true);
        if (m_currentSlide == 3) {
            m_btnNext->setText("Get Started");
        }
    } else {
        // Save wizard answers
        m_state.userName = m_txtUserName->text().strip().isEmpty() ? "Friend" : m_txtUserName->text().strip();
        m_state.themeSelection = m_cmbTheme->currentText();
        m_state.customAlertMinutes = m_spnAlert->value();
        m_state.onboarded = true;
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
        "  border-radius: 12px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QLineEdit {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "  padding: 6px;"
        "  color: %3;"
        "}"
        "QComboBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 5px;"
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
        "  border-radius: 5px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
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
    setWindowFlags(Qt::WindowType.FramelessWindowHint | Qt.WindowType.Dialog);
    setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground);
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
    title->setFont(QFont("Segoe UI", 12, QFont::Weight.Bold));
    title->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(title);

    // Username
    QHBoxLayout *nameLay = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("User Name:", m_outerFrame);
    lbl1->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    m_txtUserName = new QLineEdit(m_state.userName, m_outerFrame);
    nameLay->addWidget(lbl1);
    nameLay->addWidget(m_txtUserName, 1);
    innerLayout->addLayout(nameLay);

    // Theme preset
    QHBoxLayout *themeLay = new QHBoxLayout();
    QLabel *lbl2 = new QLabel("Theme Preset:", m_outerFrame);
    lbl2->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    m_cmbTheme = new QComboBox(m_outerFrame);
    m_cmbTheme->addItems(list(THEMES.keys()));
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
    lbl3->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    m_cmbSound = new QComboBox(m_outerFrame);
    m_cmbSound->addItems({"pop", "sparkle", "chime", "silent"});
    m_cmbSound->setCurrentText(m_state.soundSelection);
    soundLay->addWidget(lbl3);
    soundLay->addWidget(m_cmbSound, 1);
    innerLayout->addLayout(soundLay);

    // Alert spinbox
    QHBoxLayout *alertLay = new QHBoxLayout();
    QLabel *lbl4 = new QLabel("Focus Beep Reminder:", m_outerFrame);
    lbl4->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    m_spnAlert = new QSpinBox(m_outerFrame);
    m_spnAlert->setRange(1, 30);
    m_spnAlert->setSuffix(" min");
    m_spnAlert->setValue(m_state.customAlertMinutes);
    alertLay->addWidget(lbl4);
    alertLay->addWidget(m_spnAlert);
    innerLayout->addLayout(alertLay);

    // Separator line
    QFrame *line = new QFrame(m_outerFrame);
    line->setFrameShape(QFrame.Shape.HLine);
    line->setFrameShadow(QFrame.Shadow.Sunken);
    innerLayout->addWidget(line);

    // Profile Management Section
    QLabel *profTitle = new QLabel("MANAGE PROFILES", m_outerFrame);
    profTitle->setFont(QFont("Segoe UI", 10, QFont::Weight.Bold));
    profTitle->setAlignment(Qt::AlignCenter);
    innerLayout->addWidget(profTitle);

    QHBoxLayout *profLay = new QHBoxLayout();
    m_txtProfInput = new QLineEdit(m_outerFrame);
    m_txtProfInput->setPlaceholderText("Create new profile name");
    m_btnCreateProf = new QPushButton("Create", m_outerFrame);
    m_btnCreateProf->setCursor(Qt::CursorShape.PointingHandCursor);
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
    m_btnDeleteProf->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnDeleteProf, &QPushButton::clicked, this, &SettingsDialog::deleteSelectedProfile);
    profDelLay->addWidget(m_cmbDelProfile, 1);
    profDelLay->addWidget(m_btnDeleteProf);
    innerLayout->addLayout(profDelLay);

    // Add Task to selected profile
    QLabel *taskLbl = new QLabel("Add task to selected profile:", m_outerFrame);
    taskLbl->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    innerLayout->addWidget(taskLbl);

    QHBoxLayout *taskLay = new QHBoxLayout();
    m_txtTaskInput = new QLineEdit(m_outerFrame);
    m_txtTaskInput->setPlaceholderText("e.g. Coding (2h)");
    m_btnAddTask = new QPushButton("Add Task", m_outerFrame);
    m_btnAddTask->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnAddTask, &QPushButton::clicked, this, &SettingsDialog::addCustomTask);
    taskLay->addWidget(m_txtTaskInput, 1);
    taskLay->addWidget(m_btnAddTask);
    innerLayout->addLayout(taskLay);

    innerLayout->addStretch();

    // Footer actions
    QHBoxLayout *btnLay = new QHBoxLayout();
    m_btnCancel = new QPushButton("Cancel", m_outerFrame);
    m_btnCancel->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton("Save", m_outerFrame);
    m_btnSave->setCursor(Qt.CursorShape.PointingHandCursor);
    m_btnSave->setObjectName("SaveButton");
    connect(m_btnSave, &QPushButton::clicked, this, &SettingsDialog::saveSettings);

    btnLay->addWidget(m_btnCancel);
    btnLay->addWidget(m_btnSave);
    innerLayout->addLayout(btnLay);

    applyThemeStyles();
}

void SettingsDialog::createNewProfile() {
    QString name = m_txtProfInput->text().strip();
    if (!name.isEmpty() && !m_tasksDict.profiles.contains(name)) {
        m_tasksDict.profiles[name] = QList<QString>();
        m_state.profileStates[name] = QList<bool>();
        m_txtProfInput->clear();
        m_txtProfInput->setPlaceholderText("Created!");

        // Update delete combobox
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
    QString task = m_txtTaskInput->text().strip();
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
    m_state.userName = m_txtUserName->text().strip().isEmpty() ? "Friend" : m_txtUserName->text().strip();

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
        "  border-radius: 5px;"
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
        "  border-radius: 12px;"
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
        "  border-radius: 6px;"
        "  padding: 6px;"
        "  color: %3;"
        "  font-size: 12px;"
        "}"
        "QSpinBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 5px;"
        "  padding: 4px;"
        "  color: %3;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
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
    setWindowFlags(Qt::WindowType.FramelessWindowHint);
    setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground);
    setFixedSize(300, 520);

    // Apply clamp boundary guard
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
    m_lblGreeting->setFont(QFont("Segoe UI", 11, QFont::Weight.Bold));
    header->addWidget(m_lblGreeting, 1);

    m_btnSettings = new QPushButton("⚙️", m_outerFrame);
    m_btnSettings->setFixedSize(28, 28);
    m_btnSettings->setCursor(Qt::CursorShape.PointingHandCursor);
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::openSettings);

    m_btnAnalytics = new QPushButton("📈", m_outerFrame);
    m_btnAnalytics->setFixedSize(28, 28);
    m_btnAnalytics->setCursor(Qt::CursorShape.PointingHandCursor);
    connect(m_btnAnalytics, &QPushButton::clicked, this, &MainWindow::openAnalytics);

    header->addWidget(m_btnAnalytics);
    header->addWidget(m_btnSettings);
    innerLayout->addLayout(header);

    // Profile selector
    m_cmbProfile = new QComboBox(m_outerFrame);
    m_cmbProfile->addItems(m_tasksDict.profiles.keys());
    m_cmbProfile->setCurrentText(m_selectedProfile);
    connect(m_cmbProfile, &QComboBox::currentTextChanged, this, &MainWindow::onProfileSwitched);
    innerLayout->addWidget(m_cmbProfile);

    // Tasks checklist
    m_listWidget = new QListWidget(m_outerFrame);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setSpacing(4);
    m_listWidget->setStyleSheet("background: transparent;");
    innerLayout->addWidget(m_listWidget, 1);

    // Progress Bar card
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
    m_lblStreak->setFont(QFont("Segoe UI", 9, QFont::Weight.Bold));
    m_lblStreak->setAlignment(Qt::AlignRight);

    progressLabels->addWidget(m_lblProgressText);
    progressLabels->addWidget(m_lblStreak);
    innerLayout->addLayout(progressLabels);

    // Footer actions
    QHBoxLayout *footerLay = new QHBoxLayout();
    m_btnReset = new QPushButton("Reset Day", m_outerFrame);
    m_btnReset->setCursor(Qt.CursorShape.PointingHandCursor);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::resetDay);

    m_btnSave = new QPushButton("Save", m_outerFrame);
    m_btnSave->setCursor(Qt.CursorShape.PointingHandCursor);
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

    // Align length
    if (compList.size() != tasks.size()) {
        compList = QList<bool>(tasks.size(), false);
        m_state.profileStates[m_selectedProfile] = compList;
    }
    m_state.completed = compList;

    for (int i = 0; i < tasks.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(100, 44));

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

        // Play pop sound
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

void MainWindow::onStartTimer(int index) {
    if (m_activeTimer) {
        m_activeTimer->close();
    }

    QString taskText = m_tasksDict.profiles[m_selectedProfile][index];
    int secs = parseDuration(taskText);

    m_activeTimer = new FocusTimerWidget(taskText, secs, m_state.customAlertMinutes, m_themeName);
    connect(m_activeTimer, &FocusTimerWidget::timerFinished, this, &MainWindow::onTimerFinished);
    
    // Set near dashboard
    m_activeTimer->move(x() + width() + 10, y());
    m_activeTimer->show();
}

void MainWindow::onTimerFinished() {
    if (m_activeTimer) {
        m_activeTimer->close();
        m_activeTimer = nullptr;
    }

    // Level up sound
    playWavSound("chime.wav");

    // Automatically check off task item
    // For safety, set first uncompleted task
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
    int containerW = 268; // width minus margins
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

    // Check complete dopamine splash
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
    Storage::saveTasks(m_tasksDict);
}

void MainWindow::setPinnedMode(bool pinned) {
    m_state.pinned = pinned;
    // Set Window stays on top flag dynamically
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (pinned) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
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

void MainWindow::applyThemeStyles() {
    ThemeColors colors = THEMES.value(m_themeName, THEMES["catppuccin"]);

    m_outerFrame->setStyleSheet(QString(
        "QFrame#MainFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 12px;"
        "}"
        "QLabel {"
        "  color: %3;"
        "}"
        "QComboBox {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  color: %3;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %4;"
        "  border: 1px solid %2;"
        "  selection-background-color: %2;"
        "  color: %3;"
        "}"
        "QWidget#ProgressContainer {"
        "  background-color: %4;"
        "  border-radius: 4px;"
        "}"
        "QWidget#ProgressFill {"
        "  background-color: %5;"
        "  border-radius: 4px;"
        "}"
        "QPushButton {"
        "  background: %4;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
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

    m_cmbProfile->setStyleSheet(QString(
        "QComboBox {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 5px;"
        "  padding: 4px 8px;"
        "  color: %3;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  color: %3;"
        "  selection-background-color: %2;"
        "}"
    ).arg(colors.card, colors.border, colors.fg));

    // Update list widget children themes
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        TaskItemWidget *w = qobject_cast<TaskItemWidget*>(m_listWidget->itemWidget(item));
        if (w) w->applyTheme(m_themeName);
    }
}

#ifndef UI_H
#define UI_H

#include <QWidget>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include "storage.h"

// Custom double-clickable Label for quick renames
class EditableLabel : public QLabel {
    Q_OBJECT
public:
    explicit EditableLabel(const QString &text, QWidget *parent = nullptr);
signals:
    void doubleClicked();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

// Premium Vector IconButton
class IconButton : public QPushButton {
    Q_OBJECT
public:
    enum IconType { Settings, Analytics, Play, Pause, Trash, Close, Plus, Minus, Check, Add5, Sub5, Minimize };
    explicit IconButton(IconType type, const QString &themeName, QWidget *parent = nullptr);
    void setTheme(const QString &themeName);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    IconType m_type;
    QString m_themeName;
    bool m_hovered = false;
};

// Premium Custom Checkbox
class CustomCheckBox : public QAbstractButton {
    Q_OBJECT
public:
    explicit CustomCheckBox(const QString &themeName, QWidget *parent = nullptr);
    void setTheme(const QString &themeName);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    QString m_themeName;
    bool m_hovered = false;
};

// Task item row widget
class TaskItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskItemWidget(int index, const QString &text, bool completed, const QString &themeName, QWidget *parent = nullptr);
    void applyTheme(const QString &themeName);

signals:
    void taskToggled(int index, bool checked);
    void taskEdited(int index, const QString &newText);
    void startTimerRequested(int index);
    void taskDeleted(int index);

private slots:
    void onCheckBoxToggled(bool checked);
    void triggerRename();
    void saveRename();

private:
    int m_index;
    QString m_text;
    QString m_themeName;

    CustomCheckBox *m_chkDone;
    EditableLabel *m_lblText;
    QLineEdit *m_lineEdit;
    IconButton *m_btnPlay;
    IconButton *m_btnDelete;
    QFrame *m_cardFrame;
};

// Focus countdown circular timer overlay
class FocusTimerWidget : public QWidget {
    Q_OBJECT
public:
    explicit FocusTimerWidget(const QString &taskName, int durationSeconds, int alertIntervalMins = 5, const QString &themeName = "catppuccin", QWidget *parent = nullptr);
    void setTheme(const QString &themeName);

signals:
    void timerFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onTick();
    void togglePause();
    void adjustTime(int deltaSecs);

private:
    void applyThemeStyles();
    void playTickSound();

    QString m_taskName;
    int m_duration;
    int m_timeLeft;
    int m_alertInterval;
    QString m_themeName;
    bool m_isPaused = false;

    QPoint m_dragPos;
    QTimer *m_timer;

    QFrame *m_outerFrame;
    QLabel *m_lblTask;
    QWidget *m_circleProgress;
    QPushButton *m_btnPause;
    QPushButton *m_btnCancel;
    IconButton *m_btnAdd5;
    IconButton *m_btnSub5;
};

// Stacked onboarding wizard
class OnboardingDialog : public QDialog {
    Q_OBJECT
public:
    explicit OnboardingDialog(Storage::AppState &state, Storage::TasksConfig &tasksDict, const QString &themeName = "catppuccin", QWidget *parent = nullptr);

signals:
    void themeChanged(const QString &newTheme);

private slots:
    void nextPage();
    void prevPage();
    void onThemeSelected(const QString &theme);
    void addOnboardTask();
    void removeOnboardTask();

private:
    void initUI();
    void applyThemeStyles();
    QWidget* createSlide1();
    QWidget* createSlide2();
    QWidget* createSlide3TaskBuilder();
    QWidget* createSlide4Guide();
    QWidget* createSlide5AllSet();

    Storage::AppState &m_state;
    Storage::TasksConfig &m_tasksDict;
    QString m_themeName;
    int m_currentSlide = 0;

    QFrame *m_outerFrame;
    QStackedWidget *m_stack;
    QPushButton *m_btnNext;
    QPushButton *m_btnPrev;

    // Slide 2 Inputs
    QLineEdit *m_txtUserName;
    QComboBox *m_cmbTheme;
    QSpinBox *m_spnAge;
    QLineEdit *m_txtFocusGoal;

    // Slide 3 Inputs
    QListWidget *m_lstTasks;
    QLineEdit *m_txtTaskInput;
    IconButton *m_btnAddTask;
};

// Main settings dialogues panel
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Storage::AppState &state, Storage::TasksConfig &tasksDict, const QString &themeName = "catppuccin", QWidget *parent = nullptr);

signals:
    void settingsSaved();

private slots:
    void createNewProfile();
    void deleteSelectedProfile();
    void addCustomTask();
    void saveSettings();

private:
    void initUI();
    void applyThemeStyles();

    Storage::AppState &m_state;
    Storage::TasksConfig &m_tasksDict;
    QString m_themeName;

    QFrame *m_outerFrame;
    QLineEdit *m_txtUserName;
    QComboBox *m_cmbTheme;
    QCheckBox *m_chkPinned;
    QCheckBox *m_chkStartup;
    QComboBox *m_cmbSound;
    QSpinBox *m_spnAlert;
    
    // Profile Management
    QLineEdit *m_txtProfInput;
    QPushButton *m_btnCreateProf;
    QComboBox *m_cmbDelProfile;
    QPushButton *m_btnDeleteProf;

    // Add Task
    QLineEdit *m_txtTaskInput;
    QPushButton *m_btnAddTask;

    QPushButton *m_btnCancel;
    QPushButton *m_btnSave;
};

// Main widget dashboard layout wrapper
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Storage::AppState &state, Storage::TasksConfig &tasksDict, QWidget *parent = nullptr);
    ~MainWindow();

    void buildTaskItems();
    void openSettings();
    void openAnalytics();
    void showResetBanner(const QString &message);
    void saveWidgetState();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateProgress(bool animate = true);
    void onProfileSwitched(const QString &profileName);
    void onTaskToggled(int index, bool checked);
    void onTaskEdited(int index, const QString &newText);
    void onTaskDeleted(int index);
    void onStartTimer(int index);
    void onTimerFinished();
    void resetDay();

private:
    void initUI();
    void applyThemeStyles();
    void setPinnedMode(bool pinned);

    Storage::AppState &m_state;
    Storage::TasksConfig &m_tasksDict;
    QString m_selectedProfile;
    QString m_themeName;

    QPoint m_dragPos;
    FocusTimerWidget *m_activeTimer = nullptr;

    QFrame *m_outerFrame;
    QLabel *m_lblGreeting;
    QComboBox *m_cmbProfile;
    IconButton *m_btnSettings;
    IconButton *m_btnAnalytics;
    IconButton *m_btnMinimize;
    IconButton *m_btnClose;
    QListWidget *m_listWidget;
    
    // Progress bar
    QWidget *m_progressBarContainer;
    QWidget *m_progressBarFill;
    QLabel *m_lblProgressText;
    QLabel *m_lblStreak;

    // Actions
    QPushButton *m_btnReset;
    QPushButton *m_btnSave;
};

#endif // UI_H

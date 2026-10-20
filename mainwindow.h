#pragma once
#include "preset_manager.h"
#include "protect-eyes/intervalpage.h"
#include "protect-eyes/timer.h"
#include "settings_manager.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QVector>

struct MonitorWidgets {
    QLabel*    name_label        = nullptr;
    QSlider*   brightness_slider = nullptr;
    QLabel*    brightness_val    = nullptr;
    QCheckBox* night_mode        = nullptr;
    QSlider*   warmth_slider     = nullptr;
    QLabel*    warmth_val        = nullptr;
    QWidget*   warmth_container  = nullptr;
    QString    output_name;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

  protected:
    void closeEvent(QCloseEvent* event) override;

  private slots:
    void on_brightness_changed(int idx, int value);
    void on_night_mode_toggled(int idx, bool checked);
    void on_warmth_changed(int idx, int value);
    void on_tray_activated(QSystemTrayIcon::ActivationReason reason);
    void on_break_settings_changed();
    void on_worktime_timeout(quint16 break_minutes);
    void on_interval_timeout();
    void update_countdown(const QTime& remaining, bool is_working);
    void set_day_mode();
    void set_night_mode();

    void save_current_as_preset();
    void load_selected_preset();
    void set_selected_as_default();
    void delete_selected_preset();
    void on_preset_list_double_clicked(QListWidgetItem* item);

  private:
    void setup_ui();
    void setup_tray();
    void setup_break_scheduler();
    void apply_monitor(int idx);
    void apply_preset(const QJsonObject& preset);
    void load_settings();
    void save_settings();
    void refresh_preset_list();
    void set_break_controls(const QJsonObject& settings);

    SettingsManager m_settings;
    PresetManager   m_presets;

    QVector<MonitorWidgets> m_monitors;

    QTabWidget*      m_tab_widget        = nullptr;
    QCheckBox*       m_sync_brightness   = nullptr;
    QCheckBox*       m_sync_night        = nullptr;
    QSystemTrayIcon* m_tray_icon         = nullptr;
    QListWidget*     m_preset_list       = nullptr;
    QLineEdit*       m_preset_name_edit  = nullptr;
    QCheckBox*       m_break_enabled     = nullptr;
    QSpinBox*        m_work_minutes      = nullptr;
    QSpinBox*        m_break_minutes     = nullptr;
    QLineEdit*       m_break_message     = nullptr;
    QLabel*          m_countdown_label   = nullptr;
    Timer*           m_timer             = nullptr;
    IntervalPage*    m_interval_page     = nullptr;
    bool             m_updating          = false;
};

#include "mainwindow.h"
#include "color_temp.h"
#include "monitor_ctrl.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <qnamespace.h>
#include <qspinbox.h>

static constexpr int BRIGHTNESS_MIN = 10;
static constexpr int BRIGHTNESS_MAX = 200;
static constexpr int WARMTH_MIN = 1000;
static constexpr int WARMTH_MAX = 7500;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

  setWindowTitle("Screen Light Adjust");
  setMinimumWidth(400);

  for (const auto &mi : MonitorController::discover()) {
    MonitorWidgets mw;
    mw.output_name = mi.name;
    m_monitors.append(mw);
  }

  setup_ui();
  setup_tray();
  load_settings();
  setup_break_scheduler();

  connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::save_settings);
}

// ============================================================
//  UI Setup
// ============================================================

void MainWindow::setup_ui() {
  auto *central = new QWidget(this);
  auto *main_layout = new QVBoxLayout(central);
  main_layout->setContentsMargins(12, 12, 12, 12);
  main_layout->setSpacing(8);

  m_tab_widget = new QTabWidget;

  // ---------- Tab 1: Display ----------
  auto *display_tab = new QWidget;
  auto *display_layout = new QVBoxLayout(display_tab);
  display_layout->setContentsMargins(0, 8, 0, 0);
  display_layout->setSpacing(8);

  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto *scroll_content = new QWidget;
  auto *monitors_layout = new QVBoxLayout(scroll_content);
  monitors_layout->setSpacing(10);

  for (int i = 0; i < m_monitors.size(); ++i) {
    auto *mw = &m_monitors[i];

    auto *group = new QGroupBox;
    auto *gl = new QVBoxLayout(group);
    gl->setSpacing(6);

    // Monitor name
    const auto display_name =
        mw->output_name == QStringLiteral("wayland-global")
            ? QStringLiteral("Wayland（所有显示器）")
            : mw->output_name;
    auto *name = new QLabel(display_name);
    QFont name_font = name->font();
    name_font.setBold(true);
    name_font.setPointSize(name_font.pointSize() + 1);
    name->setFont(name_font);
    gl->addWidget(name);

    // Brightness row
    auto *br_row = new QHBoxLayout;
    auto *br_label = new QLabel("亮度");
    br_label->setFixedWidth(36);
    auto *br_slider = new QSlider(Qt::Horizontal);
    br_slider->setRange(BRIGHTNESS_MIN, MonitorController::isWaylandSession()
                                            ? 100
                                            : BRIGHTNESS_MAX);

    // bright spin box
    auto *br_spin = new QSpinBox;
    br_spin->setRange(BRIGHTNESS_MIN, MonitorController::isWaylandSession()
                                          ? 100
                                          : BRIGHTNESS_MAX);
    br_spin->setSuffix("%");
    br_spin->setSingleStep(1);
    br_spin->setKeyboardTracking(false);
    br_spin->setAccelerated(true);
    br_spin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    br_spin->setValue(100);

    br_slider->setValue(100);

    br_row->addWidget(br_label);
    br_row->addWidget(br_slider, 1);
    br_row->addWidget(br_spin);
    gl->addLayout(br_row);

    // Night mode toggle
    auto *night = new QCheckBox("护眼模式");
    gl->addWidget(night);

    // Warmth row (hidden until night mode on)
    auto *wm_row = new QHBoxLayout;
    auto *wm_label = new QLabel("色温");
    wm_label->setFixedWidth(36);
    auto *wm_slider = new QSlider(Qt::Horizontal);
    wm_slider->setRange(WARMTH_MIN, WARMTH_MAX);
    wm_slider->setSingleStep(50);
    wm_slider->setPageStep(500);
    wm_slider->setValue(4500);

    // wm spin box
    auto *wm_spin = new QSpinBox;
    wm_spin->setRange(WARMTH_MIN, WARMTH_MAX);
    wm_spin->setSuffix("K");
    wm_spin->setSingleStep(50);
    wm_spin->setKeyboardTracking(false);
    wm_spin->setAccelerated(true);
    wm_spin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    wm_spin->setFixedWidth(80);
    wm_spin->setValue(4500);

    wm_row->addWidget(wm_label);
    wm_row->addWidget(wm_slider, 1);
    wm_row->addWidget(wm_spin);

    auto *wm_container = new QWidget;
    wm_container->setLayout(wm_row);
    wm_container->setVisible(false);
    gl->addWidget(wm_container);

    monitors_layout->addWidget(group);

    mw->name_label = name;
    mw->brightness_slider = br_slider;
    mw->brightness_spin = br_spin;
    mw->night_mode = night;
    mw->warmth_slider = wm_slider;
    mw->warmth_spin = wm_spin;
    mw->warmth_container = wm_container;

    // Brightness connections
    connect(br_slider, &QSlider::valueChanged, this,
            [this, i](int v) { on_brightness_changed(i, v); });

    connect(br_slider, &QSlider::valueChanged, br_spin, &QSpinBox::setValue);
    connect(br_spin, QOverload<int>::of(&QSpinBox::valueChanged), br_slider,
            &QSlider::setValue);

    connect(night, &QCheckBox::toggled, this,
            [this, i](bool c) { on_night_mode_toggled(i, c); });

    // Warmth connections
    connect(wm_slider, &QSlider::valueChanged, this,
            [this, i](int v) { on_warmth_changed(i, v); });
    connect(wm_slider, &QSlider::valueChanged, wm_spin, &QSpinBox::setValue);
    connect(wm_spin, QOverload<int>::of(&QSpinBox::valueChanged), wm_slider,
            &QSlider::setValue);
  }

  if (m_monitors.isEmpty()) {
    auto *unavailable = new QLabel(
        MonitorController::isWaylandSession()
            ? QStringLiteral("未检测到可调整的显示器；休息提醒仍可使用。")
            : QStringLiteral("未检测到 xrandr 显示输出；休息提醒仍可使用。"));
    unavailable->setWordWrap(true);
    unavailable->setStyleSheet(QStringLiteral("color: #a33;"));
    monitors_layout->addWidget(unavailable);
  } else if (MonitorController::isWaylandSession()) {
    auto *wayland_hint = new QLabel(QStringLiteral(
        "Wayland "
        "色温和亮度由桌面合成器统一调整，休息提醒会显示在每个屏幕。"));
    wayland_hint->setWordWrap(true);
    wayland_hint->setStyleSheet(QStringLiteral("color: gray;"));
    monitors_layout->addWidget(wayland_hint);
  }

  scroll->setWidget(scroll_content);
  display_layout->addWidget(scroll, 1);

  // Separator
  auto *sep = new QFrame;
  sep->setFrameShape(QFrame::HLine);
  sep->setFrameShadow(QFrame::Sunken);
  display_layout->addWidget(sep);

  // Sync toggles
  auto *sync_row = new QHBoxLayout;
  m_sync_brightness = new QCheckBox("同步亮度");
  m_sync_night = new QCheckBox("同步护眼模式");
  sync_row->addWidget(m_sync_brightness);
  sync_row->addStretch();
  sync_row->addWidget(m_sync_night);

  auto *sync_container = new QWidget;
  sync_container->setLayout(sync_row);
  display_layout->addWidget(sync_container);

  m_tab_widget->addTab(display_tab, "显示");

  // ---------- Tab 2: Break reminders ----------
  auto *break_tab = new QWidget;
  auto *break_layout = new QVBoxLayout(break_tab);
  break_layout->setContentsMargins(12, 12, 12, 12);
  break_layout->setSpacing(10);

  m_break_enabled = new QCheckBox(QStringLiteral("启用定时休息提醒"));
  break_layout->addWidget(m_break_enabled);

  auto *break_form = new QFormLayout;
  m_work_minutes = new QSpinBox;
  m_work_minutes->setRange(1, 150);
  m_work_minutes->setSuffix(QStringLiteral(" 分钟"));
  m_work_minutes->setValue(120);
  m_break_minutes = new QSpinBox;
  m_break_minutes->setRange(1, 150);
  m_break_minutes->setSuffix(QStringLiteral(" 分钟"));
  m_break_minutes->setValue(20);
  m_break_message = new QLineEdit;
  m_break_message->setPlaceholderText(
      QStringLiteral("输入每个屏幕都显示的休息提示"));
  break_form->addRow(QStringLiteral("工作时长"), m_work_minutes);
  break_form->addRow(QStringLiteral("休息时长"), m_break_minutes);
  break_form->addRow(QStringLiteral("提示语句"), m_break_message);
  break_layout->addLayout(break_form);

  m_countdown_label = new QLabel(QStringLiteral("休息提醒尚未启动"));
  m_countdown_label->setAlignment(Qt::AlignCenter);
  break_layout->addWidget(m_countdown_label);
  break_layout->addStretch();
  m_tab_widget->addTab(break_tab, QStringLiteral("休息"));

  // ---------- Tab 3: Presets ----------
  auto *presets_tab = new QWidget;
  auto *presets_layout = new QVBoxLayout(presets_tab);
  presets_layout->setContentsMargins(8, 8, 8, 8);
  presets_layout->setSpacing(8);

  auto *preset_title = new QLabel("预设管理");
  QFont title_font = preset_title->font();
  title_font.setBold(true);
  title_font.setPointSize(title_font.pointSize() + 1);
  preset_title->setFont(title_font);
  presets_layout->addWidget(preset_title);

  m_preset_list = new QListWidget;
  m_preset_list->setAlternatingRowColors(true);
  presets_layout->addWidget(m_preset_list, 1);

  // Save as preset
  auto *save_row = new QHBoxLayout;
  m_preset_name_edit = new QLineEdit;
  m_preset_name_edit->setPlaceholderText("输入预设名称...");
  auto *save_btn = new QPushButton("保存当前为预设");
  save_row->addWidget(m_preset_name_edit, 1);
  save_row->addWidget(save_btn);
  presets_layout->addLayout(save_row);

  // Load / Delete / Set default
  auto *action_row = new QHBoxLayout;
  auto *load_btn = new QPushButton("加载预设");
  auto *del_btn = new QPushButton("删除预设");
  auto *def_btn = new QPushButton("设为默认预设");
  action_row->addWidget(load_btn);
  action_row->addWidget(del_btn);
  action_row->addWidget(def_btn);
  presets_layout->addLayout(action_row);

  presets_layout->addStretch();

  m_tab_widget->addTab(presets_tab, "预设");

  main_layout->addWidget(m_tab_widget, 1);

  // Hint
  auto *hint =
      new QLabel("关闭窗口后进入系统托盘；桌面没有托盘服务时会退出程序。");
  hint->setAlignment(Qt::AlignCenter);
  QFont hint_font = hint->font();
  hint_font.setPointSize(hint_font.pointSize() - 1);
  hint->setFont(hint_font);
  hint->setStyleSheet("color: gray;");
  main_layout->addWidget(hint);

  setCentralWidget(central);

  // Connections for preset tab
  connect(save_btn, &QPushButton::clicked, this,
          &MainWindow::save_current_as_preset);
  connect(load_btn, &QPushButton::clicked, this,
          &MainWindow::load_selected_preset);
  connect(del_btn, &QPushButton::clicked, this,
          &MainWindow::delete_selected_preset);
  connect(def_btn, &QPushButton::clicked, this,
          &MainWindow::set_selected_as_default);
  connect(m_preset_list, &QListWidget::itemDoubleClicked, this,
          &MainWindow::on_preset_list_double_clicked);

  connect(m_break_enabled, &QCheckBox::toggled, this,
          &MainWindow::on_break_settings_changed);
  connect(m_work_minutes, &QSpinBox::valueChanged, this,
          &MainWindow::on_break_settings_changed);
  connect(m_break_minutes, &QSpinBox::valueChanged, this,
          &MainWindow::on_break_settings_changed);
  connect(m_break_message, &QLineEdit::editingFinished, this,
          &MainWindow::on_break_settings_changed);
  connect(m_sync_brightness, &QCheckBox::toggled, this, [this] {
    if (!m_updating)
      save_settings();
  });
  connect(m_sync_night, &QCheckBox::toggled, this, [this] {
    if (!m_updating)
      save_settings();
  });
}

void MainWindow::setup_tray() {
  QPixmap pix(22, 22);
  pix.fill(Qt::transparent);
  {
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(255, 180, 30));
    p.setPen(Qt::NoPen);
    p.drawEllipse(3, 3, 16, 16);
  }

  m_tray_icon = new QSystemTrayIcon(QIcon(pix), this);
  m_tray_icon->setToolTip("Screen Light Adjust");
  setWindowIcon(QIcon(pix));

  auto *tray_menu = new QMenu(this);
  tray_menu->addAction("白天模式 (6500K)", this, &MainWindow::set_day_mode);
  tray_menu->addAction("护眼模式 (3500K)", this, &MainWindow::set_night_mode);
  tray_menu->addSeparator();
  tray_menu->addAction("显示/隐藏", this, [this] { setVisible(!isVisible()); });
  tray_menu->addSeparator();
  tray_menu->addAction("退出", qApp, &QApplication::quit);

  m_tray_icon->setContextMenu(tray_menu);
  connect(m_tray_icon, &QSystemTrayIcon::activated, this,
          &MainWindow::on_tray_activated);
  m_tray_icon->show();

  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    statusBar()->showMessage(
        QStringLiteral("当前桌面未提供系统托盘服务；关闭窗口会退出程序。"));
  }
}

void MainWindow::setup_break_scheduler() {
  m_timer =
      new BreakTimer(m_work_minutes->value(), m_break_minutes->value(), this);
  m_break_reminder = new BreakReminder(this);

  connect(m_timer, &BreakTimer::work_period_finished, this,
          &MainWindow::on_work_period_finished);
  connect(m_timer, &BreakTimer::break_period_finished, this,
          &MainWindow::on_break_period_finished);
  connect(m_timer, &BreakTimer::remaining_time_changed, this,
          &MainWindow::update_countdown);

  if (m_break_enabled->isChecked())
    m_timer->start_work();
  else
    m_countdown_label->setText(QStringLiteral("休息提醒已暂停"));
}

// ============================================================
//  Window Events
// ============================================================

void MainWindow::closeEvent(QCloseEvent *event) {
  if (m_tray_icon && m_tray_icon->isVisible() &&
      QSystemTrayIcon::isSystemTrayAvailable()) {
    hide();
    event->ignore();
  } else {
    event->accept();
  }
}

void MainWindow::on_break_settings_changed() {
  if (m_updating)
    return;

  if (m_timer) {
    m_timer->set_durations(m_work_minutes->value(), m_break_minutes->value());
    if (m_break_enabled->isChecked()) {
      m_break_reminder->close_break();
      m_timer->start_work();
    } else {
      m_timer->stop();
      m_break_reminder->close_break();
      m_countdown_label->setText(QStringLiteral("休息提醒已暂停"));
    }
  }
  save_settings();
}

void MainWindow::on_work_period_finished(int break_minutes) {
  Q_UNUSED(break_minutes);
  if (!m_break_enabled->isChecked())
    return;

  m_break_reminder->show_break_with_message(m_break_minutes->value(),
                                            m_break_message->text());
  m_timer->start_break();
}

void MainWindow::on_break_period_finished() {
  m_break_reminder->close_break();
  if (m_break_enabled->isChecked())
    m_timer->start_work();
}

void MainWindow::update_countdown(const QTime &remaining, bool is_working) {
  const auto phase = is_working ? QStringLiteral("距离休息还有")
                                : QStringLiteral("休息结束还剩");
  m_countdown_label->setText(
      QStringLiteral("%1 %2").arg(phase, remaining.toString("hh:mm:ss")));
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger ||
      reason == QSystemTrayIcon::DoubleClick) {
    setVisible(!isVisible());
    if (isVisible()) {
      raise();
      activateWindow();
    }
  }
}

// ============================================================
//  Mode Presets (tray)
// ============================================================

void MainWindow::set_day_mode() {
  for (int i = 0; i < m_monitors.size(); ++i)
    m_monitors[i].night_mode->setChecked(false);
}

void MainWindow::set_night_mode() {
  for (int i = 0; i < m_monitors.size(); ++i) {
    m_monitors[i].night_mode->setChecked(true);
    m_monitors[i].warmth_slider->setValue(3500);
  }
}

// ============================================================
//  Signal Handlers
// ============================================================

void MainWindow::on_brightness_changed(int idx, int value) {
  if (m_updating)
    return;
  m_updating = true;

  if (m_sync_brightness->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i) {
      if (i == idx)
        continue;
      m_monitors[i].brightness_slider->blockSignals(true);
      m_monitors[i].brightness_slider->setValue(value);
      m_monitors[i].brightness_slider->blockSignals(false);
      m_monitors[i].brightness_spin->blockSignals(true);
      m_monitors[i].brightness_spin->setValue(value);
      m_monitors[i].brightness_spin->blockSignals(false);
    }
  }

  if (m_sync_brightness->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i)
      apply_monitor(i);
  } else {
    apply_monitor(idx);
  }

  m_updating = false;
  save_settings();
}

void MainWindow::on_night_mode_toggled(int idx, bool checked) {
  if (m_updating)
    return;
  m_updating = true;

  m_monitors[idx].warmth_container->setVisible(checked);

  if (m_sync_night->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i) {
      if (i == idx)
        continue;
      m_monitors[i].night_mode->blockSignals(true);
      m_monitors[i].night_mode->setChecked(checked);
      m_monitors[i].night_mode->blockSignals(false);
      m_monitors[i].warmth_container->setVisible(checked);
    }
  }

  if (m_sync_night->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i)
      apply_monitor(i);
  } else {
    apply_monitor(idx);
  }

  m_updating = false;
  save_settings();
}

void MainWindow::on_warmth_changed(int idx, int value) {
  if (m_updating)
    return;
  m_updating = true;

  m_monitors[idx].warmth_spin->setValue(value);

  if (m_sync_night->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i) {
      if (i == idx)
        continue;
      m_monitors[i].warmth_slider->blockSignals(true);
      m_monitors[i].warmth_slider->setValue(value);
      m_monitors[i].warmth_slider->blockSignals(false);
      m_monitors[i].warmth_spin->blockSignals(true);
      m_monitors[i].warmth_spin->setValue(value);
      m_monitors[i].warmth_spin->blockSignals(false);
    }
  }

  if (m_sync_night->isChecked()) {
    for (int i = 0; i < m_monitors.size(); ++i)
      apply_monitor(i);
  } else {
    apply_monitor(idx);
  }

  m_updating = false;
  save_settings();
}

// ============================================================
//  Apply
// ============================================================

void MainWindow::apply_monitor(int idx) {
  auto &mw = m_monitors[idx];
  double b = mw.brightness_slider->value() / 100.0;
  const int temperature =
      mw.night_mode->isChecked() ? mw.warmth_slider->value() : 6500;
  QString error;
  if (!MonitorController::apply(mw.output_name, b, temperature, &error))
    statusBar()->showMessage(QStringLiteral("显示设置未应用：%1").arg(error),
                             5000);
}

// ============================================================
//  Settings Persistence
// ============================================================

void MainWindow::load_settings() {
  m_settings.load();

  // Suppress signal-triggered save_settings() during initialization
  m_updating = true;

  m_sync_brightness->setChecked(m_settings.sync_brightness());
  m_sync_night->setChecked(m_settings.sync_night());
  m_break_enabled->setChecked(m_settings.break_enabled());
  m_work_minutes->setValue(m_settings.work_minutes());
  m_break_minutes->setValue(m_settings.break_minutes());
  m_break_message->setText(m_settings.break_message());

  auto monitors = MonitorController::discover();
  for (int i = 0; i < m_monitors.size(); ++i) {
    auto &mw = m_monitors[i];
    const auto &name = mw.output_name;

    // Check if this monitor has saved settings
    bool found = false;
    for (const auto &ms : m_settings.monitors()) {
      if (ms.name == name) {
        found = true;
        mw.brightness_slider->setValue(ms.brightness);
        mw.night_mode->setChecked(ms.night_mode);
        mw.warmth_container->setVisible(ms.night_mode);
        if (ms.night_mode) {
          mw.warmth_slider->setValue(ms.warmth);
        }
        break;
      }
    }

    if (!found) {
      // Use current xrandr state
      auto mi =
          std::find_if(monitors.begin(), monitors.end(),
                       [&](const MonitorInfo &m) { return m.name == name; });
      if (mi != monitors.end()) {
        int br = static_cast<int>(mi->brightness * 100.0);
        br = std::clamp(br, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        mw.brightness_slider->setValue(br);

        uint16_t k = estimate_kelvin(mi->gamma_r, mi->gamma_g, mi->gamma_b);
        if (k != 6500) {
          mw.night_mode->setChecked(true);
          mw.warmth_container->setVisible(true);
          mw.warmth_slider->setValue(static_cast<int>(k));
        }
      }
    }
  }

  // Window geometry
  auto geo = m_settings.window_geometry();
  if (!geo.isEmpty())
    restoreGeometry(geo);

  QString def = m_settings.default_preset();
  bool default_applied = false;
  if (!def.isEmpty()) {
    auto preset = m_presets.load(def);
    if (!preset.isEmpty()) {
      m_updating = false; // apply_preset sets its own guard
      apply_preset(preset);
      default_applied = true;
    } else {
      m_settings.set_default_preset(QString());
    }
  }

  m_updating = false;
  refresh_preset_list();

  if (!default_applied) {
    for (int i = 0; i < m_monitors.size(); ++i)
      apply_monitor(i);
  }

  // Persist UI state once after full initialization
  save_settings();
}

void MainWindow::save_settings() {
  // Sync current UI state into SettingsManager
  m_settings.set_sync_brightness(m_sync_brightness->isChecked());
  m_settings.set_sync_night(m_sync_night->isChecked());
  m_settings.set_break_enabled(m_break_enabled->isChecked());
  m_settings.set_work_minutes(m_work_minutes->value());
  m_settings.set_break_minutes(m_break_minutes->value());
  m_settings.set_break_message(m_break_message->text().trimmed());

  auto &monitors = m_settings.monitors();
  monitors.clear();
  for (const auto &mw : m_monitors) {
    MonitorSettings ms;
    ms.name = mw.output_name;
    ms.brightness = mw.brightness_slider->value();
    ms.night_mode = mw.night_mode->isChecked();
    if (ms.night_mode)
      ms.warmth = mw.warmth_slider->value();
    monitors.append(ms);
  }

  m_settings.set_window_geometry(saveGeometry());
  m_settings.save();
}

// ============================================================
//  Preset Management
// ============================================================

void MainWindow::refresh_preset_list() {
  m_preset_list->clear();
  QString def = m_settings.default_preset();
  auto names = m_presets.list_presets();
  for (const auto &name : names) {
    auto display = (name == def) ? QString("%1  (默认)").arg(name) : name;
    auto *item = new QListWidgetItem(display);
    if (name == def) {
      QFont f = item->font();
      f.setBold(true);
      item->setFont(f);
    }
    item->setData(Qt::UserRole, name); // store real name for operations
    m_preset_list->addItem(item);
  }
}

void MainWindow::save_current_as_preset() {
  QString name = m_preset_name_edit->text().trimmed();
  if (name.isEmpty()) {
    QMessageBox::information(this, "提示", "请输入预设名称");
    return;
  }

  // Build JSON from current UI state
  QJsonObject preset;
  preset["name"] = name;
  preset["sync_brightness"] = m_sync_brightness->isChecked();
  preset["sync_night"] = m_sync_night->isChecked();

  QJsonObject break_settings;
  break_settings["enabled"] = m_break_enabled->isChecked();
  break_settings["work_minutes"] = m_work_minutes->value();
  break_settings["break_minutes"] = m_break_minutes->value();
  break_settings["message"] = m_break_message->text().trimmed();
  preset["break"] = break_settings;

  QJsonArray arr;
  for (const auto &mw : m_monitors) {
    QJsonObject obj;
    obj["name"] = mw.output_name;
    obj["brightness"] = mw.brightness_slider->value();
    obj["night_mode"] = mw.night_mode->isChecked();
    if (mw.night_mode->isChecked())
      obj["warmth"] = mw.warmth_slider->value();
    arr.append(obj);
  }
  preset["monitors"] = arr;

  if (m_presets.save(name, preset)) {
    m_preset_name_edit->clear();
    refresh_preset_list();
  } else {
    QMessageBox::warning(
        this, QStringLiteral("保存失败"),
        QStringLiteral("预设名称不能包含路径分隔符，或预设文件无法写入。"));
  }
}

void MainWindow::load_selected_preset() {
  auto items = m_preset_list->selectedItems();
  if (items.isEmpty())
    return;

  QString name = items[0]->data(Qt::UserRole).toString();
  auto preset = m_presets.load(name);
  if (preset.isEmpty())
    return;

  apply_preset(preset);
  save_settings();
}

void MainWindow::on_preset_list_double_clicked(QListWidgetItem *item) {
  QString name = item->data(Qt::UserRole).toString();
  auto preset = m_presets.load(name);
  if (preset.isEmpty())
    return;

  apply_preset(preset);
  save_settings();
}

void MainWindow::delete_selected_preset() {
  auto items = m_preset_list->selectedItems();
  if (items.isEmpty())
    return;

  QString name = items[0]->data(Qt::UserRole).toString();
  auto reply = QMessageBox::question(
      this, "确认删除", QString("确定要删除预设 \"%1\" 吗？").arg(name),
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    m_presets.remove(name);
    if (m_settings.default_preset() == name)
      m_settings.set_default_preset(QString());
    refresh_preset_list();
    save_settings();
  }
}

void MainWindow::set_selected_as_default() {
  auto items = m_preset_list->selectedItems();
  if (items.isEmpty())
    return;

  QString name = items[0]->data(Qt::UserRole).toString();
  m_settings.set_default_preset(name);
  refresh_preset_list();
  save_settings();

  auto status = statusBar();
  if (status)
    status->showMessage(QString("已将 \"%1\" 设为默认预设").arg(name), 3000);
}

void MainWindow::apply_preset(const QJsonObject &preset) {
  m_updating = true;

  m_sync_brightness->setChecked(preset["sync_brightness"].toBool(false));
  m_sync_night->setChecked(preset["sync_night"].toBool(false));

  set_break_controls(preset.value("break").toObject());

  auto arr = preset["monitors"].toArray();
  for (const auto &val : arr) {
    auto obj = val.toObject();
    auto mon_name = obj["name"].toString();
    for (int i = 0; i < m_monitors.size(); ++i) {
      if (m_monitors[i].output_name != mon_name)
        continue;

      auto &mw = m_monitors[i];
      mw.brightness_slider->setValue(obj["brightness"].toInt(100));

      bool night = obj["night_mode"].toBool(false);
      mw.night_mode->setChecked(night);
      mw.warmth_container->setVisible(night);

      if (night) {
        mw.warmth_slider->setValue(obj["warmth"].toInt(4500));
      }
      break;
    }
  }

  m_updating = false;

  for (int i = 0; i < m_monitors.size(); ++i)
    apply_monitor(i);
  on_break_settings_changed();
}

void MainWindow::set_break_controls(const QJsonObject &settings) {
  if (settings.contains("enabled"))
    m_break_enabled->setChecked(settings.value("enabled").toBool(true));
  if (settings.contains("work_minutes"))
    m_work_minutes->setValue(
        settings.value("work_minutes").toInt(m_work_minutes->value()));
  if (settings.contains("break_minutes"))
    m_break_minutes->setValue(
        settings.value("break_minutes").toInt(m_break_minutes->value()));
  if (settings.contains("message"))
    m_break_message->setText(settings.value("message").toString());
}

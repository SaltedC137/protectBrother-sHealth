#include "mainwindow.h"
#include "color_temp.h"
#include "monitor_ctrl.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QStatusBar>
#include <QVBoxLayout>

static constexpr int BRIGHTNESS_MIN = 10;
static constexpr int BRIGHTNESS_MAX = 200;
static constexpr int WARMTH_MIN     = 1000;
static constexpr int WARMTH_MAX     = 7500;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    setWindowTitle("Screen Light Adjust");
    setMinimumWidth(400);

    auto monitors = MonitorController::discover();
    if (monitors.isEmpty()) {
        auto* label = new QLabel("未检测到显示器，请确认 xrandr 可用");
        label->setAlignment(Qt::AlignCenter);
        setCentralWidget(label);
        setup_tray();
        return;
    }

    for (const auto& mi : monitors) {
        MonitorWidgets mw;
        mw.output_name = mi.name;
        m_monitors.append(mw);
    }

    setup_ui();
    setup_tray();
    load_settings();

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::save_settings);
}

// ============================================================
//  UI Setup
// ============================================================

void MainWindow::setup_ui() {
    auto* central     = new QWidget(this);
    auto* main_layout = new QVBoxLayout(central);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(8);

    m_tab_widget = new QTabWidget;

    // ---------- Tab 1: Display ----------
    auto* display_tab   = new QWidget;
    auto* display_layout = new QVBoxLayout(display_tab);
    display_layout->setContentsMargins(0, 8, 0, 0);
    display_layout->setSpacing(8);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* scroll_content  = new QWidget;
    auto* monitors_layout = new QVBoxLayout(scroll_content);
    monitors_layout->setSpacing(10);

    for (int i = 0; i < m_monitors.size(); ++i) {
        auto* mw = &m_monitors[i];

        auto* group = new QGroupBox;
        auto* gl    = new QVBoxLayout(group);
        gl->setSpacing(6);

        // Monitor name
        auto* name      = new QLabel(mw->output_name);
        QFont name_font = name->font();
        name_font.setBold(true);
        name_font.setPointSize(name_font.pointSize() + 1);
        name->setFont(name_font);
        gl->addWidget(name);

        // Brightness row
        auto* br_row   = new QHBoxLayout;
        auto* br_label = new QLabel("亮度");
        br_label->setFixedWidth(36);
        auto* br_slider = new QSlider(Qt::Horizontal);
        br_slider->setRange(BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        br_slider->setValue(100);
        auto* br_val = new QLabel("100%");
        br_val->setFixedWidth(42);
        br_val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        br_row->addWidget(br_label);
        br_row->addWidget(br_slider, 1);
        br_row->addWidget(br_val);
        gl->addLayout(br_row);

        // Night mode toggle
        auto* night = new QCheckBox("护眼模式");
        gl->addWidget(night);

        // Warmth row (hidden until night mode on)
        auto* wm_row   = new QHBoxLayout;
        auto* wm_label = new QLabel("色温");
        wm_label->setFixedWidth(36);
        auto* wm_slider = new QSlider(Qt::Horizontal);
        wm_slider->setRange(WARMTH_MIN, WARMTH_MAX);
        wm_slider->setSingleStep(50);
        wm_slider->setPageStep(500);
        wm_slider->setValue(4500);
        auto* wm_val = new QLabel("4500K");
        wm_val->setFixedWidth(56);
        wm_val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wm_row->addWidget(wm_label);
        wm_row->addWidget(wm_slider, 1);
        wm_row->addWidget(wm_val);

        auto* wm_container = new QWidget;
        wm_container->setLayout(wm_row);
        wm_container->setVisible(false);
        gl->addWidget(wm_container);

        monitors_layout->addWidget(group);

        mw->name_label        = name;
        mw->brightness_slider = br_slider;
        mw->brightness_val    = br_val;
        mw->night_mode        = night;
        mw->warmth_slider     = wm_slider;
        mw->warmth_val        = wm_val;
        mw->warmth_container  = wm_container;

        connect(br_slider, &QSlider::valueChanged, this, [this, i](int v) { on_brightness_changed(i, v); });
        connect(night, &QCheckBox::toggled, this, [this, i](bool c) { on_night_mode_toggled(i, c); });
        connect(wm_slider, &QSlider::valueChanged, this, [this, i](int v) { on_warmth_changed(i, v); });
    }

    scroll->setWidget(scroll_content);
    display_layout->addWidget(scroll, 1);

    // Separator
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    display_layout->addWidget(sep);

    // Sync toggles
    auto* sync_row  = new QHBoxLayout;
    m_sync_brightness = new QCheckBox("同步亮度");
    m_sync_night      = new QCheckBox("同步护眼模式");
    sync_row->addWidget(m_sync_brightness);
    sync_row->addStretch();
    sync_row->addWidget(m_sync_night);

    auto* sync_container = new QWidget;
    sync_container->setLayout(sync_row);
    display_layout->addWidget(sync_container);

    m_tab_widget->addTab(display_tab, "显示");

    // ---------- Tab 2: Presets ----------
    auto* presets_tab   = new QWidget;
    auto* presets_layout = new QVBoxLayout(presets_tab);
    presets_layout->setContentsMargins(8, 8, 8, 8);
    presets_layout->setSpacing(8);

    auto* preset_title = new QLabel("预设管理");
    QFont title_font = preset_title->font();
    title_font.setBold(true);
    title_font.setPointSize(title_font.pointSize() + 1);
    preset_title->setFont(title_font);
    presets_layout->addWidget(preset_title);

    m_preset_list = new QListWidget;
    m_preset_list->setAlternatingRowColors(true);
    presets_layout->addWidget(m_preset_list, 1);

    // Save as preset
    auto* save_row = new QHBoxLayout;
    m_preset_name_edit = new QLineEdit;
    m_preset_name_edit->setPlaceholderText("输入预设名称...");
    auto* save_btn = new QPushButton("保存当前为预设");
    save_row->addWidget(m_preset_name_edit, 1);
    save_row->addWidget(save_btn);
    presets_layout->addLayout(save_row);

    // Load / Delete / Set default
    auto* action_row = new QHBoxLayout;
    auto* load_btn   = new QPushButton("加载预设");
    auto* del_btn    = new QPushButton("删除预设");
    auto* def_btn    = new QPushButton("设为默认预设");
    action_row->addWidget(load_btn);
    action_row->addWidget(del_btn);
    action_row->addWidget(def_btn);
    presets_layout->addLayout(action_row);

    m_load_default_preset = new QCheckBox("启动时自动加载默认预设");
    presets_layout->addWidget(m_load_default_preset);

    presets_layout->addStretch();

    m_tab_widget->addTab(presets_tab, "设置");

    main_layout->addWidget(m_tab_widget, 1);

    // Hint
    auto* hint = new QLabel("提示：关闭窗口会最小化到系统托盘");
    hint->setAlignment(Qt::AlignCenter);
    QFont hint_font = hint->font();
    hint_font.setPointSize(hint_font.pointSize() - 1);
    hint->setFont(hint_font);
    hint->setStyleSheet("color: gray;");
    main_layout->addWidget(hint);

    setCentralWidget(central);

    // Connections for preset tab
    connect(save_btn, &QPushButton::clicked, this, &MainWindow::save_current_as_preset);
    connect(load_btn, &QPushButton::clicked, this, &MainWindow::load_selected_preset);
    connect(del_btn, &QPushButton::clicked, this, &MainWindow::delete_selected_preset);
    connect(def_btn, &QPushButton::clicked, this, &MainWindow::set_selected_as_default);
    connect(m_preset_list, &QListWidget::itemDoubleClicked, this, &MainWindow::on_preset_list_double_clicked);
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

    auto* tray_menu = new QMenu(this);
    tray_menu->addAction("白天模式 (6500K)", this, &MainWindow::set_day_mode);
    tray_menu->addAction("护眼模式 (3500K)", this, &MainWindow::set_night_mode);
    tray_menu->addSeparator();
    tray_menu->addAction("显示/隐藏", this, [this] { setVisible(!isVisible()); });
    tray_menu->addSeparator();
    tray_menu->addAction("退出", qApp, &QApplication::quit);

    m_tray_icon->setContextMenu(tray_menu);
    connect(m_tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::on_tray_activated);
    m_tray_icon->show();
}

// ============================================================
//  Window Events
// ============================================================

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_tray_icon && m_tray_icon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
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

    m_monitors[idx].brightness_val->setText(QString("%1%").arg(value));

    if (m_sync_brightness->isChecked()) {
        for (int i = 0; i < m_monitors.size(); ++i) {
            if (i == idx)
                continue;
            m_monitors[i].brightness_slider->blockSignals(true);
            m_monitors[i].brightness_slider->setValue(value);
            m_monitors[i].brightness_slider->blockSignals(false);
            m_monitors[i].brightness_val->setText(QString("%1%").arg(value));
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

    m_monitors[idx].warmth_val->setText(QString("%1K").arg(value));

    if (m_sync_night->isChecked()) {
        for (int i = 0; i < m_monitors.size(); ++i) {
            if (i == idx)
                continue;
            m_monitors[i].warmth_slider->blockSignals(true);
            m_monitors[i].warmth_slider->setValue(value);
            m_monitors[i].warmth_slider->blockSignals(false);
            m_monitors[i].warmth_val->setText(QString("%1K").arg(value));
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
    auto&  mw = m_monitors[idx];
    double b  = mw.brightness_slider->value() / 100.0;

    if (mw.night_mode->isChecked()) {
        int  k   = mw.warmth_slider->value();
        auto rgb = kelvin_to_rgb(static_cast<uint16_t>(k));
        MonitorController::apply(mw.output_name, b, rgb.r, rgb.g, rgb.b);
    } else {
        MonitorController::apply(mw.output_name, b, 1.0, 1.0, 1.0);
    }
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

    auto monitors = MonitorController::discover();
    for (int i = 0; i < m_monitors.size(); ++i) {
        auto&       mw   = m_monitors[i];
        const auto& name = mw.output_name;

        // Check if this monitor has saved settings
        bool found = false;
        for (const auto& ms : m_settings.monitors()) {
            if (ms.name == name) {
                found = true;
                mw.brightness_slider->setValue(ms.brightness);
                mw.brightness_val->setText(QString("%1%").arg(ms.brightness));
                mw.night_mode->setChecked(ms.night_mode);
                mw.warmth_container->setVisible(ms.night_mode);
                if (ms.night_mode) {
                    mw.warmth_slider->setValue(ms.warmth);
                    mw.warmth_val->setText(QString("%1K").arg(ms.warmth));
                }
                break;
            }
        }

        if (!found) {
            // Use current xrandr state
            auto mi = std::find_if(
                monitors.begin(), monitors.end(), [&](const MonitorInfo& m) { return m.name == name; });
            if (mi != monitors.end()) {
                int br = static_cast<int>(mi->brightness * 100.0);
                br     = std::clamp(br, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
                mw.brightness_slider->setValue(br);
                mw.brightness_val->setText(QString("%1%").arg(br));

                uint16_t k = estimate_kelvin(mi->gamma_r, mi->gamma_g, mi->gamma_b);
                if (k != 6500) {
                    mw.night_mode->setChecked(true);
                    mw.warmth_container->setVisible(true);
                    mw.warmth_slider->setValue(static_cast<int>(k));
                    mw.warmth_val->setText(QString("%1K").arg(k));
                }
            }
        }
    }

    // Window geometry
    auto geo = m_settings.window_geometry();
    if (!geo.isEmpty())
        restoreGeometry(geo);

    // Default preset: restore checkbox state first, then apply if needed
    m_load_default_preset->setChecked(m_settings.auto_load_default());

    QString def = m_settings.default_preset();
    if (!def.isEmpty() && m_settings.auto_load_default()) {
        auto preset = m_presets.load(def);
        if (!preset.isEmpty()) {
            m_updating = false; // apply_preset sets its own guard
            apply_preset(preset);
        }
    }

    m_updating = false;
    refresh_preset_list();

    // Persist UI state once after full initialization
    save_settings();
}

void MainWindow::save_settings() {
    // Sync current UI state into SettingsManager
    m_settings.set_sync_brightness(m_sync_brightness->isChecked());
    m_settings.set_sync_night(m_sync_night->isChecked());

    auto& monitors = m_settings.monitors();
    monitors.clear();
    for (const auto& mw : m_monitors) {
        MonitorSettings ms;
        ms.name       = mw.output_name;
        ms.brightness = mw.brightness_slider->value();
        ms.night_mode = mw.night_mode->isChecked();
        if (ms.night_mode)
            ms.warmth = mw.warmth_slider->value();
        monitors.append(ms);
    }

    m_settings.set_window_geometry(saveGeometry());
    m_settings.set_auto_load_default(m_load_default_preset->isChecked());
    m_settings.save();
}

// ============================================================
//  Preset Management
// ============================================================

void MainWindow::refresh_preset_list() {
    m_preset_list->clear();
    QString def = m_settings.default_preset();
    auto names = m_presets.list_presets();
    for (const auto& name : names) {
        auto display = (name == def) ? QString("%1  (默认)").arg(name) : name;
        auto* item = new QListWidgetItem(display);
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
    preset["name"]           = name;
    preset["sync_brightness"] = m_sync_brightness->isChecked();
    preset["sync_night"]      = m_sync_night->isChecked();

    QJsonArray arr;
    for (const auto& mw : m_monitors) {
        QJsonObject obj;
        obj["name"]       = mw.output_name;
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
    if (m_load_default_preset->isChecked())
        m_settings.set_default_preset(name);
    save_settings();
}

void MainWindow::on_preset_list_double_clicked(QListWidgetItem* item) {
    QString name = item->data(Qt::UserRole).toString();
    auto preset = m_presets.load(name);
    if (preset.isEmpty())
        return;

    apply_preset(preset);
    if (m_load_default_preset->isChecked())
        m_settings.set_default_preset(name);
    save_settings();
}

void MainWindow::delete_selected_preset() {
    auto items = m_preset_list->selectedItems();
    if (items.isEmpty())
        return;

    QString name = items[0]->data(Qt::UserRole).toString();
    auto reply = QMessageBox::question(this, "确认删除",
        QString("确定要删除预设 \"%1\" 吗？").arg(name),
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
    m_load_default_preset->setChecked(true);
    refresh_preset_list();
    save_settings();

    auto status = statusBar();
    if (status)
        status->showMessage(QString("已将 \"%1\" 设为默认预设").arg(name), 3000);
}

void MainWindow::apply_preset(const QJsonObject& preset) {
    m_updating = true;

    m_sync_brightness->setChecked(preset["sync_brightness"].toBool(false));
    m_sync_night->setChecked(preset["sync_night"].toBool(false));

    auto arr = preset["monitors"].toArray();
    for (const auto& val : arr) {
        auto obj      = val.toObject();
        auto mon_name = obj["name"].toString();
        for (int i = 0; i < m_monitors.size(); ++i) {
            if (m_monitors[i].output_name != mon_name)
                continue;

            auto& mw = m_monitors[i];
            mw.brightness_slider->setValue(obj["brightness"].toInt(100));
            mw.brightness_val->setText(QString("%1%").arg(mw.brightness_slider->value()));

            bool night = obj["night_mode"].toBool(false);
            mw.night_mode->setChecked(night);
            mw.warmth_container->setVisible(night);

            if (night) {
                mw.warmth_slider->setValue(obj["warmth"].toInt(4500));
                mw.warmth_val->setText(QString("%1K").arg(mw.warmth_slider->value()));
            }
            break;
        }
    }

    m_updating = false;

    for (int i = 0; i < m_monitors.size(); ++i)
        apply_monitor(i);
}

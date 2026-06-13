#include "mainwindow.h"
#include "color_temp.h"
#include "monitor_ctrl.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

static constexpr int BRIGHTNESS_MIN = 10;
static constexpr int BRIGHTNESS_MAX = 200;
static constexpr int WARMTH_MIN		= 1000;
static constexpr int WARMTH_MAX		= 7500;

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent) {

	setWindowTitle("Screen Light Adjust");
	setMinimumWidth(380);

	auto monitors = MonitorController::discover();
	if(monitors.isEmpty()) {
		auto* label = new QLabel("未检测到显示器，请确认 xrandr 可用");
		label->setAlignment(Qt::AlignCenter);
		setCentralWidget(label);
		setup_tray();
		return;
	}

	// Build UI widgets for each monitor
	for(const auto& mi : monitors) {
		MonitorWidgets mw;
		mw.output_name = mi.name;
		m_monitors.append(mw);
	}

	setup_ui();
	setup_tray();
	load_settings();
}

void MainWindow::setup_ui() {
	auto* central	  = new QWidget(this);
	auto* main_layout = new QVBoxLayout(central);
	main_layout->setContentsMargins(12, 12, 12, 12);
	main_layout->setSpacing(8);

	// Scroll area for monitors
	auto* scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	auto* scroll_content  = new QWidget;
	auto* monitors_layout = new QVBoxLayout(scroll_content);
	monitors_layout->setSpacing(10);

	for(int i = 0; i < m_monitors.size(); ++i) {
		auto* mw	= &m_monitors[i];

		auto* group = new QGroupBox;
		auto* gl	= new QVBoxLayout(group);
		gl->setSpacing(6);

		// Monitor name
		auto* name		= new QLabel(mw->output_name);
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

		// Store widgets
		mw->name_label		  = name;
		mw->brightness_slider = br_slider;
		mw->brightness_val	  = br_val;
		mw->night_mode		  = night;
		mw->warmth_slider	  = wm_slider;
		mw->warmth_val		  = wm_val;
		mw->warmth_container  = wm_container;

		// Connections
		connect(br_slider, &QSlider::valueChanged, this, [this, i](int v) { on_brightness_changed(i, v); });
		connect(night, &QCheckBox::toggled, this, [this, i](bool c) { on_night_mode_toggled(i, c); });
		connect(wm_slider, &QSlider::valueChanged, this, [this, i](int v) { on_warmth_changed(i, v); });
	}

	scroll->setWidget(scroll_content);
	main_layout->addWidget(scroll, 1);

	// Separator
	auto* sep = new QFrame;
	sep->setFrameShape(QFrame::HLine);
	sep->setFrameShadow(QFrame::Sunken);
	main_layout->addWidget(sep);

	// Sync toggles
	auto* sync_row	  = new QHBoxLayout;
	m_sync_brightness = new QCheckBox("同步亮度");
	m_sync_night	  = new QCheckBox("同步护眼模式");
	sync_row->addWidget(m_sync_brightness);
	sync_row->addStretch();
	sync_row->addWidget(m_sync_night);

	auto* sync_container = new QWidget;
	sync_container->setLayout(sync_row);
	main_layout->addWidget(sync_container);

	// Hint label
	auto* hint = new QLabel("提示：关闭窗口会最小化到系统托盘");
	hint->setAlignment(Qt::AlignCenter);
	QFont hint_font = hint->font();
	hint_font.setPointSize(hint_font.pointSize() - 1);
	hint->setFont(hint_font);
	hint->setStyleSheet("color: gray;");
	main_layout->addWidget(hint);

	setCentralWidget(central);
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

void MainWindow::closeEvent(QCloseEvent* event) {
	if(m_tray_icon && m_tray_icon->isVisible()) {
		hide();
		event->ignore();
	} else {
		event->accept();
	}
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
	if(reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
		setVisible(!isVisible());
		if(isVisible()) {
			raise();
			activateWindow();
		}
	}
}

void MainWindow::set_day_mode() {
	for(int i = 0; i < m_monitors.size(); ++i)
		m_monitors[i].night_mode->setChecked(false);
}

void MainWindow::set_night_mode() {
	for(int i = 0; i < m_monitors.size(); ++i) {
		m_monitors[i].night_mode->setChecked(true);
		m_monitors[i].warmth_slider->setValue(3500);
	}
}

// --- Signal handlers ---

void MainWindow::on_brightness_changed(int idx, int value) {
	if(m_updating)
		return;
	m_updating = true;

	m_monitors[idx].brightness_val->setText(QString("%1%").arg(value));

	if(m_sync_brightness->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i) {
			if(i == idx)
				continue;
			m_monitors[i].brightness_slider->blockSignals(true);
			m_monitors[i].brightness_slider->setValue(value);
			m_monitors[i].brightness_slider->blockSignals(false);
			m_monitors[i].brightness_val->setText(QString("%1%").arg(value));
		}
	}

	if(m_sync_brightness->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i)
			apply_monitor(i);
	} else {
		apply_monitor(idx);
	}

	m_updating = false;
	save_settings();
}

void MainWindow::on_night_mode_toggled(int idx, bool checked) {
	if(m_updating)
		return;
	m_updating = true;

	m_monitors[idx].warmth_container->setVisible(checked);

	if(m_sync_night->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i) {
			if(i == idx)
				continue;
			m_monitors[i].night_mode->blockSignals(true);
			m_monitors[i].night_mode->setChecked(checked);
			m_monitors[i].night_mode->blockSignals(false);
			m_monitors[i].warmth_container->setVisible(checked);
		}
	}

	if(m_sync_night->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i)
			apply_monitor(i);
	} else {
		apply_monitor(idx);
	}

	m_updating = false;
	save_settings();
}

void MainWindow::on_warmth_changed(int idx, int value) {
	if(m_updating)
		return;
	m_updating = true;

	m_monitors[idx].warmth_val->setText(QString("%1K").arg(value));

	if(m_sync_night->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i) {
			if(i == idx)
				continue;
			m_monitors[i].warmth_slider->blockSignals(true);
			m_monitors[i].warmth_slider->setValue(value);
			m_monitors[i].warmth_slider->blockSignals(false);
			m_monitors[i].warmth_val->setText(QString("%1K").arg(value));
		}
	}

	if(m_sync_night->isChecked()) {
		for(int i = 0; i < m_monitors.size(); ++i)
			apply_monitor(i);
	} else {
		apply_monitor(idx);
	}

	m_updating = false;
	save_settings();
}

void MainWindow::apply_monitor(int idx) {
	auto&  mw = m_monitors[idx];
	double b  = mw.brightness_slider->value() / 100.0;

	if(mw.night_mode->isChecked()) {
		int	 k	 = mw.warmth_slider->value();
		auto rgb = kelvin_to_rgb(static_cast<uint16_t>(k));
		MonitorController::apply(mw.output_name, b, rgb.r, rgb.g, rgb.b);
	} else {
		MonitorController::apply(mw.output_name, b, 1.0, 1.0, 1.0);
	}
}

// --- Settings ---

void MainWindow::load_settings() {
	QSettings s("screen-light-adjust", "screen-light-adjust");
	m_sync_brightness->setChecked(s.value("sync/brightness", false).toBool());
	m_sync_night->setChecked(s.value("sync/night", false).toBool());

	auto monitors = MonitorController::discover();
	for(int i = 0; i < m_monitors.size(); ++i) {
		auto&		mw	 = m_monitors[i];
		const auto& name = mw.output_name;

		bool has_saved	 = s.contains(QString("monitor/%1/brightness").arg(name));

		if(has_saved) {
			int br = s.value(QString("monitor/%1/brightness").arg(name), 100).toInt();
			mw.brightness_slider->setValue(br);

			bool night = s.value(QString("monitor/%1/night_mode").arg(name), false).toBool();
			mw.night_mode->setChecked(night);

			if(night) {
				int warmth = s.value(QString("monitor/%1/warmth").arg(name), 4500).toInt();
				mw.warmth_slider->setValue(warmth);
			}
		} else {
			// Use current xrandr state
			auto mi = std::find_if(
				monitors.begin(), monitors.end(), [&](const MonitorInfo& m) { return m.name == name; });
			if(mi != monitors.end()) {
				int br = static_cast<int>(mi->brightness * 100.0);
				br	   = std::clamp(br, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
				mw.brightness_slider->setValue(br);

				uint16_t k = estimate_kelvin(mi->gamma_r, mi->gamma_g, mi->gamma_b);
				if(k != 6500) {
					mw.night_mode->setChecked(true);
					mw.warmth_slider->setValue(static_cast<int>(k));
				}
			}
		}
	}

	if(s.contains("window/geometry"))
		restoreGeometry(s.value("window/geometry").toByteArray());
}

void MainWindow::save_settings() {
	QSettings s("screen-light-adjust", "screen-light-adjust");
	s.setValue("sync/brightness", m_sync_brightness->isChecked());
	s.setValue("sync/night", m_sync_night->isChecked());

	for(auto& mw : m_monitors) {
		s.setValue(QString("monitor/%1/brightness").arg(mw.output_name), mw.brightness_slider->value());
		s.setValue(QString("monitor/%1/night_mode").arg(mw.output_name), mw.night_mode->isChecked());
		if(mw.night_mode->isChecked()) {
			s.setValue(QString("monitor/%1/warmth").arg(mw.output_name), mw.warmth_slider->value());
		}
	}

	s.setValue("window/geometry", saveGeometry());
}

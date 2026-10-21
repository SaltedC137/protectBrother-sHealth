#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

struct MonitorSettings {
	QString name;
	int		brightness = 100;
	bool	night_mode = false;
	int		warmth	   = 5000;
};

class SettingsManager {
  public:
	SettingsManager();

	void load();
	void save();

	// Monitor settings
	QVector<MonitorSettings>&		monitors();
	const QVector<MonitorSettings>& monitors() const;

	// Sync flags
	bool sync_brightness() const { return m_sync_brightness; }

	void set_sync_brightness(bool v) { m_sync_brightness = v; }

	bool sync_night() const { return m_sync_night; }

	void set_sync_night(bool v) { m_sync_night = v; }

	bool break_enabled() const { return m_break_enabled; }

	void set_break_enabled(bool v) { m_break_enabled = v; }

	int work_minutes() const { return m_work_minutes; }

	void set_work_minutes(int v) { m_work_minutes = v; }

	int break_minutes() const { return m_break_minutes; }

	void set_break_minutes(int v) { m_break_minutes = v; }

	QString break_message() const { return m_break_message; }

	void set_break_message(const QString& v) { m_break_message = v; }

	// Window geometry
	QByteArray window_geometry() const { return m_window_geometry; }

	void set_window_geometry(const QByteArray& g) { m_window_geometry = g; }

	// Default preset
	QString default_preset() const { return m_default_preset; }

	void set_default_preset(const QString& name) { m_default_preset = name; }

	// Config directory path
	static QString config_dir();

  private:
	QString					 m_file_path;
	QVector<MonitorSettings> m_monitors;
	bool					 m_sync_brightness = false;
	bool					 m_sync_night	   = false;
	bool					 m_break_enabled   = true;
	int						 m_work_minutes	   = 120;
	int						 m_break_minutes   = 20;
	QString					 m_break_message   = QStringLiteral("该休息一下了，眺望远方并活动身体。");
	QByteArray				 m_window_geometry;
	QString					 m_default_preset;
};

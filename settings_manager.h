#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

struct MonitorSettings {
    QString name;
    int     brightness = 100;
    bool    night_mode = false;
    int     warmth     = 4500;
};

class SettingsManager {
  public:
    SettingsManager();

    void load();
    void save();

    // Monitor settings
    QVector<MonitorSettings>&       monitors();
    const QVector<MonitorSettings>& monitors() const;

    // Sync flags
    bool sync_brightness() const { return m_sync_brightness; }
    void set_sync_brightness(bool v) { m_sync_brightness = v; }

    bool sync_night() const { return m_sync_night; }
    void set_sync_night(bool v) { m_sync_night = v; }

    // Window geometry
    QByteArray window_geometry() const { return m_window_geometry; }
    void       set_window_geometry(const QByteArray& g) { m_window_geometry = g; }

    // Default preset
    QString default_preset() const { return m_default_preset; }
    void    set_default_preset(const QString& name) { m_default_preset = name; }

    // Config directory path
    static QString config_dir();

  private:
    QString                    m_file_path;
    QVector<MonitorSettings>   m_monitors;
    bool                       m_sync_brightness = false;
    bool                       m_sync_night      = false;
    QByteArray                 m_window_geometry;
    QString                    m_default_preset;
};

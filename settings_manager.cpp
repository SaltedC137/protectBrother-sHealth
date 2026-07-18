#include "settings_manager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SettingsManager::SettingsManager() {
    auto dir = config_dir();
    QDir().mkpath(dir);
    m_file_path = dir + "/settings.json";
}

QString SettingsManager::config_dir() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/screen-light-adjust";
}

QVector<MonitorSettings>& SettingsManager::monitors() { return m_monitors; }
const QVector<MonitorSettings>& SettingsManager::monitors() const { return m_monitors; }

void SettingsManager::load() {
    QFile file(m_file_path);
    if (!file.open(QIODevice::ReadOnly))
        return; // First launch, use defaults

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return;

    auto root = doc.object();

    m_sync_brightness = root.value("sync_brightness").toBool(false);
    m_sync_night      = root.value("sync_night").toBool(false);
    m_default_preset  = root.value("default_preset").toString();
    m_auto_load_default = root.value("auto_load_default").toBool(false);

    auto geo = root.value("window_geometry").toString();
    if (!geo.isEmpty())
        m_window_geometry = QByteArray::fromBase64(geo.toLatin1());

    m_monitors.clear();
    auto arr = root.value("monitors").toArray();
    for (const auto& val : arr) {
        auto obj = val.toObject();
        MonitorSettings ms;
        ms.name        = obj.value("name").toString();
        ms.brightness  = obj.value("brightness").toInt(100);
        ms.night_mode  = obj.value("night_mode").toBool(false);
        ms.warmth      = obj.value("warmth").toInt(4500);
        m_monitors.append(ms);
    }
}

void SettingsManager::save() {
    QJsonArray arr;
    for (const auto& ms : m_monitors) {
        QJsonObject obj;
        obj["name"]       = ms.name;
        obj["brightness"] = ms.brightness;
        obj["night_mode"] = ms.night_mode;
        if (ms.night_mode)
            obj["warmth"] = ms.warmth;
        arr.append(obj);
    }

    QJsonObject root;
    root["version"]         = 1;
    root["sync_brightness"] = m_sync_brightness;
    root["sync_night"]      = m_sync_night;
    root["default_preset"]  = m_default_preset;
    root["auto_load_default"] = m_auto_load_default;
    root["monitors"]        = arr;

    if (!m_window_geometry.isEmpty())
        root["window_geometry"] = QString::fromLatin1(m_window_geometry.toBase64());

    QFile file(m_file_path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson());
}

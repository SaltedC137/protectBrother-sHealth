#include "preset_manager.h"
#include "settings_manager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>

PresetManager::PresetManager() {
    m_dir_path = SettingsManager::config_dir() + "/presets";
    QDir().mkpath(m_dir_path);
}

QString PresetManager::preset_file_path(const QString& name) const {
    return m_dir_path + "/" + name + ".json";
}

QStringList PresetManager::list_presets() {
    QDir dir(m_dir_path);
    auto entries = dir.entryList({"*.json"}, QDir::Files, QDir::Name);

    QStringList names;
    for (const auto& entry : entries)
        names.append(entry.chopped(5)); // remove ".json"
    return names;
}

bool PresetManager::save(const QString& name, const QJsonObject& data) {
    QFile file(preset_file_path(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(QJsonDocument(data).toJson()) > 0;
}

QJsonObject PresetManager::load(const QString& name) {
    QFile file(preset_file_path(name));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return {};
    return doc.object();
}

bool PresetManager::remove(const QString& name) {
    return QFile::remove(preset_file_path(name));
}

bool PresetManager::exists(const QString& name) {
    return QFile::exists(preset_file_path(name));
}

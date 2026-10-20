#include "screen_light_adjust/preset_manager.h"
#include "screen_light_adjust/settings_manager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

PresetManager::PresetManager() {
    m_dir_path = SettingsManager::config_dir() + "/presets";
    QDir().mkpath(m_dir_path);
}

QString PresetManager::preset_file_path(const QString& name) const {
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\'))
        return {};
    return QDir(m_dir_path).filePath(name + ".json");
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
    const auto path = preset_file_path(name);
    if (path.isEmpty())
        return false;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    const auto contents = QJsonDocument(data).toJson();
    return file.write(contents) == contents.size() && file.commit();
}

QJsonObject PresetManager::load(const QString& name) {
    const auto path = preset_file_path(name);
    if (path.isEmpty())
        return {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return {};
    return doc.object();
}

bool PresetManager::remove(const QString& name) {
    const auto path = preset_file_path(name);
    return !path.isEmpty() && QFile::remove(path);
}

bool PresetManager::exists(const QString& name) {
    const auto path = preset_file_path(name);
    return !path.isEmpty() && QFile::exists(path);
}

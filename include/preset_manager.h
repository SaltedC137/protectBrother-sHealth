#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>

class PresetManager {
  public:
	PresetManager();

	QStringList list_presets();
	bool		save(const QString& name, const QJsonObject& data);
	QJsonObject load(const QString& name);
	bool		remove(const QString& name);
	bool		exists(const QString& name);

	// Directory where presets are stored
	QString presets_dir() const { return m_dir_path; }

  private:
	QString preset_file_path(const QString& name) const;
	QString m_dir_path;
};

#pragma once

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QtGlobal>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QString>

class PlatformTray {
  public:
	static void prepare_platform_tray();
	// Call right after QApplication is constructed; only acts if
	// prepare_platform_tray() swapped away from the KDE platform theme.
	static void restore_kde_palette();
};

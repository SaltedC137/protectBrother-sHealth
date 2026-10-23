#include "platform_tray.h"
// VibeCoing: I don't understand, but this seems useful.
#if defined(SCREEN_LIGHT_ADJUST_HAVE_DBUS)
#	include <QDBusConnection>
#	include <QDBusMessage>
#	include <QDBusVariant>

namespace {

bool is_x11_session() {
	const QString platform = qEnvironmentVariable("QT_QPA_PLATFORM");
	if(platform.startsWith(QLatin1String("xcb")))
		return true;
	if(platform.contains(QLatin1String("wayland"), Qt::CaseInsensitive))
		return false;
	const QString session_type = qEnvironmentVariable("XDG_SESSION_TYPE").toLower();
	if(session_type == QLatin1String("wayland"))
		return false;
	if(session_type == QLatin1String("x11"))
		return true;
	if(!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
		return false;
	return !qEnvironmentVariableIsEmpty("DISPLAY");
}

bool uses_kde_platform_theme() {
	const QString theme = qEnvironmentVariable("QT_QPA_PLATFORMTHEME");
	if(!theme.isEmpty())
		return theme == QLatin1String("kde");
	return qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("KDE"), Qt::CaseInsensitive);
}

bool kde_theme_plugin_available() {
	const QString dir = QLibraryInfo::path(QLibraryInfo::PluginsPath) + QStringLiteral("/platformthemes/");
	return QFileInfo::exists(dir + QStringLiteral("KDEPlasmaPlatformTheme6.so"))
		   || QFileInfo::exists(dir + QStringLiteral("KDEPlasmaPlatformTheme.so"));
}

bool gtk3_theme_available() {
	return QFileInfo::exists(QLibraryInfo::path(QLibraryInfo::PluginsPath)
							 + QStringLiteral("/platformthemes/libqgtk3.so"));
}

bool breeze_style_available() {
	const QString dir = QLibraryInfo::path(QLibraryInfo::PluginsPath) + QStringLiteral("/styles/");
	return QFileInfo::exists(dir + QStringLiteral("breeze6.so"))
		   || QFileInfo::exists(dir + QStringLiteral("breeze.so"));
}

bool status_notifier_host_registered() {
	// Avoid libdbus autolaunching a private bus when there is none.
	if(qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
		return false;
	const QDBusConnection bus = QDBusConnection::sessionBus();
	if(!bus.isConnected())
		return false;
	QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.StatusNotifierWatcher"),
														  QStringLiteral("/StatusNotifierWatcher"),
														  QStringLiteral("org.freedesktop.DBus.Properties"),
														  QStringLiteral("Get"));
	message << QStringLiteral("org.kde.StatusNotifierWatcher")
			<< QStringLiteral("IsStatusNotifierHostRegistered");
	const QDBusMessage reply = bus.call(message, QDBus::Block, 300);
	if(reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
		return false;
	return reply.arguments().first().value<QDBusVariant>().variant().toBool();
}

}  // namespace
#endif

namespace {
bool g_left_kde_theme = false;
}

void PlatformTray::prepare_platform_tray() {
#if defined(SCREEN_LIGHT_ADJUST_HAVE_DBUS)
	if(!is_x11_session() || !uses_kde_platform_theme() || !kde_theme_plugin_available()
	   || status_notifier_host_registered())
		return;

	// The KDE platform theme only creates StatusNotifierItems; i3bar only
	// speaks XEmbed. Switch to a theme whose tray backend can fall back.
	if(gtk3_theme_available()) {
		g_left_kde_theme = true;
		qputenv("QT_QPA_PLATFORMTHEME", "gtk3");
		if(qEnvironmentVariableIsEmpty("QT_STYLE_OVERRIDE") && breeze_style_available())
			qputenv("QT_STYLE_OVERRIDE", "Breeze");
	} else {
		qputenv("QT_QPA_PLATFORMTHEME", "generic");
	}
#endif
}

void PlatformTray::restore_kde_palette() {
	if(!g_left_kde_theme)
		return;
	const QString path =
		QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals"));
	if(path.isEmpty())
		return;
	const QSettings kdeglobals(path, QSettings::IniFormat);

	struct Entry {
		const char*			section;
		const char*			key;
		QPalette::ColorRole role;
	};

	static const Entry entries[] = {
		{"Colors:Window",	  "BackgroundNormal",	  QPalette::Window		  },
		{"Colors:Window",	  "ForegroundNormal",	  QPalette::WindowText	  },
		{"Colors:View",		"BackgroundNormal",	QPalette::Base			  },
		{"Colors:View",		"BackgroundAlternate", QPalette::AlternateBase  },
		{"Colors:View",		"ForegroundNormal",	QPalette::Text			  },
		{"Colors:Button",	  "BackgroundNormal",	  QPalette::Button		  },
		{"Colors:Button",	  "ForegroundNormal",	  QPalette::ButtonText	  },
		{"Colors:Selection", "BackgroundNormal",	 QPalette::Highlight		},
		{"Colors:Selection", "ForegroundNormal",	 QPalette::HighlightedText},
	};
	QPalette palette = QApplication::palette();
	for(const Entry& entry : entries) {
		// kdeglobals stores colours as "R,G,B"; QSettings returns that as a QStringList.
		const QString key =
			QString::fromLatin1(entry.section) + QLatin1Char('/') + QString::fromLatin1(entry.key);
		const QStringList rgb = kdeglobals.value(key).toStringList();
		if(rgb.size() < 3)
			continue;
		const QColor color(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
		palette.setColor(QPalette::Active, entry.role, color);
		palette.setColor(QPalette::Inactive, entry.role, color);
	}
	QApplication::setPalette(palette);
}
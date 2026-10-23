#include "mainwindow.h"
#include "platform_tray.h"

#include <QApplication>

int main(int argc, char* argv[]) {

	QCoreApplication::setApplicationName("Screen Light Adjust");
	QCoreApplication::setOrganizationName("screen-light-adjust");
	QGuiApplication::setDesktopFileName("screen-light-adjust");

	PlatformTray::prepare_platform_tray();
	QApplication app(argc, argv);
	PlatformTray::restore_kde_palette();

	MainWindow window;
	window.show();

	return app.exec();
}

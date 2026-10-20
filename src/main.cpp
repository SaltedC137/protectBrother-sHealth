#include "screen_light_adjust/mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("Screen Light Adjust");
	app.setOrganizationName("screen-light-adjust");
	app.setDesktopFileName("screen-light-adjust");

	MainWindow w;
	w.show();

	return app.exec();
}

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("screen-light-adjust");
	app.setOrganizationName("screen-light-adjust");
	app.setQuitOnLastWindowClosed(false);  // keep running in tray

	MainWindow w;
	w.show();

	return app.exec();
}

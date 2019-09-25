#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	const char* configfile = ":/conf/oled.json";
	const char* host = "localhost";
	const char* port = "1339";
	if (argc > 1) {
		configfile = argv[1];
	}
	if (argc > 2) {
		host = argv[2];
	}
	if (argc > 3) {
		port = argv[3];
	}
	VPBreadboard w(configfile, host, port);
	w.show();

	return a.exec();
}

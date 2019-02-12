#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	const char* host = "localhost";
	const char* port = "1339";
	if (argc > 1) {
		host = argv[1];
	}
	if (argc > 2) {
		port = argv[2];
	}
	VPBreadboard w(host, port);
	w.show();

	return a.exec();
}

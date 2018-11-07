#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VPBreadboard w;
    w.show();

    return a.exec();
}

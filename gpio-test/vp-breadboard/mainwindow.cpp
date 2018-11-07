#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qpainter.h>
#include <cassert>
#include <iostream>
#include <QKeyEvent>

#include <unistd.h> //sleep

using namespace std;

void Sevensegment::draw()
{
    //  0
    //5   1
    //  6
    //4   2
    //  3   7
    printf(" %c\n", map & 1 ? '_' : ' ');
    printf("%c %c\n", map & (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : ' ');
    printf(" %c\n", map & (1 << 6) ? '-' : ' ');
    printf("%c %c\n", map & (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : ' ');
    printf(" %c  %c\n", map & (1 << 3) ? '-' : ' ', map & (1 << 7) ? '.' : ' ');
}

VPBreadboard::VPBreadboard(QWidget *mparent)
    : QWidget(mparent)
{
    //resize(800, 600);

    QPixmap bkgnd(":/img/breadboard.jpg");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Background, bkgnd);
    this->setPalette(palette);
    setFixedSize(size());
}

VPBreadboard::~VPBreadboard()
{
}

void VPBreadboard::paintEvent(QPaintEvent *){
    QPainter painter(this);

    //painter.scale(size_factor, size_factor);

    //Draw Header
    //QPainter mempaint(&memory);
    sevensegment.map ^= 0xFF;
    sevensegment.draw();
    painter.end();
    this->update();
}

void VPBreadboard::notifyChange(bool success)
{
    assert(success);
    update();
}


void VPBreadboard::keyPressEvent(QKeyEvent *e)
{
    cout << "Yee, keypress" << endl;
    switch (e->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        QApplication::quit();
        break;
    default:
        break;
    }
}

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
    if(!gpio.setupConnection("localhost", "1339"))
    {
        cerr << "Could not setup Connection" << endl;
        QApplication::quit();
    }
}

VPBreadboard::~VPBreadboard()
{
}

uint64_t VPBreadboard::translateGpioToExtPin(Gpio::Reg& reg)
{
    //Todo: This
    return reg;
}

uint8_t VPBreadboard::translatePinNumberToSevensegment(uint64_t pinmap)
{
    //Todo: This
    return pinmap & 0xFF;
}

void VPBreadboard::paintEvent(QPaintEvent *){
    QPainter painter(this);

    if(!gpio.update())
    {
        cerr << "Could not update values" << endl;
        //todo: Try to reconnect
        QApplication::quit();
    }

    sevensegment.map = translatePinNumberToSevensegment(translateGpioToExtPin(gpio.state.val));
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
    case Qt::Key_0:
    {
        uint8_t until = 6;
        for(uint8_t i = 0; i < 8; i++)
        {
            gpio.setBit(i, i < until ? 1 : 0);
        }
        break;
    }
    case Qt::Key_1:
    {
        for(uint8_t i = 0; i < 8; i++)
        {
            gpio.setBit(i, 0);
        }
        break;
    }
    default:
        break;
    }
}

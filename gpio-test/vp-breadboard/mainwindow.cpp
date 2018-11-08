#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qpainter.h>
#include <cassert>
#include <iostream>
#include <QKeyEvent>

#include <unistd.h> //sleep

using namespace std;

void Sevensegment::draw(QPainter& p)
{
    p.save();
    QPen segment(QColor("#fc5c44"), linewidth, Qt::PenStyle::SolidLine, Qt::PenCapStyle::RoundCap, Qt::RoundJoin);
    p.setPen(segment);

    //  0
    //5   1
    //  6
    //4   2
    //  3   7
    //printf(" %c\n", map & 1 ? '_' : ' ');
    //printf("%c %c\n", map & (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : ' ');
    //printf(" %c\n", map & (1 << 6) ? '-' : ' ');
    //printf("%c %c\n", map & (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : ' ');
    //printf(" %c  %c\n", map & (1 << 3) ? '-' : ' ', map & (1 << 7) ? '.' : ' ');

    int xcol1 = 0;
    int xcol2 = 3*(extent.x()/4);
    int yrow1 = 0;
    int xrow1 = extent.x()/4 - 2;
    int yrow2 = extent.y()/2;
    int xrow2 = extent.x()/8 - 1;
    int yrow3 = extent.y();
    int xrow3 = 0;

    if(map & 0b00000001)    //0
        p.drawLine(offs+QPoint(xcol1+xrow1, yrow1), offs+QPoint(xcol2+xrow1, yrow1));
    if(map & 0b00000010)    //1
        p.drawLine(offs+QPoint(xcol2+xrow1, yrow1), offs+QPoint(xcol2+xrow2, yrow2));
    if(map & 0b00000100)    //2
        p.drawLine(offs+QPoint(xcol2+xrow2, yrow2), offs+QPoint(xcol2+xrow3, yrow3));
    if(map & 0b00001000)    //3
        p.drawLine(offs+QPoint(xcol2+xrow3, yrow3), offs+QPoint(xcol1+xrow3, yrow3));
    if(map & 0b00010000)    //4
        p.drawLine(offs+QPoint(xcol1+xrow3, yrow3), offs+QPoint(xcol1+xrow2, yrow2));
    if(map & 0b00100000)    //5
        p.drawLine(offs+QPoint(xcol1+xrow2, yrow2), offs+QPoint(xcol1+xrow1, yrow1));
    if(map & 0b01000000)    //6
        p.drawLine(offs+QPoint(xcol1+xrow2, yrow2), offs+QPoint(xcol2+xrow2, yrow2));
    if(map & 0b10000000)    //7
        p.drawPoint(offs+extent);

    p.restore();
}

VPBreadboard::VPBreadboard(QWidget *mparent)
    : QWidget(mparent), sevensegment(QPoint(312, 353), QPoint(36, 51), 6), button(QPoint(373, 343), QSize(55, 55))
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

uint8_t VPBreadboard::translatePinToGpioOffs(uint8_t pin)
{
    return pin;
}

uint8_t VPBreadboard::getPinnumberOfButton()
{
    return 11;
}

void VPBreadboard::paintEvent(QPaintEvent *){
    QPainter painter(this);

    if(!gpio.update())
    {
        cerr << "Could not update values" << endl;
        //todo: Try to reconnect
        QApplication::quit();
    }

    sevensegment.map = translatePinNumberToSevensegment(translateGpioToExtPin(gpio.state));
    sevensegment.draw(painter);


    if(debugmode)
    {
        //painter.setBrush(QBrush(QColor("black")));
        painter.drawRect(button);
    }
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
    case Qt::Key_W:
        button.moveTopLeft(button.topLeft() - QPoint(0, 1));
        cout << "E X: " << button.topLeft().x() << " Y: " << button.topLeft().y() << endl;
        break;
    case Qt::Key_A:
        button.moveTopLeft(button.topLeft() - QPoint(1, 0));
        cout << "E X: " << button.topLeft().x() << " Y: " << button.topLeft().y() << endl;
        break;
    case Qt::Key_S:
        button.moveTopLeft(button.topLeft() + QPoint(0, 1));
        cout << "E X: " << button.topLeft().x() << " Y: " << button.topLeft().y() << endl;
        break;
    case Qt::Key_D:
        button.moveTopLeft(button.topLeft() + QPoint(1, 0));
        cout << "BR X: " << button.topLeft().x() << " Y: " << button.topLeft().y() << endl;
        break;
    case Qt::Key_Up:
        button.setHeight(button.height() - 1);
        cout << "height: " << button.height() << endl;
        break;
    case Qt::Key_Left:
        button.setWidth(button.width() - 1);
        cout << "width: " << button.height() << endl;
        break;
    case Qt::Key_Down:
        button.setHeight(button.height() + 1);
        cout << "height: " << button.height() << endl;
        break;
    case Qt::Key_Right:
        button.setWidth(button.width() + 1);
        cout << "width: " << button.height() << endl;
        break;
    case Qt::Key_Space:
        cout << "Changed Debug mode" << endl;
        debugmode ^= 1;
        break;
    default:
        break;
    }
}

void VPBreadboard::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if(button.contains(e->pos()))
        {
            cout << "button click!" << endl;
            gpio.setBit(translatePinToGpioOffs(getPinnumberOfButton()), 1);
        }
        cout << "clicked summin elz" << endl;
    }
    else
    {
        cout << "Whatcha doin' there?" << endl;
    }
    e->accept();
}

void VPBreadboard::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if(button.contains(e->pos()))
        {
            cout << "button release!" << endl;
        }
        cout << "released summin elz" << endl;
        gpio.setBit(translatePinToGpioOffs(getPinnumberOfButton()), 0);
    }
    else
    {
        cout << "Whatcha doin' there?" << endl;
    }
    e->accept();
}

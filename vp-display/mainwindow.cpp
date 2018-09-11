#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "framebuffer.h"
#include <qpainter.h>
#include <cassert>

VPDisplay::VPDisplay(QWidget *mparent)
    : QWidget(mparent)
{
    framebuffer = server.createSM();
    frame = new QImage(screenWidth, screenHeight, QImage::Format_RGB444); //two bytes per pixel
    resize(800, 600);
    setFixedSize(size());
    server.startListening(std::bind(&VPDisplay::notifyChange, this, std::placeholders::_1));
}

VPDisplay::~VPDisplay()
{
    delete frame;
}

void VPDisplay::drawMainPage(QImage* mem)
{
    for(int row = 0; row < mem->height(); row++)
    {
        uint8_t* line = mem->scanLine(row);
        memcpy(line, framebuffer->getActiveFrame().raw[row], mem->width() * 2); //two bytes per Pixel
    }
}

void VPDisplay::paintEvent(QPaintEvent *){
    QPainter painter(this);

    //painter.scale(size_factor, size_factor);

    //Draw Header
    //QPainter mempaint(&memory);

    drawMainPage(frame);
    painter.drawImage(QPoint(0, 0), *frame);
    painter.end();
}

void VPDisplay::notifyChange(bool success)
{
    assert(success);
    update();
}

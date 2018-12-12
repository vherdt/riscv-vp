#pragma once
#include "vpdisplayserver.h"
#include <QMainWindow>
#include <cassert>

namespace Ui {
class VPDisplay;
}

class VPDisplay : public QWidget
{
    Q_OBJECT
    Framebuffer* framebuffer;
    QImage* frame;
    VPDisplayserver server;
public:
    VPDisplay(QWidget *mparent = 0);
    ~VPDisplay();
    void paintEvent(QPaintEvent *);
    //void keyPressEvent(QKeyEvent *e);
    void drawMainPage(QImage* mem);

    void notifyChange(bool success);
};

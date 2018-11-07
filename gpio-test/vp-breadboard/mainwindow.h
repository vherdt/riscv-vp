#pragma once
#include <QMainWindow>
#include <cassert>
#include "../gpio-client.hpp"

namespace Ui {
class VPBreadboard;
}

struct Sevensegment
{
    uint8_t map;
    void draw();
};

class VPBreadboard : public QWidget
{
    Q_OBJECT
    GpioClient gpio;
    Sevensegment sevensegment;
    uint64_t translateGpioToExtPin(Gpio::Reg& reg);
    uint8_t translatePinNumberToSevensegment(uint64_t pinmap);

public:
    VPBreadboard(QWidget *mparent = 0);
    ~VPBreadboard();
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *e);

    void notifyChange(bool success);
};

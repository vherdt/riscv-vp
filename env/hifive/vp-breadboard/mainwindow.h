#pragma once
#include <QMainWindow>
#include <cassert>

#include <gpio/gpio-client.hpp>

namespace Ui {
class VPBreadboard;
}

struct Sevensegment {
	QPoint offs;
	QPoint extent;
	uint8_t linewidth;
	uint8_t map;
	void draw(QPainter& p);
	Sevensegment() : offs(50, 50), extent(100, 100), linewidth(10), map(0){};
	Sevensegment(QPoint offs, QPoint extent, uint8_t linewidth)
	    : offs(offs), extent(extent), linewidth(linewidth), map(0){};
};

struct RGBLed {
	QPoint offs;
	uint8_t linewidth;
	uint8_t map;
	void draw(QPainter& p);
	RGBLed(QPoint offs, uint8_t linewidth) : offs(offs), linewidth(linewidth){};
};

class VPBreadboard : public QWidget {
	Q_OBJECT
	GpioClient gpio;
	Sevensegment sevensegment;
	RGBLed rgbLed;
	QRect button;
	const char* host;
	const char* port;

	bool debugmode = false;
	bool inited = false;

	uint64_t translateGpioToExtPin(GpioCommon::Reg reg);
	uint8_t translatePinNumberToSevensegment(uint64_t pinmap);
	uint8_t translatePinNumberToRGBLed(uint64_t pinmap);
	uint8_t translatePinToGpioOffs(uint8_t pin);
	uint8_t getPinnumberOfButton();

   public:
	VPBreadboard(const char* host, const char* port, QWidget* mparent = 0);
	~VPBreadboard();
	void showConnectionErrorOverlay(QPainter& p);
	void paintEvent(QPaintEvent*) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void notifyChange(bool success);
};

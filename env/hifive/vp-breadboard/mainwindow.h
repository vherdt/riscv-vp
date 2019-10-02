#pragma once
#include <QMainWindow>
#include <cassert>

#include <gpio/gpio-client.hpp>
#include <oled/common.hpp>

namespace Ui {
class VPBreadboard;
}

static constexpr unsigned max_num_buttons = 5;

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

struct OLED
{
	const ss1106::State* state;
	QPoint offs;
	QPoint margin;
	QImage image;
	void draw(QPainter& p);
	OLED(QPoint offs, unsigned margin) : offs(offs),
			margin(QPoint(margin, margin)),
			image(ss1106::width - 2*ss1106::padding_lr, ss1106::height, QImage::Format_Grayscale8)
	{
		state = ss1106::getSharedState();
	};
};

struct Button
{
	QRect area;
	uint8_t pin;
};

class VPBreadboard : public QWidget {
	Q_OBJECT
	GpioClient gpio;
	Sevensegment* sevensegment;
	RGBLed* rgbLed;
	OLED* oled;
	Button* buttons[max_num_buttons];
	const char* host;
	const char* port;

	bool debugmode = false;
	bool inited = false;

	uint64_t translateGpioToExtPin(GpioCommon::Reg reg);
	uint8_t translatePinNumberToSevensegment(uint64_t pinmap);
	uint8_t translatePinNumberToRGBLed(uint64_t pinmap);
	uint8_t translatePinToGpioOffs(uint8_t pin);

   public:
	VPBreadboard(const char* configfile, const char* host, const char* port, QWidget* mparent = 0);
	~VPBreadboard();
	void showConnectionErrorOverlay(QPainter& p);
	void paintEvent(QPaintEvent*) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void notifyChange(bool success);
};

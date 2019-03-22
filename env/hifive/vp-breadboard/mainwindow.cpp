#include "mainwindow.h"
#include <qpainter.h>
#include <QKeyEvent>
#include <cassert>
#include <iostream>
#include "ui_mainwindow.h"

#include <unistd.h>  //sleep

using namespace std;

void Sevensegment::draw(QPainter& p) {
	p.save();
	QPen segment(QColor("#f72727"), linewidth, Qt::PenStyle::SolidLine, Qt::PenCapStyle::RoundCap, Qt::RoundJoin);
	p.setPen(segment);

	//  0
	// 5   1
	//  6
	// 4   2
	//  3   7
	// printf(" %c\n", map & 1 ? '_' : ' ');
	// printf("%c %c\n", map & (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : '
	// '); printf(" %c\n", map & (1 << 6) ? '-' : ' '); printf("%c %c\n", map &
	// (1 << 5) ? '|' : ' ', map & (1 << 1) ? '|' : ' '); printf(" %c  %c\n",
	// map & (1 << 3) ? '-' : ' ', map & (1 << 7) ? '.' : ' ');

	int xcol1 = 0;
	int xcol2 = 3 * (extent.x() / 4);
	int yrow1 = 0;
	int xrow1 = extent.x() / 4 - 2;
	int yrow2 = extent.y() / 2;
	int xrow2 = extent.x() / 8 - 1;
	int yrow3 = extent.y();
	int xrow3 = 0;

	if (map & 0b00000001)  // 0
		p.drawLine(offs + QPoint(xcol1 + xrow1, yrow1), offs + QPoint(xcol2 + xrow1, yrow1));
	if (map & 0b00000010)  // 1
		p.drawLine(offs + QPoint(xcol2 + xrow1, yrow1), offs + QPoint(xcol2 + xrow2, yrow2));
	if (map & 0b00000100)  // 2
		p.drawLine(offs + QPoint(xcol2 + xrow2, yrow2), offs + QPoint(xcol2 + xrow3, yrow3));
	if (map & 0b00001000)  // 3
		p.drawLine(offs + QPoint(xcol2 + xrow3, yrow3), offs + QPoint(xcol1 + xrow3, yrow3));
	if (map & 0b00010000)  // 4
		p.drawLine(offs + QPoint(xcol1 + xrow3, yrow3), offs + QPoint(xcol1 + xrow2, yrow2));
	if (map & 0b00100000)  // 5
		p.drawLine(offs + QPoint(xcol1 + xrow2, yrow2), offs + QPoint(xcol1 + xrow1, yrow1));
	if (map & 0b01000000)  // 6
		p.drawLine(offs + QPoint(xcol1 + xrow2, yrow2), offs + QPoint(xcol2 + xrow2, yrow2));
	if (map & 0b10000000)  // 7
		p.drawPoint(offs + extent);

	p.restore();
}

void RGBLed::draw(QPainter& p) {
	if (!map) {
		return;
	}
	p.save();
	QPen led(QColor(map & 1 ? 255 : 0, map & (1 << 1) ? 255 : 0, map & (1 << 2) ? 255 : 0, 0xC0), linewidth,
	         Qt::PenStyle::SolidLine, Qt::PenCapStyle::RoundCap, Qt::RoundJoin);
	p.setPen(led);

	p.drawPoint(offs);

	p.restore();
}

VPBreadboard::VPBreadboard(const char* host, const char* port, QWidget* mparent)
    : QWidget(mparent),
      host(host),
      port(port),
      sevensegment(QPoint(312, 353), QPoint(36, 50), 7),
      rgbLed(QPoint(89, 161), 15),
      button(QPoint(373, 343), QSize(55, 55)) {
	// resize(800, 600);

	QPixmap bkgnd(":/img/breadboard.jpg");
	bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
	QPalette palette;
	palette.setBrush(QPalette::Background, bkgnd);
	this->setPalette(palette);
	setFixedSize(size());
}

VPBreadboard::~VPBreadboard() {}

void VPBreadboard::showConnectionErrorOverlay(QPainter& p) {
	p.save();
	p.setBrush(QBrush(QColor("black")));
	p.drawRect(QRect(QPoint(150, 100), QSize(350, 200)));
	p.setFont(QFont("Arial", 25, QFont::Bold));
	QPen penHText(QColor("red"));
	p.setPen(penHText);
	p.drawText(QPoint(210, 210), QString("No connection"));
	p.restore();
}

uint64_t VPBreadboard::translateGpioToExtPin(GpioCommon::Reg reg) {
	uint64_t ext = 0;
	for (uint64_t i = 0; i < 24; i++)  // Max Pin is 32, see SiFive HiFive1
	                                   // Getting Started Guide 1.0.2 p. 20
	{
		// cout << i << " to ";;
		if (i >= 16) {
			ext |= (reg & (1l << i)) >> 16;
			// cout << i - 16 << endl;
		} else if (i <= 5) {
			ext |= (reg & (1l << i)) << 8;
			// cout << i + 8 << endl;
		} else if (i >= 9 && i <= 13) {
			ext |= (reg & (1l << i)) << 6;  // Bitshift Ninja! (with broken
			                                // legs) cout << i + 6 << endl;
		}
		// rest is not connected.
	}
	return ext;
}

uint8_t VPBreadboard::translatePinNumberToSevensegment(uint64_t pinmap) {
	uint8_t ret = 0;
	static uint8_t pinMapping[8] =
	{
	  15, 16, 17, 18, 19, 7, 6, 5
	};
	for(unsigned i = 0; i < 8; i++)
	{
		ret |= pinmap & (1 << pinMapping[i]) ? (1 << i) : 0;
	}
	return ret;
}

uint8_t VPBreadboard::translatePinNumberToRGBLed(uint64_t pinmap) {
	uint8_t ret = 0;
	ret |= (~pinmap & (1 << 6)) >> 6;  // R
	ret |= (~pinmap & (1 << 3)) >> 2;  // G
	ret |= (~pinmap & (1 << 5)) >> 3;  // B
	return ret;
}

uint8_t VPBreadboard::translatePinToGpioOffs(uint8_t pin) {
	if (pin < 8) {
		return pin + 16;  // PIN_0_OFFSET
	}
	if (pin < 20) {
		return pin - 8;
	}
	return 0;  // also ignoring non-wired pin 14 <==> 8
}

uint8_t VPBreadboard::getPinnumberOfButton() {
	return 3;
}

void printBin(char* buf, uint8_t len) {
	for (uint16_t byte = 0; byte < len; byte++) {
		for (int8_t bit = 7; bit >= 0; bit--) {
			printf("%c", buf[byte] & (1 << bit) ? '1' : '0');
		}
		printf(" ");
	}
	printf("\n");
}

void VPBreadboard::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (!inited || !gpio.update()) {
		inited = gpio.setupConnection(host, port);
		showConnectionErrorOverlay(painter);
		if (!inited)
			usleep(500000);
		this->update();
		return;
	}

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

	sevensegment.map = translatePinNumberToSevensegment(translateGpioToExtPin(gpio.state));
	sevensegment.draw(painter);

	rgbLed.map = translatePinNumberToRGBLed(translateGpioToExtPin(gpio.state));
	rgbLed.draw(painter);

	if (debugmode) {
		// painter.setBrush(QBrush(QColor("black")));
		painter.drawRect(button);
		painter.drawRect(QRect(sevensegment.offs, QSize(sevensegment.extent.x(), sevensegment.extent.y())));
	}
	painter.end();
	// intentional slow down
	// TODO: update at fixed rate, async between redraw and gpioserver
	usleep(5000);
	this->update();
}

void VPBreadboard::notifyChange(bool success) {
	assert(success);
	update();
}

void VPBreadboard::keyPressEvent(QKeyEvent* e) {
	this->update();
	// scout << "Yee, keypress" << endl;
	switch (e->key()) {
		case Qt::Key_Escape:
		case Qt::Key_Q:
			QApplication::quit();
			break;
		case Qt::Key_0: {
			uint8_t until = 6;
			for (uint8_t i = 0; i < 8; i++) {
				gpio.setBit(i, i < until ? 1 : 0);
			}
			break;
		}
		case Qt::Key_1: {
			for (uint8_t i = 0; i < 8; i++) {
				gpio.setBit(i, 0);
			}
			break;
		}
		case Qt::Key_I:
			rgbLed.offs = rgbLed.offs - QPoint(0, 1);
			cout << "E X: " << rgbLed.offs.x() << " Y: " << rgbLed.offs.y() << endl;
			break;
		case Qt::Key_J:
			rgbLed.offs = rgbLed.offs - QPoint(1, 0);
			cout << "E X: " << rgbLed.offs.x() << " Y: " << rgbLed.offs.y() << endl;
			break;
		case Qt::Key_K:
			rgbLed.offs = rgbLed.offs + QPoint(0, 1);
			cout << "E X: " << rgbLed.offs.x() << " Y: " << rgbLed.offs.y() << endl;
			break;
		case Qt::Key_L:
			rgbLed.offs = rgbLed.offs + QPoint(1, 0);
			cout << "E X: " << rgbLed.offs.x() << " Y: " << rgbLed.offs.y() << endl;
			break;

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

void VPBreadboard::mousePressEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {
		if (button.contains(e->pos())) {
			// cout << "button click!" << endl;
			gpio.setBit(translatePinToGpioOffs(getPinnumberOfButton()),
			            0);  // Active low
		}
		// cout << "clicked summin elz" << endl;
	} else {
		cout << "Whatcha doin' there?" << endl;
	}
	e->accept();
}

void VPBreadboard::mouseReleaseEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {
		if (button.contains(e->pos())) {
			// cout << "button release!" << endl;
		}
		// cout << "released summin elz" << endl;
		gpio.setBit(translatePinToGpioOffs(getPinnumberOfButton()), 1);
	} else {
		cout << "Whatcha doin' there?" << endl;
	}
	e->accept();
}

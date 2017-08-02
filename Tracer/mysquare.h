#ifndef MYSQUARE_H
#define MYSQUARE_H
#include <QPainter>
#include <QGraphicsItem>
#include <QDebug>
#include <QTextBrowser>
#include "tracer.h"

class MySquare : public QGraphicsItem
{
public:
    MySquare(int my_x,
				int my_y,
				int my_width,
				trace_info_t my_node_info,
				QTextBrowser *browser,
				QColor my_color);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    bool Pressed;

private:
    int MyX;
    int MyY;
    int MyWidth;
		QColor MyColor;
		QTextBrowser *TextBrowser;

		trace_info_t MyNodeInfo;

		void show_node_info();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

#endif // MYSQUARE_H

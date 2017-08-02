#include "mysquare.h"
#include "mainwindow.h"
#include <QTextBrowser>

MySquare::MySquare(int my_x, int my_y,int my_width, trace_info_t my_node_info, QTextBrowser *browser, QColor my_color)
{
    Pressed = false;
    setFlag(ItemIsSelectable);
    MyX = my_x;
    MyY = my_y;
    MyWidth = my_width;
		MyNodeInfo = my_node_info;
		TextBrowser = browser;
		MyColor = my_color;
}

QRectF MySquare::boundingRect() const
{
    int MyHeight = 10;

    return QRectF(MyX,MyY,MyWidth,MyHeight);
}

void MySquare::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QRectF rec = boundingRect();
  QBrush brush(Qt::black); //default color
	painter->setPen(Qt::black);

  if (Pressed){
    brush.setColor(Qt::red);
    show_node_info();
  }else{
    brush.setColor(MyColor); 
  }

  painter->fillRect(rec,brush);
  painter->drawRect(rec);
}


void MySquare::show_node_info()
{
	QString qstr;
	std::stringstream ss;
	std::stringstream sub_topic;
	std::stringstream pub_topic;

	for(int i(0); i<(int) MyNodeInfo.v_subtopic.size();i++){
		sub_topic << MyNodeInfo.v_subtopic.at(i) << ", ";
	}

	for(int i(0); i<(int) MyNodeInfo.v_pubtopic.size();i++){
		pub_topic << MyNodeInfo.v_pubtopic.at(i) << ", ";
	}

	ss << "Name: " << MyNodeInfo.name << "\n"
		 << "PID: " << MyNodeInfo.pid << "\n" 
		 << "Prio " << MyNodeInfo.prio << "\n"
		 << "Core: " << MyNodeInfo.core << "\n"
		 << "Runtime: " << MyNodeInfo.runtime << "\n"
		 << "Start Time: " << MyNodeInfo.start_time << "\n"
		 << "Finish Time: " << MyNodeInfo.start_time + MyNodeInfo.runtime << "\n"
		 << "sub Topics: " << sub_topic.str() << "\n"
		 << "pub Topic: " << pub_topic.str();

	
	qstr = QString::fromStdString(ss.str());
	TextBrowser->setText(qstr);
}

void MySquare::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Pressed = true;
    update();
    QGraphicsItem::mousePressEvent(event);

}

void MySquare::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  Pressed = false;
  update();
  QGraphicsItem::mouseReleaseEvent(event);
}

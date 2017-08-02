#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSplitter>
#include <QGraphicsItem>
#include <QtGui>
#include <QTextBrowser>
#include <QPushButton>
#include <QCheckBox>
#include "tracer.h"
#include <QGroupBox>
#include "mysquare.h"
#include <QtCore>
#include <QtGui>
#include <QGraphicsView>

class QMenu;
class SecondDialog;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QGroupBox *NodeGroup;
    QDialog *passWindow;

private:
    Ui::MainWindow *ui;

    void viz_process(std::vector<trace_info_t> info, double zoom);
    void requirePass();

    QString pass;

		std::vector<MySquare*> process_info;
		std::vector<QLabel*> label_list;
    QGraphicsView *view;
    QGraphicsScene *scene;
    MySquare *square;
		QTextBrowser *browser;
		std::vector<int> pid_list;
		std::vector<trace_info_t> info;
		double ZOOM;

    QGroupBox *createCPUGroup();
    QGroupBox *createNodeGroup();
    QGroupBox *createTextBrowser();
    QGroupBox *createButtonGroup();

		QColor get_color(int pid);
		QColor colors[6] = {Qt::red, Qt::blue, Qt::green, Qt::yellow, Qt::magenta, Qt::cyan};

public slots:
    void StartStopTrace(bool click);
    void ShowNodes(bool click);
    void quit();
    void setPass(const QString &);
    void onSetPass();
		void zoom_out_process();
		void zoom_in_process();

};

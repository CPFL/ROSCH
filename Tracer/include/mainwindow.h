#include <QCheckBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsView>
#include <QGroupBox>
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QtCore>
#include <QtGui>
#include <QtGui>
#include "mysquare.h"
#include "tracer.h"

//#define ANALYZE_DEADLINE_MISS

class QMenu;
class SecondDialog;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  QGroupBox *CPUGroup;
  QGroupBox *NodeGroup;
  QGroupBox *NodeListGroup;
  QDialog *passWindow;
  void requirePass();
  int mode;  // CPU_MODE or NODE_MODE

private:
  Ui::MainWindow *ui;

  void viz_process(std::vector<trace_info_t> info, double zoom);

  QString pass;

  std::vector<MySquare *> process_info;
  std::vector<QLabel *> label_list;
  std::vector<QLabel *> Node_list;
  std::vector<node_info_t> node_list;
  std::vector<QGraphicsLineItem *> deadline_miss_list;
  QGraphicsLineItem *deadline_miss;

  QGraphicsView *CPU_view;
  QGraphicsScene *CPU_scene;
  QGraphicsView *Node_view;
  QGraphicsScene *Node_scene;
  MySquare *square;
  QTextBrowser *browser;
  std::vector<int> pid_list;

  std::vector<trace_info_t> info;
  double ZOOM;

  QGroupBox *createCPUGroup();
  QGroupBox *createNodeGroup();
  QGroupBox *createNodeListGroup();
  QGroupBox *createTextBrowser();
  QGroupBox *createButtonGroup();

  QColor get_color(int pid);
  QColor colors[6] = { Qt::red, Qt::blue, Qt::green, Qt::yellow, Qt::magenta, Qt::cyan };
  void delete_viz_process();

#define CPU_MODE 1
#define NODE_MODE 2

public slots:
  void StartStopTrace(bool click);
  void ShowNodes(bool click);
  void quit();
  void setPass(const QString &);
  void onSetPass();
  void zoom_out_process();
  void zoom_in_process();
  void changeCPUNode(bool click);
};

#include "mainwindow.h"
#include <QBoxLayout>
#include <QColor>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextBrowser>
#include <QtCore/QString>
#include <string>
#include "mysquare.h"
#include "tracer.h"
#include "ui_mainwindow.h"

#define CORE 8

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  setGeometry(0, 0, 800, 500);

  setWindowTitle(tr("ROSHC: TRACER"));
  CPUGroup = createCPUGroup();
  NodeGroup = createNodeGroup();
  NodeListGroup = createNodeListGroup();

  QVBoxLayout *toplayout = new QVBoxLayout;
  toplayout->addWidget(CPUGroup);
  toplayout->addWidget(NodeGroup);
  toplayout->addWidget(NodeListGroup);
  toplayout->addWidget(createTextBrowser());
  toplayout->addWidget(createButtonGroup());

  QWidget *window = new QWidget();
  window->setLayout(toplayout);
  ZOOM = 1000;
  mode = CPU_MODE;

  NodeGroup->hide();
  NodeListGroup->hide();
  setCentralWidget(window);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::requirePass()
{
  passWindow = new QDialog;
  QLabel *label = new QLabel("pass: ");
  QLineEdit *edit = new QLineEdit;
  QPushButton *button = new QPushButton("ok");
  QHBoxLayout *layout = new QHBoxLayout;

  QObject::connect(edit, SIGNAL(textChanged(const QString &)), this, SLOT(setPass(const QString &)));
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(onSetPass()));

  layout->addWidget(label);
  layout->addWidget(edit);
  layout->addWidget(button);
  passWindow->setWindowTitle("Password");
  passWindow->setLayout(layout);

  passWindow->show();
}

void MainWindow::setPass(const QString &input)
{
  if (input != pass)
  {
    pass = input;
  }
}

void MainWindow::onSetPass()
{
  passWindow->hide();
}

QGroupBox *MainWindow::createCPUGroup()
{
  QGroupBox *groupBox = new QGroupBox("CPUs");
  QVBoxLayout *toplayout = new QVBoxLayout;
  QHBoxLayout *graph = new QHBoxLayout;

  QLabel *cpu0 = new QLabel("cpu 0");
  QLabel *cpu1 = new QLabel("cpu 1");
  QLabel *cpu2 = new QLabel("cpu 2");
  QLabel *cpu3 = new QLabel("cpu 3");
  QLabel *cpu4 = new QLabel("cpu 4");
  QLabel *cpu5 = new QLabel("cpu 5");
  QLabel *cpu6 = new QLabel("cpu 6");
  QLabel *cpu7 = new QLabel("cpu 7");
  QVBoxLayout *layout = new QVBoxLayout;

  layout->setSpacing(20);
  layout->addSpacing(10);
  layout->addWidget(cpu0);
  layout->addWidget(cpu1);
  layout->addWidget(cpu2);
  layout->addWidget(cpu3);
  layout->addWidget(cpu4);
  layout->addWidget(cpu5);
  layout->addWidget(cpu6);
  layout->addWidget(cpu7);
  layout->addSpacing(20);
  graph->addLayout(layout);

  CPU_scene = new QGraphicsScene(this);
  ui->graphicsView->setScene(CPU_scene);

  CPU_view = new QGraphicsView(CPU_scene);
  graph->addWidget(CPU_view);

  QHBoxLayout *buttons = new QHBoxLayout;
  QPushButton *ZoomOut = new QPushButton("Zoom Out");
  QPushButton *ZoomIn = new QPushButton("Zoom In");
  buttons->addWidget(ZoomOut);
  buttons->addWidget(ZoomIn);
  buttons->addStretch();

  QObject::connect(ZoomOut, SIGNAL(clicked()), this, SLOT(zoom_out_process()));
  QObject::connect(ZoomIn, SIGNAL(clicked()), this, SLOT(zoom_in_process()));

  toplayout->addLayout(graph);
  toplayout->addLayout(buttons);
  toplayout->addStretch();
  groupBox->setLayout(toplayout);

  return groupBox;
}

QGroupBox *MainWindow::createNodeGroup()
{
  QGroupBox *groupBox = new QGroupBox("Nodes");
  QVBoxLayout *toplayout = new QVBoxLayout;
  QHBoxLayout *graph = new QHBoxLayout;

  SchedViz::Tracer tracer;
  std::string node_name_;
  QLabel *name_label;

  /* create labelske */
  node_list = tracer.get_node_list();
  QVBoxLayout *layout = new QVBoxLayout;

  layout->setSpacing(20);
  layout->addSpacing(10);

  for (int i(0); i < (int)node_list.size(); i++)
  {
    node_name_ = node_list[i].name.c_str();
    name_label = new QLabel(node_name_.c_str());
    Node_list.push_back(name_label);
    layout->addWidget(Node_list[i]);
    pid_list.push_back(node_list[i].pid);
  }

  layout->addSpacing(20);
  graph->addLayout(layout);

  /* create scene */
  Node_scene = new QGraphicsScene(this);
  ui->graphicsView->setScene(Node_scene);

  /* create view */
  Node_view = new QGraphicsView(Node_scene);
  Node_view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  graph->addWidget(Node_view);

  /* create buttons */
  QHBoxLayout *buttons = new QHBoxLayout;
  QPushButton *ZoomOut = new QPushButton("Zoom Out");
  QPushButton *ZoomIn = new QPushButton("Zoom In");
  buttons->addWidget(ZoomOut);
  buttons->addWidget(ZoomIn);
  buttons->addStretch();

  QObject::connect(ZoomOut, SIGNAL(clicked()), this, SLOT(zoom_out_process()));
  QObject::connect(ZoomIn, SIGNAL(clicked()), this, SLOT(zoom_in_process()));

  toplayout->addLayout(graph);
  toplayout->addLayout(buttons);
  toplayout->addStretch();
  groupBox->setLayout(toplayout);

  return groupBox;
}

QGroupBox *MainWindow::createNodeListGroup()
{
  QGroupBox *groupBox = new QGroupBox("Node List");
  QVBoxLayout *layout = new QVBoxLayout;
  std::string node_name_;
  QLabel *name_label;

  for (int i(0); i < (int)node_list.size(); i++)
  {
    node_name_ = node_list[i].name.c_str();
    name_label = new QLabel(node_name_.c_str());
    label_list.push_back(name_label);
    layout->addWidget(label_list[i]);
  }

  groupBox->setLayout(layout);

  return groupBox;
}

QGroupBox *MainWindow::createTextBrowser()
{
  QGroupBox *groupBox = new QGroupBox("Node Info");
  browser = new QTextBrowser;
  QVBoxLayout *layout = new QVBoxLayout;

  layout->addWidget(browser);
  browser->setText("Here, Node's informations are shown \n(e.g, PID, prio,...)");
  layout->addStretch();
  groupBox->setLayout(layout);

  return groupBox;
}

QGroupBox *MainWindow::createButtonGroup()
{
  QGroupBox *groupBox = new QGroupBox("Operations");
  QPushButton *StartStopButton = new QPushButton("Start / Stop");
  QPushButton *CPUNodeButton = new QPushButton("CPU / Node");
  QPushButton *NodeListButton = new QPushButton("View Node List");
  QPushButton *CloseButton = new QPushButton("Close");
  QHBoxLayout *layout = new QHBoxLayout;

  layout->addWidget(StartStopButton);
  layout->addWidget(CPUNodeButton);
  layout->addWidget(NodeListButton);
  layout->addWidget(CloseButton);
  layout->addStretch(1);
  groupBox->setLayout(layout);

  StartStopButton->setCheckable(true);
  CPUNodeButton->setCheckable(true);
  NodeListButton->setCheckable(true);

  QObject::connect(StartStopButton, SIGNAL(toggled(bool)), this, SLOT(StartStopTrace(bool)));
  QObject::connect(CPUNodeButton, SIGNAL(toggled(bool)), this, SLOT(changeCPUNode(bool)));
  QObject::connect(NodeListButton, SIGNAL(toggled(bool)), this, SLOT(ShowNodes(bool)));
  QObject::connect(CloseButton, SIGNAL(clicked()), this, SLOT(quit()));

  return groupBox;
}

void MainWindow::changeCPUNode(bool click)
{
  int zoom = 1;

  if (click)
  {
    mode = NODE_MODE;

    if (process_info.size() != 0)
    {
      delete_viz_process();
      viz_process(info, zoom);
    }

    CPUGroup->hide();
    NodeGroup->show();
  }
  else
  {
    mode = CPU_MODE;

    if (process_info.size() != 0)
    {
      delete_viz_process();
      viz_process(info, zoom);
    }

    NodeGroup->hide();
    CPUGroup->show();
  }
}

void MainWindow::StartStopTrace(bool click)
{
  if (click)
  {
    /* Preparation for second and subsequent procesing */
    if (process_info.size() != 0)
      delete_viz_process();

    /* Start tracing */
    SchedViz::Tracer tracer;
    tracer.setup(pass.toStdString());
    tracer.start_ftrace(pass.toStdString());
  }
  else
  {
    /* Get trace infomation */
    int zoom = 1;
    SchedViz::Tracer tracer;
    tracer.reset(pass.toStdString());
    info = tracer.get_info();
    viz_process(info, zoom);
  }
}

void MainWindow::ShowNodes(bool click)
{
  if (click)
  {
    NodeListGroup->show();
  }
  else
  {
    NodeListGroup->hide();
  }
}

void MainWindow::viz_process(std::vector<trace_info_t> info, double zoom)
{
  double base_time = info[0].start_time;
  double base_x = -50;
  double base_y = 12;
  double space = 38;
  double width = 0;
  double x = 0;
  double y = 0;
  int k = 0;
  QGraphicsScene *scene;
  ZOOM += zoom;

  switch (mode)
  {
    case CPU_MODE:
      scene = CPU_scene;

      /* Initialize geometry
       * y = upper of QGraphicsScene
       * */
      scene->addRect(0, 0, 1, 10, QPen(Qt::white), QBrush(Qt::white));

      /* Initialize geometry
       * y = bottom of QGraphicsScene
       * */
      scene->addRect(0, 7 * 39, 1, 10, QPen(Qt::white), QBrush(Qt::white));
      break;

    case NODE_MODE:
      scene = Node_scene;

      /* Initialize geometry
       * y = upper of QGraphicsScene
       * */
      Node_scene->addRect(0, 0, 1, 10, QPen(Qt::white), QBrush(Qt::white));

      /* Initialize geometry
       * y = bottom of QGraphicsScene
       * */
      Node_scene->addRect(0, (node_list.size() - 1) * 39, 1, 10, QPen(Qt::white), QBrush(Qt::white));
      break;

    default:
      scene = CPU_scene;
      break;
  }

  /* plot node */
  for (int i(0); i < (int)info.size(); i++)
  {
    x = info[i].start_time - base_time;
    width = info[i].runtime;

    switch (mode)
    {
      case CPU_MODE:
        y = base_y + (info[i].core * space);
        break;
      case NODE_MODE:
        double finish_time;
        double finish_time_prev;

        /* Classify nodes */
        for (int j(0); j < (int)node_list.size(); j++)
        {
          if (info[i].name == node_list[j].name)
            y = base_y + (j * space);
        }

#ifdef ANALYZE_DEADLINE_MISS
        /* analyze deadline miss */
        finish_time = info[i].start_time + info[i].runtime;
        finish_time_prev = info[i - 1].start_time + info[i - 1].runtime;

        if (i != 0 && finish_time - finish_time_prev > info[i].deadline)
        {
          deadline_miss = new QGraphicsLineItem();
          deadline_miss->setLine(x * ZOOM + base_x + info[i].runtime, y, x * ZOOM + base_x + info[i].runtime,
                                 y - 10); /* length of line */

          deadline_miss->setPen(QPen(Qt::red));
          deadline_miss_list.push_back(deadline_miss);
          scene->addItem(deadline_miss_list[k]);
          k++;
        }
#endif
        break;

      default:
        y = base_y;
    }

    square = new MySquare(x * ZOOM + base_x, y, width * ZOOM, info[i], browser, get_color(info[i].pid));

    process_info.push_back(square);
    scene->addItem(process_info[i]);
  }
}

QColor MainWindow::get_color(int pid)
{
  QColor color;

  for (int i(0); i < (int)label_list.size(); i++)
  {
    if (pid == pid_list[i])
      color = colors[i];
  }
  return color;
}

void MainWindow::zoom_in_process()
{
  delete_viz_process();
  viz_process(info, 500);
}

void MainWindow::zoom_out_process()
{
  delete_viz_process();

  if (ZOOM - 500 <= 0)
    viz_process(info, 0);
  else
    viz_process(info, -500);
}

void MainWindow::delete_viz_process()
{
  for (int i(0); i < (int)process_info.size(); i++)
  {
    delete process_info[i];
  }
  process_info.resize(0);

  for (int i(0); i < (int)deadline_miss_list.size(); i++)
  {
    delete deadline_miss_list[i];
  }

  deadline_miss_list.resize(0);
}

void MainWindow::quit()
{
  exit(1);
}

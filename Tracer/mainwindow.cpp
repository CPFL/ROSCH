#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tracer.h"
#include "mysquare.h"
#include <QTextBrowser>
#include <QDialog>
#include <QGroupBox>
#include <string>
#include <QtCore/QString>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QColor>

#define CORE 8

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){

  ui->setupUi(this);

  setGeometry(0,0,800,500);
  requirePass();

  setWindowTitle(tr("ROSHC: TRACER"));
  NodeGroup = createNodeGroup();

  QVBoxLayout *toplayout = new QVBoxLayout;
  toplayout->addWidget(createCPUGroup());
  toplayout->addWidget(NodeGroup);
  toplayout->addWidget(createTextBrowser());
  toplayout->addWidget(createButtonGroup());

  QWidget *window = new QWidget();
  window->setLayout(toplayout);
	ZOOM = 1000;

  NodeGroup->hide();
  setCentralWidget(window);
}

MainWindow::~MainWindow(){
	delete ui;
}

void MainWindow::requirePass(){
  passWindow = new QDialog;
  QLabel *label = new QLabel("pass: ");
  QLineEdit *edit = new QLineEdit;
  QPushButton *button = new QPushButton("ok");
  QHBoxLayout *layout = new QHBoxLayout;

  QObject::connect(edit, SIGNAL(textChanged(const QString &)),
			this, SLOT(setPass(const QString &)));
  QObject::connect(button,SIGNAL(clicked()),
			this,SLOT(onSetPass()));

  layout->addWidget(label);
  layout->addWidget(edit);
  layout->addWidget(button);
  passWindow->setWindowTitle("Password");
  passWindow->setLayout(layout);


  passWindow->show();
}

void MainWindow::setPass(const QString &input){
  if(input!=pass){
    pass = input;
  }
}

void MainWindow::onSetPass(){
  passWindow->hide();
}

QGroupBox *MainWindow::createCPUGroup(){
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

  scene = new QGraphicsScene(this);
  ui->graphicsView->setScene(scene);

  view = new QGraphicsView(scene);
  graph->addWidget(view);

	QHBoxLayout *buttons = new QHBoxLayout;
	QPushButton *ZoomOut = new QPushButton("Zoom Out");	
  QPushButton *ZoomIn = new QPushButton("Zoom In");
  buttons->addWidget(ZoomOut);
  buttons->addWidget(ZoomIn);
	buttons->addStretch();

  QObject::connect(ZoomOut, SIGNAL(clicked()),
				this, SLOT(zoom_out_process()));
	QObject::connect(ZoomIn, SIGNAL(clicked()),
				this, SLOT(zoom_in_process()));


  toplayout->addLayout(graph);
  toplayout->addLayout(buttons);
	toplayout->addStretch();
  groupBox->setLayout(toplayout);

  return groupBox;
}

QGroupBox *MainWindow::createNodeGroup(){
  QGroupBox *groupBox = new QGroupBox("Node");
  SchedViz::Tracer tracer;
  std::vector<node_info_t> node_list;
  std::string node_name_;
  QLabel *name_label;
		
  node_list = tracer.get_node_list();
  QVBoxLayout *layout = new QVBoxLayout;

  for(int i(0); i<(int)node_list.size();i++){
    node_name_ = node_list[i].name.c_str();
    name_label = new QLabel(node_name_.c_str());
    label_list.push_back(name_label);			
    layout->addWidget(label_list[i]);
    pid_list.push_back(node_list[i].pid); 
  }

  groupBox->setLayout(layout);

  return groupBox;
}

QGroupBox *MainWindow::createTextBrowser(){
  QGroupBox *groupBox = new QGroupBox("Node Info");
  browser = new QTextBrowser;
  QVBoxLayout *layout = new QVBoxLayout;

  layout->addWidget(browser);
  browser->setText("Here, Node's informations are shown \n(e.g, PID, prio,...)");
  layout->addStretch();
  groupBox->setLayout(layout);

  return groupBox;
}

QGroupBox *MainWindow::createButtonGroup(){
  QGroupBox *groupBox = new QGroupBox("Operations");
  QPushButton *StartStopButton = new QPushButton("Start / Stop");
  QPushButton *NodeListButton = new QPushButton("View Node List");
  QPushButton *CloseButton = new QPushButton("Close");
  QHBoxLayout *layout = new QHBoxLayout;

  layout->addWidget(StartStopButton);
  layout->addWidget(NodeListButton);
  layout->addWidget(CloseButton);
  layout->addStretch(1);
  groupBox->setLayout(layout);

  StartStopButton->setCheckable(true);
  NodeListButton->setCheckable(true);

  QObject::connect(StartStopButton, SIGNAL(toggled(bool)),
			this, SLOT(StartStopTrace(bool)));
	QObject::connect(NodeListButton, SIGNAL(toggled(bool)),
			this, SLOT(ShowNodes(bool)));
	QObject::connect(CloseButton, SIGNAL(clicked()),
			this, SLOT(quit()));

  return groupBox;
}

void MainWindow::StartStopTrace(bool click){

  if (click){

    /* Preparation for second and subsequent procesing */
		if(process_info.size() != 0){
      for(int i(0); i< (int)process_info.size();i++){
        delete process_info[i];
			}
	    process_info.resize(0);
		}

    /* Start tracing */
    SchedViz::Tracer tracer;
    tracer.setup(pass.toStdString());
    tracer.start_ftrace(pass.toStdString());
  }else{

		/* Get trace infomation */
		int zoom = 1;
    SchedViz::Tracer tracer;
    tracer.reset(pass.toStdString());
		info = tracer.get_info();
    viz_process(info,zoom);
  }
}

void MainWindow::ShowNodes(bool click){

  if(click){
    NodeGroup->show();
  }
  else{
    NodeGroup->hide();
  }
}


void MainWindow::viz_process(std::vector<trace_info_t> info, double zoom){
  double base_time = info[0].start_time;
  double base_x = -50;
  double base_y = 12;
	double space = 38;
  double width =0;
  double x=0;
  double y=0;
	ZOOM += zoom;


	/* Initialize geometry
	 * y = upper of QGraphicsScene 
	 * */
	scene->addRect(0, 0, 1, 10, QPen(Qt::white), QBrush(Qt::white));
	
	/* Initialize geometry
	 * y = bottom of QGraphicsScene 
	 * */
	scene->addRect(0, 7*39, 1, 10, QPen(Qt::white), QBrush(Qt::white));


	/* plot node */
  for(int i(0);i<(int)info.size();i++){
    x = info[i].start_time - base_time;
    width = info[i].runtime;
    y = base_y + (info[i].core * space);

		square = new MySquare(
				x*ZOOM+base_x,
				y,
				width*ZOOM,
				info[i],
				browser,
				get_color(info[i].pid)
				);
    
		process_info.push_back(square); 
    scene->addItem(process_info[i]);
  }
}

QColor MainWindow::get_color(int pid){
	QColor color;

	for(int i(0); i<(int)label_list.size();i++){
	  if(pid == pid_list[i])
			color = colors[i];
	}
	return color;
}

void MainWindow::zoom_in_process(){

	for(int i(0); i< (int)process_info.size();i++){
		delete process_info[i];
	}
	process_info.resize(0);
	viz_process(info,500);

}

void MainWindow::zoom_out_process(){

	for(int i(0); i< (int)process_info.size();i++){
		delete process_info[i];
	}
	process_info.resize(0);
	
	if(ZOOM - 500 <= 0)
		viz_process(info,0);
	else
		viz_process(info,-500);
}

void MainWindow::quit(){
  exit(1);
}


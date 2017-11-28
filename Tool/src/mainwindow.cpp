#include "mainwindow.h"
#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTextBlock>
#include "create_file.h"
#include "type.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  setWindowTitle(tr("Parser"));
  mode_flag = 0;

  Mode = createMode();
  NodeListViewer = createNodeListViewer();
  Previewer = createPreviewer();
  Creator = createCreator();

  /* top window */
  QVBoxLayout *topLayout = new QVBoxLayout;
  topLayout->addWidget(Mode);
  topLayout->addWidget(NodeListViewer);
  topLayout->addWidget(Previewer);
  topLayout->addWidget(Creator);
  QWidget *window = new QWidget();
  window->setLayout(topLayout);

  setCentralWidget(window);
}

MainWindow::~MainWindow()
{
  delete ui;
}

QGroupBox *MainWindow::createMode()
{
  QGroupBox *groupBox = new QGroupBox("1. Select Mode");
  QVBoxLayout *toplayout = new QVBoxLayout;

  QPushButton *measurer = new QPushButton("Measurer");
  QPushButton *analyzer = new QPushButton("Analyzer");
  QPushButton *scheduler = new QPushButton("Scheduler");
  QPushButton *tracer = new QPushButton("Tracer");

  measurer->setCheckable(true);
  analyzer->setCheckable(true);
  scheduler->setCheckable(true);
  tracer->setCheckable(true);

  QHBoxLayout *modeLayout = new QHBoxLayout;
  modeLayout->addWidget(measurer);
  modeLayout->addWidget(analyzer);
  modeLayout->addWidget(scheduler);
  modeLayout->addWidget(tracer);

  QButtonGroup *ExclusiveButtons = new QButtonGroup;
  ExclusiveButtons->addButton(measurer);
  ExclusiveButtons->addButton(analyzer);
  ExclusiveButtons->addButton(scheduler);
  ExclusiveButtons->addButton(tracer);
  ExclusiveButtons->setExclusive(true);

  toplayout->addLayout(modeLayout);
  groupBox->setLayout(toplayout);

  QObject::connect(measurer, SIGNAL(clicked()), this, SLOT(measurer_mode()));
  QObject::connect(analyzer, SIGNAL(clicked()), this, SLOT(analyzer_mode()));
  QObject::connect(scheduler, SIGNAL(clicked()), this, SLOT(scheduler_mode()));
  QObject::connect(tracer, SIGNAL(clicked()), this, SLOT(tracer_mode()));

  return groupBox;
}

QGroupBox *MainWindow::createNodeListViewer()
{
  QGroupBox *groupBox = new QGroupBox("2. Check Topics");
  QVBoxLayout *toplayout = new QVBoxLayout;

  widget = new QListWidget;
  QPushButton *getNodeList = new QPushButton("Refresh");
  toplayout->addWidget(getNodeList);
  toplayout->addWidget(widget);
  groupBox->setLayout(toplayout);

  QObject::connect(getNodeList, SIGNAL(clicked()), this, SLOT(refresch_node_list()));

  return groupBox;
}

QGroupBox *MainWindow::createPreviewer()
{
  QGroupBox *groupBox = new QGroupBox("3. Preview Topics Dependancies");
  QVBoxLayout *toplayout = new QVBoxLayout;

  QPushButton *previewer = new QPushButton("Preview");
  toplayout->addWidget(previewer);
  groupBox->setLayout(toplayout);

  QObject::connect(previewer, SIGNAL(clicked()), this, SLOT(preview_topics_depend()));

  return groupBox;
}

QGroupBox *MainWindow::createCreator()
{
  QGroupBox *groupBox = new QGroupBox("4. Create YAML File");
  QVBoxLayout *toplayout = new QVBoxLayout;

  QPushButton *okButton = new QPushButton("Create yaml");
  toplayout->addWidget(okButton);
  QLabel *my_label = new QLabel;
  my_label->setText("Note that this only creates stationery files.\nSo you need to change some values initialized to "
                    "\"0\".");
  my_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
  toplayout->addWidget(my_label);
  groupBox->setLayout(toplayout);

  QObject::connect(okButton, SIGNAL(clicked()), this, SLOT(create_file()));

  return groupBox;
}

void MainWindow::highlightChecked(QListWidgetItem *item)
{
  if (item->checkState() == Qt::Checked)
  {
    int index;
    DependInfo depend_info;
    node_info_t element;
    item->setBackgroundColor(QColor("#ffffb2"));
    index = item->listWidget()->row(item);
    depend_info = parser.get_depend_info(topic_list.at(index));

    element.name = (topic_list.at(index));
    element.index = index;
    element.core = 0;     /* template value */
    element.runtime = 0;  /* template value */
    element.deadline = 0; /* template value */
    element.depend = depend_info;

    SchedInfo sched;
    sched.core = 0;     /* template value */
    sched.priority = 0; /* template value */
    sched.runtime = 0;  /* template value */
    element.sched_info.push_back(sched);

    node_info.push_back(element);
  }
  else
  {
    int index;
    item->setBackgroundColor(QColor("#ffffff"));
    index = item->listWidget()->row(item);

    for (int i(0); i < (int)node_info.size(); i++)
    {
      if (node_info[i].index == index)
      {
        node_info.erase(node_info.begin() + i);
      }
    }
  }
}

void MainWindow::refresch_node_list()
{
  widget->clear();
  node_info.clear();
  topic_list.clear();
  topic_list = parser.get_node_list();

  QStringList strList;
  for (int i(0); i < (int)topic_list.size(); i++)
  {
    QString qstr = QString::fromStdString(topic_list.at(i));
    strList << qstr;
  }

  widget->addItems(strList);

  QListWidgetItem *item = 0;
  for (int i = 0; i < widget->count(); ++i)
  {
    item = widget->item(i);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
  }

  QObject::connect(widget, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(highlightChecked(QListWidgetItem *)));
}

void MainWindow::preview_topics_depend()
{
  parser.preview_topics_depend(node_info);
}

void MainWindow::create_file()
{
  parser.create_file(node_info, mode_flag);
}

void MainWindow::measurer_mode()
{
  mode_flag = MEASURER;
}
void MainWindow::analyzer_mode()
{
  mode_flag = ANALYZER;
}
void MainWindow::scheduler_mode()
{
  mode_flag = SCHEDULER;
}
void MainWindow::tracer_mode()
{
  mode_flag = TRACER;
}

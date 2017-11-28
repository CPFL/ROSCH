#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <string>
#include <vector>
#include "create_file.h"

#define MEASURER 1
#define ANALYZER 2
#define SCHEDULER 3
#define TRACER 4

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

  QGroupBox *Mode;
  QGroupBox *NodeListViewer;
  QGroupBox *Previewer;
  QGroupBox *Creator;
  int mode_flag;
  std::vector<std::string> topic_list;
  std::vector<node_info_t> node_info;

public slots:
  void highlightChecked(QListWidgetItem *item);
  void create_file();
  void refresch_node_list();
  void measurer_mode();
  void analyzer_mode();
  void scheduler_mode();
  void tracer_mode();
  void preview_topics_depend();

private:
  Ui::MainWindow *ui;

  QListWidget *mode_widget;
  QListWidget *widget;

  QGroupBox *createMode();
  QGroupBox *createNodeListViewer();
  QGroupBox *createPreviewer();
  QGroupBox *createCreator();
  void createListWidget();
  ROSCH::Parser parser;
};

#endif  // MAINWINDOW_H

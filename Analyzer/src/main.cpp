#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "node.h"
#include "node_graph.h"
#include "node_graph_core.h"
#include "opencv2/opencv.hpp"
#include "sched_analyzer.h"

static const int WIDTH = 1200;
static const int HEIGHT = 900;
static const int LEFT_CORE_PAD = 50;
int periodic_count;

cv::Scalar set_colors(int index)
{
  double R, G, B;
  R = G = B = 200;
  R -= g[desc[index]].period_count * 60 % 256;  // 100
  G -= g[desc[index]].period_count * 80 % 256;  // 35
  B -= g[desc[index]].period_count * 90 % 256;  // 160
  return cv::Scalar(R, G, B);
}

int main(int argc, char* argv[])
{
  switch (argc)
  {
    case 1:
      periodic_count = 1;
      break;
    case 2:
      periodic_count = atoi(argv[1]);
      break;
    default:
      std::cout << "Usage: "
                << " periodic" << std::endl;
      exit(1);
  }
  std::cout << "start Sched Analyzer" << std::endl;

  sched_analyzer::SchedAnalyzer analyzer;
  analyzer.run();
  analyzer.show_sched_cpu_tasks();
  std::vector<sched_analyzer::V_sched_node> cpu_taskset;
  analyzer.get_cpu_taskset(cpu_taskset);
  int core = analyzer.get_spec_core();

  cv::Mat image(cv::Size(WIDTH, HEIGHT), CV_8UC3, cv::Scalar(255, 255, 255));
  for (int i(0); i < core; ++i)
  {
    int core_line = HEIGHT / (core + 1);
    cv::line(image, cv::Point(LEFT_CORE_PAD, core_line * (i + 1)), cv::Point(WIDTH, core_line * (i + 1)),
             cv::Scalar(0, 0, 0), 3, 4);
    cv::putText(image, std::string("core" + std::to_string(i)), cv::Point(0, core_line * (i + 1)),
                cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 0), 1, CV_AA);
  }
  int mag_ratio = WIDTH / (analyzer.get_makespan() * 1.1);
  for (int i(0); i < core; ++i)
  {
    for (int j(0); (size_t)j < cpu_taskset.at(i).size(); ++j)
    {
      int core_line = HEIGHT / (core + 1);
      if (cpu_taskset.at(i).at(j).empty)
        continue;
      int index = (cpu_taskset.at(i).at(j).node_index);
      int start = (cpu_taskset.at(i).at(j).start_time);
      int end = (cpu_taskset.at(i).at(j).end_time);
      int start_mag = (start)*mag_ratio + LEFT_CORE_PAD;
      int end_mag = (end)*mag_ratio + LEFT_CORE_PAD;
      cv::Scalar colors = set_colors(index);

      cv::rectangle(image, cv::Point(start_mag, core_line * (i + 1) - core_line / 2),
                    cv::Point(end_mag, core_line * (i + 1)), colors, -1, 8);
      cv::rectangle(image, cv::Point(start_mag, core_line * (i + 1) - core_line / 2),
                    cv::Point(end_mag, core_line * (i + 1)), cv::Scalar(0, 0, 0), 2, 8);
      cv::putText(image, std::string(std::to_string(start)), cv::Point(start_mag - 5, core_line * (i + 1) + 15),
                  cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 0), 1, CV_AA);
      cv::putText(image, std::string(std::to_string(end)), cv::Point(end_mag - 5, core_line * (i + 1) + 15),
                  cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 0), 1, CV_AA);
      cv::putText(image, std::string(g[desc[index]].name_p),
                  cv::Point((start_mag + end_mag) / 2 - 5, core_line * (i + 1) - ((core_line / 2) + 5)),
                  cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 255), 1, CV_AA);
    }
  }
  std::ofstream ofs_inform_;
  ofs_inform_.open("test.txt", std::ios::app);

  for (int i(0); i < analyzer.get_node_list_size(); ++i)
  {
    std::string node_name;
    analyzer.get_node_name(i, node_name);
    ofs_inform_ << i << ":" << node_name << std::endl;
  }
  ofs_inform_.close();

  cv::imshow("Show image", image);
  cv::imwrite("test.jpg", image);
  cv::waitKey(0);

  // analyzer.show_tree_dfs();
  // analyzer.show_leaf_list();

  return 0;
}

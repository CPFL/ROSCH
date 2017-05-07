#ifndef ANALYZER_HPP
#define ANALYZER_HPP
#include "ros_rosch/core_count_manager.hpp"
#include "ros_rosch/exec_time.hpp"
#include "ros_rosch/node_graph.hpp"
#include <fstream>
#include <string>

namespace rosch {
class Analyzer : ExecTime {
public:
  explicit Analyzer(const std::string &node_name, const std::string &topic,
                    const unsigned int &max_times = 1000,
                    const unsigned int &ignore_times = 50);
  ~Analyzer();
  void start_time();    //
  void end_time();      //
  void finish_myself(); //
  unsigned int get_counter();
  double get_max_time_ms();
  double get_min_time_ms();
  double get_exec_time_ms();
  int get_target_index();
  bool is_in_range();
  bool is_target();
  bool is_target_topic();
  bool is_target_node();
  bool is_in_node_graph();
  void set_rt();
  void set_fair();
  std::string get_node_name();
  std::string get_topic_name();
  void update_graph();
  bool check_need_refresh();
  void core_refresh();

private:
  SingletonNodeGraphAnalyzer *graph_analyzer_;
  SingletonCoreCountManager *core_count_manager_;
  void open_output_file(bool init);
  bool set_affinity(int core);
  unsigned int max_analyze_times_;
  unsigned int ignore_times_;
  unsigned int counter_;
  double max_ms_;
  double min_ms_;
  double average_ms_;
  std::string node_name_;
  std::string topic_;
  bool is_aleady_rt_;
  std::ofstream ofs_;
  int core_;
};
}

#endif // ANALYZER_HPP

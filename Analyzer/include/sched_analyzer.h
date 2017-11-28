#ifndef SCHED_ANALYZER_H
#define SCHED_ANALYZER_H
#include <vector>
#include "config.h"
#include "node.h"
#include "node_graph.h"
#include "spec.h"

namespace sched_analyzer
{
struct sched_node_t
{
  bool empty;
  int run_time;
  int node_index;
  int esc_time;
  int start_time;
  int end_time;
};

typedef std::vector<sched_node_t> V_sched_node;

class SchedAnalyzer
{
public:
  SchedAnalyzer();
  ~SchedAnalyzer();
  int run();
  bool is_schedulable(int deadline);
  int get_makespan();
  int get_spec_core();
  int show_sched_cpu_tasks();
  spec_t get_spec();
  int get_cpu_taskset(std::vector<V_sched_node> &v_sched_cpu_task);
  int get_node_list_size();
  int get_node_name(int index, std::string &node_name);

private:
  typedef struct sort_esc_time_t
  {
    int core_index;
    int esc_time;
    bool operator<(const sort_esc_time_t &right) const
    {
      return esc_time < right.esc_time;
    }
  } sort_inv_laxity_t;
  typedef struct sched_node_info_t
  {
    int start_time;
    int core_index;
    bool need_empty;
  } sched_node_info_t;
  typedef struct sched_v_node_info_t
  {
    int start_time;
    int core_index;
    int empty_index;
    bool need_before_empty;
    bool need_after_empty;
  } sched_v_node_info_t;

  void load_spec_(const std::string &filename);
  int get_cpu_status(std::vector<sort_esc_time_t> &v_cpu_status);
  int get_min_start_time(int index, int &min_start_time);
  int get_sched_node_info(const int min_start_time, sched_node_info_t &sched_node_info);
  int compute_makespan(int &makepan);
  int create_empty_node(const int empty_start_time, const int empty_end_time, sched_node_t &sched_empty_node);
  int create_sched_node(const int index, const int start_time, const int run_time, sched_node_t &sched_node);
  int get_sched_vacancy_node_info(const int run_time, const int min_start_time,
                                  sched_v_node_info_t &sched_vacancy_node_info);
  int set_sched_vacancy_node(const int index, const int run_time, const sched_v_node_info_t &sched_vacancy_node_info);
  int get_sched_node_info_in_selected_cores(const int min_start_time, sched_node_info_t &sched_node_info,
                                            std::vector<int> v_can_use_core);
  int get_sched_vacancy_node_info_in_selected_cores(const int run_time, const int min_start_time,
                                                    sched_v_node_info_t &sched_vacancy_node_info,
                                                    std::vector<int> v_can_use_core);
  void get_can_use_core(const std::vector<int> &v_used_core, std::vector<int> *v_can_use);
  SingletonNodeGraphAnalyzer &node_graph_analyzer_;
  std::vector<V_sched_node> v_sched_cpu_task_;
  std::vector<node_t> node_queue_;
  std::vector<Node_P> Queue;
  Config config_;
  spec_t spec_;  // Hardware spec
  int makespan_;
};
}
#endif  // SCHED_ANALYZER_H

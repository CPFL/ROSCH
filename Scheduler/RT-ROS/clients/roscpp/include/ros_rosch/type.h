#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>

typedef struct SchedInfo {
  int core;
  int priority;
  int run_time;
  int start_time;
  int end_time;
} SchedInfo;

typedef struct NodeInfo {
  std::string name;
  int index;
  int core;
  std::vector<SchedInfo> v_sched_info;
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
	bool is_single_process;
	int period_count;
} NodeInfo;

#endif // TYPE_H

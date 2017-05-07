#ifndef TASK_ATTRIBUTE_PROCESSER_H
#define TASK_ATTRIBUTE_PROCESSER_H

#include <sched.h>
#include <vector>

namespace rosch {
class TaskAttributeProcesser {
public:
  TaskAttributeProcesser(){};
  ~TaskAttributeProcesser(){};
  void setRealtimePriority(std::vector<pid_t> v_pid, int prio);
  bool setCoreAffinity(std::vector<int> v_core);
  bool setAffinityToAllCore();
  void setCFS(std::vector<pid_t> v_pid);
  void setDefaultScheduling(std::vector<pid_t> v_pid);
};
}

#endif

#define _GNU_SOURCE 1
#include "ros_rosch/task_attribute_processer.h"
#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <vector>

using namespace rosch;

void TaskAttributeProcesser::setRealtimePriority(std::vector<pid_t> v_pid,
                                                 int prio) {
  if (prio > 0) {
    std::cout << "setRealtimePriority:" << prio << std::endl;
    struct sched_param sp;
    sp.sched_priority = prio;
    for (int i = 0; i < (int)v_pid.size(); ++i) {
      sched_setscheduler(v_pid.at(i), SCHED_FIFO, &sp);
    }
  } else {
    std::cerr << "Failed to setRealtimePriority:" << prio << std::endl;
  }
}
void TaskAttributeProcesser::setCFS(std::vector<pid_t> v_pid) {
  struct sched_param sp;
  sp.sched_priority = 0;
  for (int i = 0; i < (int)v_pid.size(); ++i) {
    sched_setscheduler(v_pid.at(i), SCHED_OTHER, &sp);
  }
}
bool TaskAttributeProcesser::setCoreAffinity(std::vector<int> v_core) {
  if (v_core.size() == 0) {
    std::cerr << "Failed to set CPU affinity" << std::endl;
    return false;
  }
  cpu_set_t mask;
  CPU_ZERO(&mask);
  std::cout << "setCoreAffinity:";
  for (int i = 0; i < (int)v_core.size(); ++i) {
    std::cout << v_core.at(i) << ",";
    CPU_SET(v_core.at(i), &mask);
  }
  std::cout << std::endl;
  if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
    std::cerr << "Failed to set CPU affinity" << std::endl;
    return false;
  }
  return true;
}
bool TaskAttributeProcesser::setAffinityToAllCore() {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  for (int i = 0; i < sysconf(_SC_NPROCESSORS_CONF); ++i) {
    CPU_SET(i, &mask);
  }
  if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
    std::cerr << "Failed to set CPU affinity" << std::endl;
    return false;
  }
  return true;
}
void TaskAttributeProcesser::setDefaultScheduling(std::vector<pid_t> v_pid) {
  setAffinityToAllCore();
  setCFS(v_pid);
}

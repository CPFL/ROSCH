#ifndef TRACER_H
#define TRACER_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "config.h"
#include "yaml-cpp/yaml.h"

typedef struct node_info_t
{
  std::string name;
  unsigned int pid;
  double deadline;
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
} node_info_t;

typedef struct trace_info_t
{
  std::string name;
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
  unsigned int pid;
  int core;
  double runtime;
  double start_time;
  int prio;
  double deadline;

  bool operator<(const trace_info_t& another) const
  {
    return start_time < another.start_time;
  }
} trace_info_t;

namespace SchedViz
{
class Tracer
{
public:
  Tracer();
  ~Tracer();
  void setup(std::string);
  void reset(std::string);
  void start_ftrace(std::string);

  std::vector<trace_info_t> get_info();
  std::vector<node_info_t> get_node_list();
  std::vector<node_info_t> v_node_info_;
  std::vector<trace_info_t> v_trace_info;

private:
  void load_config_(const std::string& filename);
  Config config_;

  unsigned int get_pid(std::string name);
  void mount(bool, std::string);
  void set_tracing_on(int mode, std::string);
  void set_trace(int mode, std::string);
  void set_events_enable(int mode, std::string);
  void set_current_tracer(std::string, std::string);
  void set_event(std::string, std::string);
  void output_log(std::string);
  void filter_pid(bool mode, std::string);
  void extract_period();
  void create_process_info(std::vector<std::string> find_prev_pids, std::vector<std::string> find_next_pids);
  std::vector<std::string> split(std::string str, std::string delim);
  std::string trim(const std::string& string);
  int ctoi(std::string s);
  unsigned int get_topic(char* line);
};
}

#endif  // TRACER_H

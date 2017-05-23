#ifndef TRACER_H
#define TRACER_H

#include <stdlib.h> 
#include <string>
#include "config.h"
#include <vector>
#include "yaml-cpp/yaml.h"

typedef struct node_info_t {
  std::string name;
	unsigned int pid;
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
} node_info_t;


namespace SchedViz{
class Tracer{
public:
	Tracer();
	~Tracer();	
	FILE *fp;
	void setup();
	void reset();
	void start_ftrace();

private:
	void load_config_(const std::string &filename);
	Config config_;
	std::vector<node_info_t> v_node_info_;
	unsigned int get_pid(std::string name);


	void set_tracing_on(char *mode);
	void set_trace(char *mode);
	void set_events_enable(char *mode);
	void set_current_tracer(char *mode);
	void set_event(char *mode);
	void output_log();
	void filter_pid(bool mode);


};
}

#endif

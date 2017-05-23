#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <time.h>
#include "tracer.h"
#include "string"
#include "yaml-cpp/yaml.h"



using namespace SchedViz;

Tracer::Tracer() : config_(){
	std::string filename(config_.get_configpath());
	load_config_(filename);
}

Tracer::~Tracer(){}

void Tracer::load_config_(const std::string &filename) {
  std::cout << filename << std::endl;

  try {
    YAML::Node node_list;
    node_list = YAML::LoadFile(filename);
    
		for (unsigned int i(0); i < node_list.size(); i++) {
      const YAML::Node node_name = node_list[i]["nodename"];
      const YAML::Node node_subtopic = node_list[i]["sub_topic"];
      const YAML::Node node_pubtopic = node_list[i]["pub_topic"];
			node_info_t node_info;
      node_info.name = node_name.as<std::string>();

			node_info.v_subtopic.resize(0);
      for (int j(0); (size_t)j < node_subtopic.size(); ++j) {
        node_info.v_subtopic.push_back(node_subtopic[j].as<std::string>());
      }

      node_info.v_pubtopic.resize(0);
      for (int j(0); (size_t)j < node_pubtopic.size(); ++j) {
        node_info.v_pubtopic.push_back(node_pubtopic[j].as<std::string>());
      }

			node_info.pid = get_pid(node_info.name);
      v_node_info_.push_back(node_info);
    }

  } catch (YAML::Exception &e) {
    std::cerr << e.what() << std::endl;
  }
}


unsigned int Tracer::get_pid(std::string name){
	FILE *pid_fp;
	unsigned int pid;
	char buf[1024];
	std::string pidof_x = "pidof ";
	pidof_x += name;

	if ((pid_fp=popen(pidof_x.c_str(),"r")) ==NULL)
		exit(1);
	
	while(fgets(buf, sizeof(buf), pid_fp) != NULL)
		pid = atoi(buf);

	std::cout << name.c_str() << ": " << pid <<std::endl;
	pclose(pid_fp);
	
	return pid;
}

void Tracer::setup(){
	system("mount -t debugfs nodev /sys/kernel/debug/");
	Tracer::filter_pid(true);
	Tracer::set_tracing_on((char*)"0");
	Tracer::set_trace((char*)"0");
	Tracer::set_events_enable((char*)"1");
	Tracer::set_event((char*)"sched:sched_switch");
	Tracer::set_current_tracer((char*)"function"); 
}

void Tracer::reset(){
	Tracer::set_tracing_on((char*)"0"); 
	Tracer::set_events_enable((char*)"0");
	Tracer::output_log();
	Tracer::filter_pid(false); 
	system("umount /sys/kernel/debug/");
}

void Tracer::set_tracing_on(char *mode){
	std::ofstream _set_tracing_on("/sys/kernel/debug/tracing/tracing_on");
	_set_tracing_on << mode << std::endl;
	if(_set_tracing_on.fail()){
		std::cerr << "open error" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}
}

void Tracer::set_trace(char *mode){
	std::ofstream _set_trace("/sys/kernel/debug/tracing/trace");
	_set_trace << mode << std::endl;
	if(_set_trace.fail()){
		std::cerr << "open error" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}
}

void Tracer::set_events_enable(char *mode){
	std::ofstream _set_events_enable("/sys/kernel/debug/tracing/events/enable");
	_set_events_enable << mode << std::endl;
	if(_set_events_enable.fail()){
		std::cerr << "open error" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}
}

void Tracer::set_current_tracer(char *mode){
	std::ofstream _set_current_tracer("/sys/kernel/debug/tracing/current_tracer");
	_set_current_tracer << mode << std::endl;
	if(_set_current_tracer.fail()){
		std::cerr << "open error" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}
}

void Tracer::set_event(char *mode){
	std::ofstream _set_event("/sys/kernel/debug/tracing/set_event");
	_set_event << mode << std::endl;
	if(_set_event.fail()){
		std::cerr << "open error" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}
}

void Tracer::start_ftrace(){
	std::cout << "Trace start " << std::endl; 
	std::ofstream _set_ftrace("/sys/kernel/debug/tracing/tracing_on");
	_set_ftrace << 1 << std::endl; 
}

void Tracer::output_log(){
	std::ifstream _trace_log("/sys/kernel/debug/tracing/trace");
	//std::ifstream _trace_log("/sys/kernel/debug/tracing/trace_pipe"); 
	std::ofstream trace_log("./trace.log");
	std::string str;

	std::cout << "Trace finish"<< str << std::endl;

	if(_trace_log.fail()){
		std::cerr << "open error" << std::endl;
 		system("umount /sys/kernel/debug/");
		exit(1);
	}
	while(getline(_trace_log,str))
		trace_log << str << std::endl;

}

void Tracer::filter_pid(bool mode){
	
	std::ofstream _filter_pid("/sys/kernel/debug/tracing/set_ftrace_pid", std::ios::out | std::ios::app);

	if(mode == false){
		_filter_pid << "" << std::endl;
	}
	else{
		for (int i(0); i < (int)v_node_info_.size(); ++i) {
			_filter_pid << v_node_info_.at(i).pid << std::endl;
		}
	}

	if(_filter_pid.fail()){
		std::cerr << "open error filter" << std::endl;
		system("umount /sys/kernel/debug/");
		exit(1);
	}

}


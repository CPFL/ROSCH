#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "tracer.h"
#include "string"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <sstream>

using namespace SchedViz;

Tracer::Tracer() : config_(){
  std::string filename(config_.get_configpath());
  load_config_(filename);
}

Tracer::~Tracer(){}

void Tracer::load_config_(const std::string &filename) {
  
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
  unsigned int pid=0;
  char buf[1024];
  std::string pidof_x = "pidof ";
  pidof_x += name;

  if ((pid_fp=popen(pidof_x.c_str(),"r")) ==NULL)
    exit(1);

  while(fgets(buf, sizeof(buf), pid_fp) != NULL)
    pid = atoi(buf);

    pclose(pid_fp);

    return pid;
}

void Tracer::setup(std::string userPass){
  Tracer::mount(true,userPass);
  Tracer::set_tracing_on(0, userPass);
  Tracer::set_trace(0,userPass);
  Tracer::set_events_enable(1,userPass);
  Tracer::filter_pid(true,userPass);
  Tracer::set_event((std::string)"sched:sched_switch",userPass);
  Tracer::set_current_tracer((std::string)"function",userPass);
}

void Tracer::reset(std::string userPass){
  Tracer::set_tracing_on(0,userPass);
  Tracer::set_events_enable(0,userPass);
  Tracer::output_log(userPass);
  Tracer::filter_pid(false,userPass);
  Tracer::mount(false,userPass);
  Tracer::extract_period();
}

void Tracer::mount(bool mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com;

  switch (mode){
    case true:
      second_com = "| sudo -S mount -t debugfs nodev /sys/kernel/debug/"; break;
    case false:
      second_com = "| sudo -S umount /sys/kernel/debug/"; break;
    default:
      break;
  }

  first_com += (userPass + second_com);
  system(first_com.c_str());

}

void Tracer::set_tracing_on(int mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com;

  switch (mode){
    case 0:
      second_com = " | sudo -S sh -c \"echo \'0\' > /sys/kernel/debug/tracing/tracing_on\"";
      first_com += (userPass + second_com); break;
    case 1:
      second_com = " | sudo -S sh -c \"echo \'1\' > /sys/kernel/debug/tracing/tracing_on\"";
      first_com += (userPass + second_com); break;
    default:
      break;
  }
  system(first_com.c_str());
}

void Tracer::set_trace(int mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com;

  switch (mode){
    case 0:
      second_com = " | sudo -S sh -c \"echo \'0\' > /sys/kernel/debug/tracing/trace\"";
      first_com += (userPass + second_com); break;
    case 1:
      second_com = " | sudo -S sh -c \"echo \'1\' > /sys/kernel/debug/tracing/trace\"";
      first_com += (userPass + second_com); break;
    default:
      break;
  }
  system(first_com.c_str());
}

void Tracer::set_events_enable(int mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com;

  switch (mode){
    case 0:
      second_com = " | sudo -S sh -c \"echo \'0\' > /sys/kernel/debug/tracing/events/enable\"";
      first_com += (userPass + second_com); break;
    case 1:
      second_com = " | sudo -S sh -c \"echo \'1\' > /sys/kernel/debug/tracing/events/enable\"";
      first_com += (userPass + second_com); break;
    default:
      break;
  }
  system(first_com.c_str());
}

void Tracer::set_current_tracer(std::string mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com = " | sudo -S sh -c \"echo \'";
  std::string third_com = "\' > /sys/kernel/debug/tracing/current_tracer\"";

  first_com += (userPass + second_com + mode + third_com);

  system(first_com.c_str());
}

void Tracer::set_event(std::string mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com = " | sudo -S sh -c \"echo \'";
  std::string third_com = "\' > /sys/kernel/debug/tracing/set_event\"";

  first_com += (userPass + second_com + mode + third_com);
  system(first_com.c_str());

}

void Tracer::start_ftrace(std::string userPass){
  std::string start_com = "echo ";
  std::string second_com = "| sudo -S sh -c \"echo \'1\' > /sys/kernel/debug/tracing/tracing_on\"";
  start_com += (userPass + second_com);
  system(start_com.c_str());
}

void Tracer::output_log(std::string userPass){
  std::string first_com = "echo ";
  std::string second_com = "| sudo -S sh -c \"cat /sys/kernel/debug/tracing/trace > ./ftrace.log\"";
  first_com += userPass + second_com;
  system(first_com.c_str());
}

void Tracer::filter_pid(bool mode, std::string userPass){
  std::string first_com = "echo ";
  std::string second_com = " | sudo -S sh -c \"echo \'";
  std::string third_com = "\' > /sys/kernel/debug/tracing/set_ftrace_pid\"";
  std::ostringstream pids;

  // Initialize pids
  pids.str(""); 
  pids.clear(std::stringstream::goodbit);
  first_com += (userPass + second_com);
  first_com += (pids.str() + third_com);
  system(first_com.c_str());

	
  if(mode == true){
    first_com = "echo ";
   		
    // loop pid's number times
    for (int i(0); i < (int)v_node_info_.size(); ++i){
      pids <<  v_node_info_.at(i).pid;
      first_com += (userPass + second_com);
      first_com += (pids.str() + third_com);
			
      pids.str("");
      pids.clear(std::stringstream::goodbit);

      system(first_com.c_str());
      first_com = "echo ";
    }

  }else{
    system(first_com.c_str());
  }
}


void Tracer::extract_period(){
  std::vector<std::string> find_prev_pids;
  std::vector<std::string> find_next_pids;
  std::string _prev_pid = "prev_pid=";
  std::string _next_pid = "next_pid=";
  std::stringstream prev_pid, next_pid;

  // Initialize find pid list
  for (int i(0); i < (int)v_node_info_.size(); ++i) {

    // Clear buffer
    prev_pid.str("");
    next_pid.str("");
    prev_pid.clear(std::stringstream::goodbit);
    next_pid.clear(std::stringstream::goodbit);

    prev_pid << _prev_pid << v_node_info_.at(i).pid;
    next_pid << _next_pid << v_node_info_.at(i).pid;
    find_prev_pids.push_back(prev_pid.str());
    find_next_pids.push_back(next_pid.str());
  }

create_process_info(find_prev_pids, find_next_pids);

}


void Tracer::create_process_info(
        std::vector<std::string> find_prev_pid,
        std::vector<std::string> find_next_pid){

	std::string buf;
  std::string delim = " \t\v\r\n";
  std::vector<std::string> start_time_s, finish_time_s;
  double start_time_i = 0;
  double finish_time_i = 0;
  trace_info_t trace_info;

  // Loop by the number of pid
  for (int i(0); i < (int)find_prev_pid.size();i++){
    std::ifstream _trace_log("./ftrace.log");

    while(std::getline(_trace_log,buf)){

      // start time
      if(buf.find(find_next_pid.at(i)) != -1){
			  start_time_s = split(trim(buf),delim);
				trace_info.prio = stoi(start_time_s[11].substr(10));
				start_time_s = split(trim(buf),delim);
        start_time_i = strtod(start_time_s[2].c_str(),NULL); //index of array is 2 or 3
        trace_info.name = find_next_pid.at(i).substr(9); //pid
        trace_info.pid = std::stoi(find_next_pid.at(i).substr(9)); //pid
				trace_info.start_time = start_time_i;
        trace_info.core = ctoi(start_time_s[0]);

				for(int j(0); j<(int)v_node_info_.size();j++){
				  if(trace_info.pid ==  v_node_info_.at(i).pid){
					  trace_info.name = v_node_info_.at(i).name;
						trace_info.v_subtopic = v_node_info_.at(i).v_subtopic;
						trace_info.v_pubtopic = v_node_info_.at(i).v_pubtopic; 
					}
				}
      }

      // end time
      if(buf.find(find_prev_pid.at(i)) != -1){
        finish_time_s = split(trim(buf),delim);
        finish_time_i = strtod(finish_time_s[2].c_str(),NULL); //index of array is 2 or 3
        trace_info.runtime = finish_time_i - start_time_i;
        v_trace_info.push_back(trace_info);
      }
    }
  }

  //sort by start_time
  std::sort(v_trace_info.begin(),v_trace_info.end());

#ifdef PRINT_DEBUG
  for(int i=0;i<(int)v_trace_info.size();i++){
    std::cout<< v_trace_info[i].name << " " << v_trace_info[i].pid;
    printf(" core: %d, s_time: %f. r_time: %f\n"
				,v_trace_info[i].core
        ,v_trace_info[i].start_time
				, v_trace_info[i].runtime);
  }
#endif
}


 // Separate trace.log by spaces
std::vector<std::string> Tracer::split(std::string str, std::string delim) {
  std::vector<std::string> items;
  std::size_t dlm_idx;
  if(str.npos == (dlm_idx = str.find_first_of(delim))) {
    items.push_back(str.substr(0, dlm_idx));
  }
  while(str.npos != (dlm_idx = str.find_first_of(delim))) {
    if(str.npos == str.find_first_not_of(delim)) {
      break;
    }
    items.push_back(str.substr(0, dlm_idx));
    dlm_idx++;
    str = str.erase(0, dlm_idx);
    if(str.npos == str.find_first_of(delim) && "" != str) {
      items.push_back(str);
      break;
    }
  }
  return items;
}

 // Align the start position to read the trace.log per line
std::string Tracer::trim(const std::string& string){
  const char* trimCharacterList = "[";
  std::string result;

  std::string::size_type left = string.find_first_of(trimCharacterList);

  if (left != std::string::npos){
    std::string::size_type right = string.find_last_of(trimCharacterList);  
		//result = string.substr(left, right - left + 1);
    result = string.substr(left);
  }

  return result;
}


int Tracer::ctoi(std::string s){
  if(s == "[000]")
    return 0;
  else if(s == "[001]")
    return 1;
  else if(s == "[002]")
    return 2;
  else if(s == "[003]")
    return 3;
  else if(s == "[004]")
    return 4;
  else if(s == "[005]")
    return 5;
  else if(s == "[006]")
    return 6;
  else if(s == "[007]")
    return 7;
  else if(s == "[008]")
    return 8;
  else
    return -1;
}

std::vector<trace_info_t> Tracer::get_info(){
  return v_trace_info;
}

std::vector<node_info_t> Tracer::get_node_list(){
	return v_node_info_;
}

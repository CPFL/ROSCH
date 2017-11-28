#include "ros_rosch/node_graph.hpp"
#include "ros_rosch/type.h"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <map>
#include <string>

using namespace rosch;

NodesInfo::NodesInfo() : v_node_info_(0), config_() {
  std::string filename(config_.get_configpath());
  loadConfig(std::string("/tmp/scheduler_rosch.yaml"));
  //    loadConfig(filename);
}

NodesInfo::~NodesInfo() {}

void NodesInfo::loadConfig(const std::string &filename) {
  std::cerr << filename << std::endl;
  try {
    YAML::Node node_list;
    node_list = YAML::LoadFile(filename);
    for (unsigned int i(0); i < node_list.size(); i++) {
      const YAML::Node name = node_list[i]["nodename"];
//      const YAML::Node index = node_list[i]["nodeindex"];
      const YAML::Node core = node_list[i]["core"];
      const YAML::Node subtopic = node_list[i]["sub_topic"];
      const YAML::Node pubtopic = node_list[i]["pub_topic"];
      const YAML::Node sched_info = node_list[i]["sched_info"];

      NodeInfo node_info;
      node_info.name = name.as<std::string>();
      //      node_info.index = index.as<int>();
      node_info.index = i;
      node_info.core = core.as<int>();
			
			node_info.period_count = 0;
			if(node_info.core >= 2)
				node_info.is_single_process = false;
			else
				node_info.is_single_process = true;

      node_info.v_subtopic.resize(0);
      for (int idx(0); idx < subtopic.size(); ++idx) {
        node_info.v_subtopic.push_back(subtopic[idx].as<std::string>());
      }

      node_info.v_pubtopic.resize(0);
      for (int idx(0); idx < pubtopic.size(); ++idx) {
        node_info.v_pubtopic.push_back(pubtopic[idx].as<std::string>());
      }

      node_info.v_sched_info.resize(0);
      for (int idx(0); idx < sched_info.size(); ++idx) {
        SchedInfo sched_info_element;
        sched_info_element.core = sched_info[idx]["core"].as<int>();
        sched_info_element.priority = sched_info[idx]["priority"].as<int>();
        sched_info_element.run_time = sched_info[idx]["run_time"].as<int>();
        // sched_info_element.start_time =
        // sched_info[idx]["start_time"].as<int>();
        // sched_info_element.end_time = sched_info_element.start_time +
        // sched_info_element.run_time;
   	    node_info.v_sched_info.push_back(sched_info_element);
      }

      v_node_info_.push_back(node_info);
    }
  } catch (YAML::Exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

NodeInfo NodesInfo::getNodeInfo(const int index) {
  try {
    for (int idx(0); idx < v_node_info_.size(); ++idx) {
      if (v_node_info_.at(idx).index == index)
        return v_node_info_.at(idx);
    }
    throw index;
  } catch (int e) {
    std::cerr << "Node index : " << e << ". cannot find node infomartion."
              << std::endl;
    NodeInfo empty_node_info;
    createEmptyNodeInfo(&empty_node_info);
    return empty_node_info;
  }
}

NodeInfo NodesInfo::getNodeInfo(const std::string name) {
  try {
    for (int idx(0); idx < v_node_info_.size(); ++idx) {
      if (v_node_info_.at(idx).name == name)
        return v_node_info_.at(idx);
    }
    throw name;
  } catch (std::string e) {
    std::cerr << "Node name : " << e << ". cannot find node infomartion."
              << std::endl;
    NodeInfo empty_node_info;
    createEmptyNodeInfo(&empty_node_info);
    return empty_node_info;
  }
}

void NodesInfo::createEmptyNodeInfo(NodeInfo *node_info) {
  node_info->name = "";
  node_info->index = -1;
  node_info->core = -1;
  node_info->v_sched_info.clear();
  node_info->v_subtopic.clear();
  node_info->v_pubtopic.clear();
}

size_t NodesInfo::getNodeListSize(void) { return v_node_info_.size(); }

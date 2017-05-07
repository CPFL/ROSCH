#include "node_graph.h"
#include "node.h"
#include "node_graph_core.h"
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

Graph g;
std::map<int, Graph::vertex_descriptor> desc;
const std::string name = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz"; 
//const char* label[] ={"N0","N1","N2","N3","N4","N5","N6"};

using namespace sched_analyzer;  

NodeGraph::NodeGraph() : config_() {
  std::string filename(config_.get_configpath());
  load_config_(filename);
}

NodeGraph::~NodeGraph() {}

void NodeGraph::load_config_(const std::string &filename) {
  std::cout << filename << std::endl;

  try {
    YAML::Node node_list;
    node_list = YAML::LoadFile(filename);
    for (unsigned int i(0); i < node_list.size(); i++) {
      const YAML::Node subnode_name = node_list[i]["nodename"];
      const YAML::Node subnode_index = node_list[i]["nodeindex"];
      const YAML::Node subnode_core = node_list[i]["core"];
      const YAML::Node subnode_subtopic = node_list[i]["sub_topic"];
      const YAML::Node subnode_pubtopic = node_list[i]["pub_topic"];
			const YAML::Node subnode_deadline = node_list[i]["deadline"];
			const YAML::Node subnode_period = node_list[i]["period"];
#ifdef SCHED_ANALYZER
      const YAML::Node subnode_runtime = node_list[i]["run_time"];
#endif
      node_info_t node_info;
      node_info.name = subnode_name.as<std::string>();
      node_info.index = i;
			node_info.deadline = subnode_deadline.as<int>();
			node_info.period = subnode_period.as<int>();
#ifdef SCHED_ANALYZER
      node_info.run_time = subnode_runtime.as<int>();
#endif
      node_info.core = subnode_core.as<int>();
      node_info.v_subtopic.resize(0);
      for (int j(0); (size_t)j < subnode_subtopic.size(); ++j) {
        node_info.v_subtopic.push_back(subnode_subtopic[j].as<std::string>());
      }
      node_info.v_pubtopic.resize(0);
      for (int j(0); (size_t)j < subnode_pubtopic.size(); ++j) {
        node_info.v_pubtopic.push_back(subnode_pubtopic[j].as<std::string>());
      }

      v_node_info_.push_back(node_info);
    }
  } catch (YAML::Exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

size_t NodeGraph::get_node_list_size() { return v_node_info_.size(); }

std::string NodeGraph::get_node_name(const int node_index) {
  for (int i(0); (size_t)i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).name;
  }
  exit(-1);
}

int NodeGraph::get_node_core(const int node_index) {
  for (int i(0); (size_t)i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).core;
  }
  exit(-1);
}

std::vector<std::string> NodeGraph::get_node_subtopic_p(const int node_index, std::string u_bar, int period) {
  for (int i(0); i < (int)v_node_info_.size()*(period+1); ++i) {
    if (v_node_info_.at(i).index == node_index) {
			for (int j(0); (size_t)j < v_node_info_.at(i).v_subtopic.size(); ++j) {
				v_node_info_.at(i).v_subtopic.at(j) += u_bar;
			}
      return v_node_info_.at(i).v_subtopic; 
    }
  }
  exit(-1);
}

int NodeGraph::get_node_run_time(const int node_index) {
  for (int i(0); (size_t)i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).run_time;
  }
  exit(-1);
}

int NodeGraph::get_node_deadline(const int node_index) {
  for (int i(0); (size_t)i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).deadline;
  }
  exit(-1);
}

int NodeGraph::get_node_period(const int node_index) {
  for (int i(0); (size_t)i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).period;
  }
  exit(-1);
}

std::vector<std::string> NodeGraph::get_node_pubtopic_p(const int node_index, std::string periodic_info, int period) {
  for (int i(0); i < (int)v_node_info_.size()*(period+1); ++i) {
    if (v_node_info_.at(i).index == node_index) {
			for (int j(0); (size_t)j < v_node_info_.at(i).v_pubtopic.size(); ++j) {
				v_node_info_.at(i).v_pubtopic.at(j) += periodic_info;
			}
      return v_node_info_.at(i).v_pubtopic;
    }
  }
  exit(-1);
}

SingletonNodeGraphAnalyzer::SingletonNodeGraphAnalyzer() {
 	int index=0;
	std::string u_bar = "_"; 

  /* Create Nodes */
	for(int i(0); i<periodic_count; i++){
	  for (int original(0); index < (int)get_node_list_size()*(i+1); ++index, ++original) {
			desc[index] = add_vertex(g);
			g[desc[index]].name_p = get_node_name(original); g[desc[index]].name_p += (u_bar + std::to_string(i));
			g[desc[index]].id_p = index;
			g[desc[index]].core_p = get_node_core(original);
			g[desc[index]].runtime_p = get_node_run_time(original);
			g[desc[index]].laxity_p = 10000; // initialize to set lower priority
			g[desc[index]].deadline_p = get_node_deadline(original) *(i+1); // if not an end node, this value is 0
			g[desc[index]].period_p = i == 0 ? 0 : get_node_period(original)*i;
			g[desc[index]].period_count = i;
			g[index].sub_topic_p = get_node_subtopic_p(original, u_bar, i);
			g[index].pub_topic_p = get_node_pubtopic_p(original, u_bar, i);
		}
  }

  /* Create DAG from Node info */
	for (int index1(0); (size_t)index1 < get_node_list_size()*periodic_count; index1++){
		for (int index2(0); (size_t)index2 < get_node_list_size()*periodic_count; index2++){
			for (int i(0); (size_t)i < g[desc[index1]].sub_topic_p.size(); ++i) {
				for (int j(0); (size_t)j < g[desc[index2]].pub_topic_p.size(); ++j) { 
				 	if(g[desc[index1]].sub_topic_p.at(i) == g[desc[index2]].pub_topic_p.at(j)){
						add_edge(desc[index2], desc[index1], g); 
					}
				}
			}
		}
	}

	/* print DAG */
	//boost::print_graph(g, name.c_str()); //print depedancies
	std::ofstream file("Graph.dot");
	boost::write_graphviz(file, g, boost::make_label_writer(name.c_str()));
	//boost::write_graphviz(file, g, boost::make_label_writer(label));
	system("dot -Tpng Graph.dot -o Graph.png");
	system("eog Graph.png"); 
}

SingletonNodeGraphAnalyzer::~SingletonNodeGraphAnalyzer() {
}

void SingletonNodeGraphAnalyzer::compute_laxity() {

	for (int index(0); (size_t)index < get_node_list_size()*periodic_count; ++index) { 
		if(g[desc[index]].deadline_p > 0){ // end node
				g[desc[index]].laxity_p = g[desc[index]].deadline_p - g[desc[index]].runtime_p;
				recursive_comp_laxity((unsigned int)index);
		}
	}
}

void SingletonNodeGraphAnalyzer::recursive_comp_laxity(unsigned int index) {
	IndexMap list = get(boost::vertex_index, g);
	boost::graph_traits<Graph>::edge_iterator ei, ei_end;
	int temp_laxity;

	for (tie(ei, ei_end) = edges(g); ei != ei_end; ei++) { // loop (dependencies) times
		if( index == list[target(*ei, g)]){
			temp_laxity = g[desc[index]].laxity_p - g[desc[list[source(*ei, g)]]].runtime_p; 

			if(g[desc[list[source(*ei, g)]]].laxity_p > temp_laxity){
				g[desc[list[source(*ei, g)]]].laxity_p = temp_laxity;
			}
			recursive_comp_laxity(list[source(*ei, g)]);
		}
	}
}

bool SingletonNodeGraphAnalyzer::sched_child_list(
    int index, std::vector<node_t> &node_list) {
  node_t *child_list[CHILD_SIZE];
  if (native_c::search_child_nodes(root_node, index, child_list, CHILD_SIZE)) {
    for (int i(0); i < CHILD_SIZE; ++i) {
      if (child_list[i] == NULL) {
        return true;
      }
      node_list.push_back(*(child_list[i]));
    }
  }
  return false;
}

bool SingletonNodeGraphAnalyzer::sched_node_queue(std::vector<Node_P> &Queue){

	for (int index(0); (size_t)index < get_node_list_size()*periodic_count; ++index) {
		Queue.push_back(g[desc[index]]);
	}

	std::sort(Queue.begin(),Queue.end());

#if 0
	std::cout << std::endl;
	for(std::vector<Node_P>::const_iterator i = Queue.begin(); i!=Queue.end();i++){
		std::cout << "node: " << i->name_p << ", laxity " << i->laxity_p << std::endl;
	}	
#endif
	return true;
}

SingletonNodeGraphAnalyzer &SingletonNodeGraphAnalyzer::getInstance() {
  static SingletonNodeGraphAnalyzer inst;
  return inst;
}

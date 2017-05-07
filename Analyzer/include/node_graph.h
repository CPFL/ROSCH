#ifndef NODE_GRAPH_H
#define NODE_GRAPH_H

#include "config.h"
#include "node.h"
#include "spec.h"
#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>

namespace sched_analyzer {
class NodeGraph {
public:
  NodeGraph();
  ~NodeGraph();
  std::string get_node_name(const int node_index);
  int get_node_core(const int node_index);
  int get_node_run_time(const int node_index);
	int get_node_deadline(const int node_index);
	int get_node_period(const int node_index);
	std::vector<std::string> get_node_subtopic_p(const int node_index, std::string u_bar,int period); 
	std::vector<std::string> get_node_pubtopic_p(const int node_index, std::string u_bar, int period);
  size_t get_node_list_size();

private:
  void load_config_(const std::string &filename);
  Config config_;
  std::vector<node_info_t> v_node_info_;
};

class SingletonNodeGraphAnalyzer : public NodeGraph {
public:
  static SingletonNodeGraphAnalyzer &getInstance();
  node_t *get_target_node();
	
  bool sched_leaf_list(std::vector<node_t> &node_list);
	bool sched_node_queue(std::vector<Node_P> &Queue);
  bool sched_child_list(int index, std::vector<node_t> &node_list);
  bool sched_parent_list(int index, std::vector<node_t> &node_list);
	node_t *search_node(int node_index);

  void compute_laxity();
	void recursive_comp_laxity(unsigned int index);

private:
  enum { PARENT_SIZE = 16, CHILD_SIZE = 16 }; // enum hack
  SingletonNodeGraphAnalyzer();
  ~SingletonNodeGraphAnalyzer();
  std::vector<node_t *> v_node_; // Vector of nodes
  node_t *root_node;
};
}
#endif // NODE_GRAPH_H

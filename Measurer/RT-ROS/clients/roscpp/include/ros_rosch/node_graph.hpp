#ifndef NODE_GRAPH_HPP
#define NODE_GRAPH_HPP
#include "yaml-cpp/yaml.h"
#include <map>
#include <string>

#define DISPLAY_MAX_BUF 32
#define MAX_LEAF_LIST 16

typedef struct node {
  std::string name;
  int index;
  int core;
  int pid;
  int depth;
  int is_exist;
  int target;
  float wcet;
  float laxity;
  float global_wcet;
  std::vector<std::string> v_sub_topic;
  std::vector<std::string> v_pub_topic;
  struct node *parent;
  struct node *child;
  struct node *next;
  /* struct node* parent_next; */
} node_t;

typedef struct node_info {
  std::string name;
  int index;
  int core;
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
} node_info_t;

namespace rosch {
class NodeGraph {
public:
  NodeGraph();
  NodeGraph(const std::string &filename);
  ~NodeGraph();
  int get_node_index(const std::string);
  std::string get_node_name(const int node_index);
  int get_node_core(const int node_index);
  int get_node_core(const std::string node_name);

  std::vector<std::string> get_node_subtopic(const int node_index);
  std::vector<std::string> get_node_pubtopic(const int node_index);
  size_t get_node_list_size();

private:
  std::string get_selfpath();
  void load_config(const std::string &filename);
  std::vector<node_info_t> v_node_info_;
};

class SingletonNodeGraphAnalyzer : public NodeGraph {
private:
  SingletonNodeGraphAnalyzer();
  SingletonNodeGraphAnalyzer(const std::string &filename);
  SingletonNodeGraphAnalyzer(const SingletonNodeGraphAnalyzer &);
  SingletonNodeGraphAnalyzer &operator=(const SingletonNodeGraphAnalyzer &);
  ~SingletonNodeGraphAnalyzer();
  void free_graph_node_();
  int test;
  std::vector<std::string> v_removed_sub_topic_;

public:
  std::vector<node_t *> v_node_;
  node_t *root_node;
  static SingletonNodeGraphAnalyzer &getInstance();
  int getIntTest();
  node_t *get_target_node();
  void finish_target_node();
  void finish_node(node_t *node);
  void finish_node(int node_index);
  void finish_topic(int node_index, std::string topic);
  node_t *search_node(int node_index);
  bool is_empty_topic_list(int node_index);
  bool is_in_node_graph(int node_index);
  void refresh_topic_list(int node_index);
};
}

#endif // NODE_GRAPH_HPP

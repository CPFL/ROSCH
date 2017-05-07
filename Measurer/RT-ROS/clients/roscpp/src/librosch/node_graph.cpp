#include "ros_rosch/node_graph.hpp"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <map>
#include <string>

using namespace rosch;
static const int PATH_MAX(128);

NodeGraph::NodeGraph() : v_node_info_(0) {
  std::string self_path(get_selfpath());
  load_config(std::string("/tmp/node_graph.yaml"));
}

NodeGraph::NodeGraph(const std::string &filename) : v_node_info_(0) {
  load_config(filename);
}

NodeGraph::~NodeGraph() {}

std::string NodeGraph::get_selfpath() {
  char buff[PATH_MAX];
  ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff) - 1);
  if (len != -1) {
    buff[len] = '\0';
    return std::string(buff);
  }
  /* handle error condition */
}

void NodeGraph::load_config(const std::string &filename) {
  try {
    YAML::Node node_list;
    node_list = YAML::LoadFile(filename);
    for (unsigned int i(0); i < node_list.size(); i++) {
      const YAML::Node subnode_name = node_list[i]["nodename"];
      const YAML::Node subnode_index = node_list[i]["nodeindex"];
      const YAML::Node subnode_core = node_list[i]["core"];
      const YAML::Node subnode_subtopic = node_list[i]["sub_topic"];
      const YAML::Node subnode_pubtopic = node_list[i]["pub_topic"];

      node_info_t node_info;
      node_info.name = subnode_name.as<std::string>();
      node_info.index = subnode_index.as<int>();
      node_info.index = i;
      node_info.core = subnode_core.as<int>();
      node_info.v_subtopic.resize(0);
      for (int i = 0; i < subnode_subtopic.size(); ++i) {
        node_info.v_subtopic.push_back(subnode_subtopic[i].as<std::string>());
      }
      node_info.v_pubtopic.resize(0);
      for (int i = 0; i < subnode_pubtopic.size(); ++i) {
        node_info.v_pubtopic.push_back(subnode_pubtopic[i].as<std::string>());
      }
      v_node_info_.push_back(node_info);
    }
  } catch (YAML::Exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

int NodeGraph::get_node_index(const std::string node_name) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).name == node_name)
      return v_node_info_.at(i).index;
  }
  return -1;
}

size_t NodeGraph::get_node_list_size() { return v_node_info_.size(); }

std::string NodeGraph::get_node_name(const int node_index) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).name;
  }
  return "";
}

int NodeGraph::get_node_core(const int node_index) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index)
      return v_node_info_.at(i).core;
  }
  return 1;
}

int NodeGraph::get_node_core(const std::string node_name) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).name == node_name)
      return v_node_info_.at(i).core;
  }
  return -1;
}

std::vector<std::string> NodeGraph::get_node_subtopic(const int node_index) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index) {
      return v_node_info_.at(i).v_subtopic;
    }
  }
  std::vector<std::string> v(0);
  return v;
}

std::vector<std::string> NodeGraph::get_node_pubtopic(const int node_index) {
  for (int i = 0; i < v_node_info_.size(); ++i) {
    if (v_node_info_.at(i).index == node_index) {
      return v_node_info_.at(i).v_pubtopic;
    }
  }
  std::vector<std::string> v(0);
  return v;
}

void node_init(node_t *node);
void insert_child_node(node_t *parent_node, node_t *child_node);
node_t *make_node(const std::string name, const int node_index, const int core,
                  const std::vector<std::string> v_subtopic,
                  const std::vector<std::string> v_pubtopic);
node_t *make_root_node();
node_t *_search_node(node_t *node, int node_index);
node_t **search_leaf_node(node_t *node);
void show_tree_dfs(node_t *node);
void free_tree(node_t *node);
int free_node(node_t *root_node, node_t *node);
void display(node_t *node);
void _search_leaf_node(node_t *node, node_t **leaf_list, int *i);
int _is_exist_leaf_node(node_t *node, node_t **leaf_list, int list_size);
void _free_tree(node_t *root_node, node_t *node);
node_t *search_parent_node(node_t *node, int node_index);
void remove_node(node_t *root_node, node_t *node);
node_t *get_target(node_t *node);
void finish_target(node_t *root_node, node_t *node);
bool is_leaf_node(node_t *root_node, node_t *node);

SingletonNodeGraphAnalyzer::SingletonNodeGraphAnalyzer() : NodeGraph(), test(0) {
  root_node = make_root_node();
  for (int index = 0; index < get_node_list_size(); ++index) {
    v_node_.push_back(make_node(get_node_name(index), index,
                                get_node_core(index), get_node_subtopic(index),
                                get_node_pubtopic(index)));
    insert_child_node(root_node, v_node_.at(index));
  }

  show_tree_dfs(root_node);
}

SingletonNodeGraphAnalyzer::SingletonNodeGraphAnalyzer(const std::string &filename) :  NodeGraph(filename), test(0) {
  root_node = make_root_node();
  for (int index = 0; index < get_node_list_size(); ++index) {
    v_node_.push_back(make_node(get_node_name(index), index,
                                get_node_core(index), get_node_subtopic(index),
                                get_node_pubtopic(index)));
    insert_child_node(root_node, v_node_.at(index));
  }

  show_tree_dfs(root_node);
}

SingletonNodeGraphAnalyzer::~SingletonNodeGraphAnalyzer() {
  free_graph_node_();
}

int SingletonNodeGraphAnalyzer::getIntTest() {
  test++;

  return test;
}

node_t *SingletonNodeGraphAnalyzer::search_node(int node_index) {
  return _search_node(root_node, node_index);
}

void SingletonNodeGraphAnalyzer::free_graph_node_() { free_tree(root_node); }

node_t *SingletonNodeGraphAnalyzer::get_target_node() { get_target(root_node); }
void SingletonNodeGraphAnalyzer::finish_target_node() {
  finish_target(root_node, get_target_node());
}

void SingletonNodeGraphAnalyzer::finish_node(node_t *node) {
  finish_target(root_node, node);
}

void SingletonNodeGraphAnalyzer::finish_node(int node_index) {
  finish_target(root_node, search_node(node_index));
}

bool SingletonNodeGraphAnalyzer::is_empty_topic_list(int node_index) {
  node_t *searched = search_node(node_index);
  if (searched == NULL)
    return false;
  else
    return searched->v_sub_topic.empty();
}
bool SingletonNodeGraphAnalyzer::is_in_node_graph(int node_index) {
  node_t *searched = search_node(node_index);
  if (searched == NULL)
    return false;
  return true;
}

SingletonNodeGraphAnalyzer &SingletonNodeGraphAnalyzer::getInstance() {
  static SingletonNodeGraphAnalyzer inst;
  return inst;
}

void SingletonNodeGraphAnalyzer::finish_topic(int node_index,
                                              std::string topic) {
  std::vector<std::string>::iterator itr =
      search_node(node_index)->v_sub_topic.begin();
  while (itr != search_node(node_index)->v_sub_topic.end()) {
    if (*itr == topic) {
      itr = search_node(node_index)->v_sub_topic.erase(itr);
    } else {
      itr++;
    }
  }
}

void SingletonNodeGraphAnalyzer::refresh_topic_list(int node_index) {
  std::vector<std::string> v_subtopic(get_node_subtopic(node_index));
  search_node(node_index)->v_sub_topic.resize(v_subtopic.size());
  std::copy(v_subtopic.begin(), v_subtopic.end(),
            search_node(node_index)->v_sub_topic.begin());
}

/*
 * Integrated from related to ROS function in RESCH
*/
void node_init(node_t *node) {
  node->index = -1;
  node->pid = -1;
  node->depth = 0;
  node->is_exist = 0;
  node->is_exist = 0;
  node->wcet = -1;
  node->laxity = -1;
  node->global_wcet = -1;
  node->parent = NULL;
  node->child = NULL;
  node->next = NULL;
}

void insert_child_node(node_t *parent_node, node_t *child_node) {
  if (parent_node == NULL || child_node == NULL)
    return;

  /* Update deepest depth */
  if (child_node->depth < parent_node->depth + 1 || child_node->depth == 0) {
    child_node->depth = parent_node->depth + 1;
  }

  if (parent_node->child == NULL) {
    parent_node->child = child_node;
  } else {
    node_t *next_child = parent_node->child;
    while (next_child->next != NULL) {
      next_child = next_child->next;
    }
    next_child->next = child_node;
  }
}

/**
 * API : Make node.
 * arg 1 : node index
 */
node_t *make_node(const std::string name, const int node_index, const int core,
                  const std::vector<std::string> v_subtopic,
                  const std::vector<std::string> v_pubtopic) {
  node_t *node = new node_t;
  if (node != NULL) {
    node_init(node);
    node->name = name;
    node->index = node_index;
    node->core = core;
    node->v_sub_topic.resize(v_subtopic.size());
    std::copy(v_subtopic.begin(), v_subtopic.end(), node->v_sub_topic.begin());
    node->v_pub_topic.resize(v_pubtopic.size());
    std::copy(v_pubtopic.begin(), v_pubtopic.end(), node->v_pub_topic.begin());

    std::cout << "create:" << node_index << std::endl;
  }
  return node;
}
/**
 * API : Make node.
 * arg 1 : node index
 */
node_t *make_root_node() {
  node_t *node = new node_t;
  node_init(node);
  node->name = "root";
  node->index = -10;
  node->core = 0;
  node->v_sub_topic.resize(0);
  node->v_pub_topic.resize(0);
  return node;
}
/**
 * API : Search node from index.
 * arg 1 : root node
 * arg 2 : node index
 */
node_t *_search_node(node_t *node, int node_index) {
  if (node == NULL)
    return NULL;
  if (node->index == node_index)
    return node;

  if (node->child) {
    node_t *result = _search_node(node->child, node_index);
    if (result != NULL)
      return result;
  }

  if (node->next) {
    node_t *result = _search_node(node->next, node_index);
    if (result != NULL)
      return result;
  }

  return NULL;
}

void _search_leaf_node(node_t *node, node_t **leaf_list, int *i) {
  /* if (node == NULL) */
  /*     return; */

  if (node->child) {
    _search_leaf_node(node->child, leaf_list, i);
  } else {
    if (_is_exist_leaf_node(node, leaf_list, *i)) {
      return;
    } else {
      leaf_list[(*i)] = node;
      ++(*i);
    }
  }

  if (node->next)
    _search_leaf_node(node->next, leaf_list, i);
}

int _is_exist_leaf_node(node_t *node, node_t **leaf_list, int list_size) {
  int i;
  for (i = 0; i < list_size; ++i) {
    if (leaf_list[i] == node)
      return 1; // exist(true)
  }
  return 0;
}

/**
 * API: Return leaf nodes as node_t**.
 * End of leaf is NULL.
 * arg 1 : root node
 */
node_t **search_leaf_node(node_t *node) {
  if (node == NULL)
    return NULL;

  int i = 0;
  static node_t *leaf_list[MAX_LEAF_LIST];

  _search_leaf_node(node, leaf_list, &i);

  leaf_list[i] = NULL;

  int j;
  for (j = 0; j < i; ++j) {
    std::cout << "leaf[" << leaf_list[j]->index << "] is found.\n" << std::endl;
  }

  return leaf_list;
}

/**
 * API: Return leaf nodes as node_t**.
 * End of leaf is NULL.
 * arg 1 : root node
 */
bool is_leaf_node(node_t *root_node, node_t *node) {
  if (node == NULL)
    return NULL;

  int i = 0;
  static node_t *leaf_list[MAX_LEAF_LIST];

  _search_leaf_node(root_node, leaf_list, &i);

  leaf_list[i] = NULL;

  int j;
  for (j = 0; j < i; ++j) {
    if (node->index == leaf_list[j]->index)
      return true;
  }

  return false;
}

/**
 * API: Print node.
 * arg 1 : root node
 */
void display(node_t *node) {
  int i;
  char buf[DISPLAY_MAX_BUF];
  for (i = 0; i < node->depth && i < sizeof(buf); ++i) {
    buf[i] = ' ';
  }
  buf[i] = '\0';

  std::cout << buf << "L" << node->index << std::endl;
}

/**
 * API: Print node graph.
 * arg 1 : root node
 */
void show_tree_dfs(node_t *node) {
  if (node == NULL)
    return;
  display(node);
  if (node->child)
    show_tree_dfs(node->child);
  if (node->next)
    show_tree_dfs(node->next);
}

/**
 * API: Free node graph tree.
 * arg 1 : root node
 */
void free_tree(node_t *root_node) { _free_tree(root_node, root_node); }

void _free_tree(node_t *root_node, node_t *node) {
  if (node == NULL)
    return;
  if (node->child)
    _free_tree(root_node, node->child);
  if (node->next)
    _free_tree(root_node, node->next);
  display(node);

  free_node(root_node, node);
}

/**
 * API: Free node.
 * arg 1 : root node
 * arg 2 : node
 */
int free_node(node_t *root_node, node_t *node) {
  if (is_leaf_node(root_node, node)) {
    remove_node(root_node, node);
    delete node;
  }
}

/**
 * API: Remove node in node graph.
 * arg 1 : root node
 * arg 2 : node
 */
void remove_node(node_t *root_node, node_t *node) {
  node_t *parent_node = search_parent_node(root_node, node->index);

  while (parent_node != NULL) {
    if (parent_node->child) {
      if (parent_node->child->index == node->index)
        parent_node->child = NULL;
    }
    if (parent_node->next) {
      if (parent_node->next->index == node->index)
        parent_node->next = NULL;
    }

    parent_node = search_parent_node(root_node, node->index);
  }
}

node_t *search_parent_node(node_t *node, int node_index) {
  if (node == NULL)
    return NULL;

  if (node->child) {
    if (node->child->index == node_index) {
      return node;
    }

    node_t *result = search_parent_node(node->child, node_index);
    if (result != NULL)
      return result;
  }

  if (node->next) {
    if (node->next->index == node_index) {
      return node;
    }

    node_t *result = search_parent_node(node->next, node_index);
    if (result != NULL)
      return result;
  }

  return NULL;
}

/**
 * API: Get target node in node graph.
 * arg 1 : root node
 */
node_t *get_target(node_t *node) {
  if (node == NULL)
    return NULL;
  if (node->child) {
    node_t *result = get_target(node->child);
    if (result != NULL)
      return result;
  }
  if (node->next) {
    node_t *result = get_target(node->next);
    if (result != NULL)
      return result;
  }

  return node;
}

/**
 * API: Remove target node in node graph.
 * arg 1 : root node
 * arg 2 : node
 */
void finish_target(node_t *root_node, node_t *node) {
  free_node(root_node, node);
}

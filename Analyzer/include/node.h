#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

#define LAXITY
#ifdef LAXITY
#include <iostream>
#include <stdlib.h> 
#include <algorithm>
#include <utility>  
#include <fstream>  
#include <boost/graph/adjacency_list.hpp>  
#include <boost/graph/graphviz.hpp> 
#include <boost/graph/graph_utility.hpp> 
#include <boost/graph/graph_traits.hpp>
#include <functional>
#endif

static const int PARENT_MAX = 6;
extern int periodic_count;

struct Node_P{
	std::string name_p;
	int id_p;
	int core_p;
	int runtime_p;
	int laxity_p;
	int deadline_p;
	int period_p;
	int period_count;
	std::vector<std::string> sub_topic_p;
	std::vector<std::string> pub_topic_p;

	bool operator<(const Node_P& another) const{
		return laxity_p < another.laxity_p;
	}
}; 

#ifdef LAXITY
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Node_P> Graph;
typedef std::pair<int, int> Edge;
typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
extern Graph g;
extern std::map<int, Graph::vertex_descriptor> desc;
#endif

typedef struct node_t {
  std::string name;
  int index;
  int depth;
  int core;
#if 1 // C++
  std::vector<std::string> v_sub_topic;
  std::vector<std::string> v_pub_topic;
#else // Native C
#endif
#ifdef SCHED_ANALYZER
  int run_time;
	int deadline;
	int period;
  int inv_laxity;
#endif
  node_t *parent[PARENT_MAX];
  node_t *child;
  node_t *next;
} node_t;

typedef struct node_info_t {
  std::string name;
  int index;
  int core;
#ifdef SCHED_ANALYZER
  int run_time;
	int deadline;
	int period;
#endif
  std::vector<std::string> v_subtopic;
  std::vector<std::string> v_pubtopic;
} node_info_t;
#endif // NODE_H

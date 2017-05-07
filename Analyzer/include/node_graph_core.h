#ifndef NODE_GRAPH_CORE_H
#define NODE_GRAPH_CORE_H
#include "node.h"
#include <string>
namespace native_c {
extern void node_init(node_t *node);
extern void insert_child_node(node_t *parent_node, node_t *child_node);
extern node_t *make_node(const std::string name, const int node_index,
                         const int core,
                         const std::vector<std::string> v_subtopic,
                         const std::vector<std::string> v_pubtopic,
                         const int run_time);
extern node_t *make_root_node();
extern node_t *search_node(node_t *node, int node_index);
extern node_t **search_leaf_node(node_t *root_node);
extern bool search_parent_nodes(node_t *root_node, int node_index,
                                node_t **node_list, int list_size);
extern bool search_child_nodes(node_t *root_node, int node_index,
                               node_t **node_list, int list_size);
extern void show_tree_dfs(node_t *root_node);
extern void compute_laxity(node_t *root_node);
extern void free_tree(node_t *root_node);
extern int free_node(node_t *root_node, node_t *node);
extern void display(node_t *root_node);
extern node_t *get_target(node_t *root_node);
extern void finish_target(node_t *root_node, node_t *node);
extern bool is_leaf_node(node_t *root_node, node_t *node);
extern void show_leaf_list(node_t *node);
extern int get_max_leaf_list();
extern int get_max_parent_list();
}
#endif

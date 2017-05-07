#include "node.h"
#include "node_graph_core.h"
#include <iostream>
#include <stdio.h>

namespace native_c {
static const int MAX_LEAF_LIST = 16;
static const int DISPLAY_MAX_BUF = 64;

void node_init(node_t *node);
void insert_child_node(node_t *parent_node, node_t *child_node);
#if 1 // C++
node_t *make_node(const std::string name, const int node_index, const int core,
                  const std::vector<std::string> v_subtopic,
                  const std::vector<std::string> v_pubtopic,
                  const int run_time);
#else // Native C
node_t *make_node(const std::string name, const int node_index, const int core);
#endif
node_t *make_root_node();
node_t *search_node(node_t *node, int node_index);
node_t **search_leaf_node(node_t *node);
void show_tree_dfs(node_t *node);
void show_leaf_list(node_t *node);
void free_tree(node_t *node);
int free_node(node_t *root_node, node_t *node);
#if 0 // old code
void remove_node(node_t *root_node, node_t *node);
node_t *search_parent_nodes(node_t *node, int node_index);
#else // new code
void remove_node(node_t *node);
bool search_parent_nodes(node_t *root_node, int node_index, node_t **node_list,
                         int list_size);
#endif
bool search_child_nodes(node_t *root_node, int node_index, node_t **node_list,
                        int list_size);
node_t *get_target(node_t *node);
void finish_target(node_t *root_node, node_t *node);
bool is_leaf_node(node_t *root_node, node_t *node);
int _is_exist_leaf_node(node_t *node, node_t **leaf_list, int list_size);
void _free_tree(node_t *root_node, node_t *node, int *i);
void display(node_t *node, int depth);
void _show_tree_dfs(node_t *node, int *i);
void compute_laxity(node_t *root_node);
void _compute_laxity(node_t *node, int *sum_run_time);
int get_max_leaf_list();
int get_max_parent_list();
int get_max_child_list();

/*
 * Integrated from related to ROS function in RESCH
*/
void node_init(node_t *node) {
  node->index = -1;
  node->depth = 0;
  node->run_time = -1;
  node->inv_laxity = -1;
  int i;
  for (i = 0; i < PARENT_MAX; ++i)
    node->parent[i] = NULL;
  node->child = NULL;
  node->next = NULL;
}

int get_max_leaf_list() { return MAX_LEAF_LIST; }

int get_max_parent_list() { return PARENT_MAX; }

void insert_child_node(node_t *parent_node, node_t *child_node) {
  if (parent_node == NULL || child_node == NULL)
    return;

  /* Set parent */
  int i;
  for (i = 0; i < PARENT_MAX; ++i) {
    if (child_node->parent[i] == NULL) {
      child_node->parent[i] = parent_node;
      break;
    }
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
 * arg 1 : node name, index, core, subtopic, pubtopic
 */
#if 1 // C++
node_t *make_node(const std::string name, const int node_index, const int core,
                  const std::vector<std::string> v_subtopic,
                  const std::vector<std::string> v_pubtopic,
                  const int run_time) {
#else // Native C
node_t *make_node(const std::string name, const int node_index,
                  const int core) {
#endif
  node_t *node = new node_t;
  if (node != NULL) {
    node_init(node);
    node->name = name;
    node->index = node_index;
    node->core = core;
    node->run_time = run_time;
#if 1 // C++
    node->v_sub_topic.resize(v_subtopic.size());
    std::copy(v_subtopic.begin(), v_subtopic.end(), node->v_sub_topic.begin());
    node->v_pub_topic.resize(v_pubtopic.size());
    std::copy(v_pubtopic.begin(), v_pubtopic.end(), node->v_pub_topic.begin());
#else // Native C
#endif
//    printf("create:(index:%3d)%s\n", node_index, name.c_str());
  }
  return node;
}
/**
 * API : Make root node.
 */
node_t *make_root_node() {
  node_t *node = new node_t;
  node_init(node);
  node->name = "root";
  node->index = -10;
  node->core = 0;
#if 1 // C++
  node->v_sub_topic.resize(0);
  node->v_pub_topic.resize(0);
#else // Native C
#endif
  return node;
}
/**
 * API : Search node from index.
 * arg 1 : root node
 * arg 2 : node index
 */
node_t *search_node(node_t *node, int node_index) {
  if (node == NULL)
    return NULL;
  if (node->index == node_index)
    return node;

  if (node->child) {
    node_t *result = search_node(node->child, node_index);
    if (result != NULL)
      return result;
  }

  if (node->next) {
    node_t *result = search_node(node->next, node_index);
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

  // int j;
  // for (j = 0; j < i; ++j) {
  //   std::cout << "leaf[" << leaf_list[j]->index << "] is found." <<
  //   std::endl;
  // }

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
 * API: Compute all laxity by run_time.
 * arg 1 : root node
 */
void compute_laxity(node_t *root_node) {
  int sum_run_time = 0;
  _compute_laxity(root_node, &sum_run_time);
}

void _compute_laxity(node_t *node, int *sum_run_time) {
  if (node->run_time + *sum_run_time > node->inv_laxity) {
    if (node->run_time >= 0)
      node->inv_laxity = *sum_run_time + node->run_time;
  }
  if (node->child) {
    if (node->run_time >= 0)
      *sum_run_time += node->run_time;
    _compute_laxity(node->child, sum_run_time);
    if (node->run_time >= 0)
      *sum_run_time -= node->run_time;
  }
  if (node->next)
    _compute_laxity(node->next, sum_run_time);
}

/**
 * API: Print node.
 * arg 1 : root node
 */
void display(node_t *node, int depth) {
  int i;
  char buf[DISPLAY_MAX_BUF];
  for (i = 0; i < depth && i < (DISPLAY_MAX_BUF - 1); ++i) {
    buf[i] = ' ';
  }
  buf[i] = '\0';

  std::cout << buf << "L" << node->name << "@" << node->index
            << "(run_time:" << node->inv_laxity << ")" << std::endl;
}

/**
 * API: Print leaf nodes.
 * arg 1 : root node
 */
void show_leaf_list(node_t *node) {
  node_t **leaf_list = search_leaf_node(node);
  int i;
  printf("---- show leaf list ----\n");
  for (i = 0; i < MAX_LEAF_LIST; ++i) {
    if (leaf_list[i] == NULL)
      break;
    printf("leaf:%s@%d\n", leaf_list[i]->name.c_str(), leaf_list[i]->index);
  }
}

/**
 * API: Print node graph.
 * arg 1 : root node
 */
void show_tree_dfs(node_t *node) {
  if (node == NULL)
    return;
  int i = 0;
  printf("---- show tree ----\n");
  _show_tree_dfs(node, &i);
}

void _show_tree_dfs(node_t *node, int *i) {
  display(node, *i);
  if (node->child) {
    (*i)++;
    _show_tree_dfs(node->child, i);
    (*i)--;
  }
  if (node->next)
    _show_tree_dfs(node->next, i);
}
/**
 * API: Free node graph tree.
 * arg 1 : root node
 */
void free_tree(node_t *root_node) {
  int i = 0;
//  printf("---- free tree ----\n");
  _free_tree(root_node, root_node, &i);
}

void _free_tree(node_t *root_node, node_t *node, int *i) {
  if (node == NULL)
    return;
  if (node->child) {
    (*i)++;
    _free_tree(root_node, node->child, i);
    (*i)--;
  }
  if (node->next) {
    _free_tree(root_node, node->next, i);
  }
//  display(node, *i);
  free_node(root_node, node);
}

/**
 * API: Free node.
 * arg 1 : root node
 * arg 2 : node
 */
int free_node(node_t *root_node, node_t *node) {
  if (is_leaf_node(root_node, node)) {
    remove_node(node);
    delete node;
    return 1;
  }
  return -1;
}

/**
 * API: Remove node in node graph.
 * arg 1 : root node
 * arg 2 : node
 */
#if 1 // new code
void remove_node(node_t *node) {
  int i;
  for (i = 0; i < PARENT_MAX; ++i) {
    if (node->parent[i] == NULL) {
      return;
    }
    {
      node_t *child;
      child = node->parent[i]->child;
      if (child == node) {
        node->parent[i]->child = node->next;
        continue;
      }
    }
    {
      node_t *child;
      child = node->parent[i]->child;
      while (child->next) {
        if (child->next == node) {
          child->next = node->next;
          continue;
        }
        child = child->next;
      }
    }
  }
}

bool search_child_nodes(node_t *root_node, int node_index, node_t **node_list,
                        int list_size) {
  if (NULL == root_node) {
    //fprintf(stderr, "root node is NULL\n");
    return false;
  }
  if (list_size < 1) {
    fprintf(stderr, "list size is %d. It is too small.\n", list_size);
    return false;
  }

  node_t *node = search_node(root_node, node_index);
  if (NULL == node) {
    fprintf(stderr, "cannot find node from nodeindex[%d].\n", node_index);
    return false;
  }

  node_list[0] = node->child;
  /* have no child */
  if (NULL == node->child)
    return true;

  int i;
  node_t *child = node->child->next;
  for (i = 1; i < list_size; ++i) {
    node_list[i] = child;
    if (NULL == node_list[i])
      return true;
    child = child->next;
  }

  fprintf(stderr, "list size is %d. It is too small.\n", list_size);
  return false;
}

bool search_parent_nodes(node_t *root_node, int node_index, node_t **node_list,
                         int list_size) {
  if (root_node == NULL)
    return NULL;
  if (NULL == root_node) {
    //fprintf(stderr, "root node is NULL\n");
    return false;
  }
  if (list_size < 1) {
    fprintf(stderr, "list size is %d. It is too small.\n", list_size);
    return false;
  }
  node_t *node = search_node(root_node, node_index);
  if (NULL == node) {
    fprintf(stderr, "cannot find node from nodeindex[%d].\n", node_index);
    return false;
  }

  node_t **parent = node->parent;
  int i;
  for (i = 0; i < list_size; ++i) {
    if (i == PARENT_MAX) {
      fprintf(stderr, "Please change PARENT_MAX[%d].\n", PARENT_MAX);
      return false;
    }
    node_list[i] = parent[i];
    if (NULL == node_list[i]) {
      return true;
    }
  }

  fprintf(stderr, "list size is %d. It is too small.\n", list_size);
  return false;
}

#else // old code
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

node_t *search_parent_nodes(node_t *node, int node_index) {
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
#endif

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
}

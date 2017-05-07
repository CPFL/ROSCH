#include <asm/current.h>
#include <linux/slab.h>
#include "sched_ros.h"

#define DISPLAY_MAX_BUF 32
#define MAX_LEAF_LIST 16
#define MAX_FREED_NODE_LIST 32

void node_init(node_t* node)
{
    node->name = NULL;
    node->index = -1;
    node->pid = -1;
    node->depth = 0;
    node->is_exist = 0;
    node->wcet = -1;
    node->laxity = -1;
    node->global_wcet = -1;
    node->parent = NULL;
    node->child = NULL;
    node->next = NULL;
    /* node->parent_next = NULL; */
}

void insert_child_node(node_t* parent_node, node_t* child_node)
{
    if (parent_node == NULL || child_node == NULL)
        return;

    /* Update deepest depth */
    if(child_node->depth < parent_node->depth + 1 || child_node->depth == 0) {
        child_node->depth = parent_node->depth + 1;
    }

    if(parent_node->child == NULL) {
        parent_node->child = child_node;
    }
    else {
        node_t* next_child = parent_node->child;
        while (next_child->next != NULL) {
            next_child = next_child->next;
        }
        next_child->next = child_node;
    }
}

node_t* make_node(int node_index)
{
    node_t *node = (node_t*)kmalloc(sizeof(node_t), GFP_KERNEL);
    if (node != NULL) {
        node_init(node);
        node->index = node_index;
        printk(KERN_INFO
               "create:%d\n", node_index);
    }
    return node;
}

node_t* search_node(node_t* node, int node_index)
{
  if (node == NULL)
      return NULL;
  if(node->index == node_index)
      return node;

  if (node->child) {
      node_t* result = search_node(node->child, node_index);
      if (result != NULL)
          return result;
  }

  if (node->next) {
      node_t* result = search_node(node->next, node_index);
      if (result != NULL)
          return result;
  }

  return NULL;
}



void _search_leaf_node(node_t* node, node_t** leaf_list, int* i)
{
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

int _is_exist_leaf_node(node_t* node, node_t** leaf_list, int list_size)
{
    int i;
    for (i = 0; i < list_size; ++i) {
        if (leaf_list[i] == node)
            return 1; // exist(true)
    }
    return 0;
}

/**
 * API: return leaf nodes as node_t**.
 * End of leaf is NULL.
 */
node_t** search_leaf_node(node_t* node)
{
    if (node == NULL)
        return NULL;

    int i = 0;
    static node_t* leaf_list[MAX_LEAF_LIST];

    _search_leaf_node(node, leaf_list, &i);

    leaf_list[i] = NULL;

    int j;
    for (j = 0; j < i; ++j) {
        printk(KERN_INFO
               "leaf list[%d] is found.\n", leaf_list[j]->index);
    }

    return leaf_list;
}

void display(node_t* node)
{
    int i;
    char buf[DISPLAY_MAX_BUF];
    for (i = 0; i < node->depth && i < sizeof(buf); ++i) {
        buf[i] = ' ';
    }
    buf[i] = '\0';

    printk(KERN_INFO
           "%sL %d\n", buf, node->index);
}

void show_tree_dfs(node_t* node)
{
  if (node == NULL)
      return;
  display(node);
  if (node->child)
      show_tree_dfs(node->child);
  if (node->next)
      show_tree_dfs(node->next);
}

void free_tree(node_t* node)
{
    int i = 0;
    int freed_node_index_list[MAX_FREED_NODE_LIST];
    _free_tree(node, freed_node_index_list, &i);
}


void _free_tree(node_t* node, int* freed_node_index_list, int* i)
{
    if (node == NULL)
        return;
    if (node->child)
        _free_tree(node->child, freed_node_index_list, i);
    if (node->next)
        _free_tree(node->next, freed_node_index_list, i);
    display(node);

    if(_is_exist_freed_node(node, freed_node_index_list, *i))
        return;

    freed_node_index_list[(*i)] = node->index;
    ++(*i);
    free_node(node);
}

int _is_exist_freed_node(node_t* node, int* freed_node_index_list, int list_size)
{
    int i;
    for (i = 0; i < list_size; ++i) {
        if(freed_node_index_list[i] == node->index) {
            printk(KERN_INFO
                   "node[%d] is aleady freed.\n", node->index);
            return 1; // exist (true)
        }
    }

    return 0;
}

int free_node(node_t* node)
{
    kfree(node);
}

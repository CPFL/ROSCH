#ifndef __RESCH_SCHED_ROS__
#define __RESCH_SCHED_ROS__

typedef struct node {
    char* name;
    int index;
    int pid;
    int depth;
    int is_exist;
    float wcet;
    float laxity;
    float global_wcet;
    struct node* parent;
    struct node* child;
    struct node* next;
    /* struct node* parent_next; */
} node_t;

extern void node_init(node_t* node);
extern void insert_child_node(node_t* parent_node, node_t* child_node);
extern node_t* make_node(int node_index);
extern node_t* search_node(node_t* node, int node_index);
extern node_t** search_leaf_node(node_t* node);
extern void show_tree_dfs(node_t* node);
extern void free_tree(node_t* node);
extern int free_node(node_t* node);

#endif

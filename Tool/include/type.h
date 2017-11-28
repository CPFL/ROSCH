#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>

typedef struct SchedInfo
{
  int core;
  int priority;
  int runtime;
} SchedInfo;

typedef struct DependInfo
{
  std::vector<std::string> pub_topic;
  std::vector<std::string> sub_topic;
} DependInfo;

typedef struct node_info_t
{
  std::string name;
  int index;
  int core;
  int runtime;
  int deadline;
  DependInfo depend;
  std::vector<SchedInfo> sched_info;

  bool operator<(const node_info_t &right) const
  {
    return index < right.index;  //小さいのから大きい順番に並べたいとき
  }

} node_info_t;

#endif  // TYPE_H

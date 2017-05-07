#ifndef SCHED_NODE_H
#define SCHED_NODE_H

#include "ros_rosch/type.h"
#include "ros_rosch/task_attribute_processer.h"
#include <vector>

namespace rosch {
class SchedNode {
public:
  SchedNode(NodeInfo node_info);
  ~SchedNode();

private:
  NodeInfo node_info_;
  TaskAttributeProcesser task_attr_processer_;
};
}

#endif

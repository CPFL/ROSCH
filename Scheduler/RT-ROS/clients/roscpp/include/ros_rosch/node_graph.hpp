#ifndef NODE_GRAPH_HPP
#define NODE_GRAPH_HPP
#include "ros_rosch/config.h"
#include "ros_rosch/spec.h"
#include "ros_rosch/type.h"
#include "yaml-cpp/yaml.h"
#include <map>
#include <string>
#include <vector>

namespace rosch {
class NodesInfo {
public:
    NodesInfo();
    NodesInfo(const std::string &filename);
    ~NodesInfo();
    NodeInfo getNodeInfo(const int index);
    NodeInfo getNodeInfo(const std::string name);
    size_t getNodeListSize(void);
private:
  Config config_;
  void loadConfig(const std::string &filename);
  void createEmptyNodeInfo(NodeInfo *node_info);
  std::vector<NodeInfo> v_node_info_;
};
}

#endif // NODE_GRAPH_HPP

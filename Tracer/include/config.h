#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace SchedViz {
class Config {
public:
	Config(const std::string &config_file = "node_graph.yaml");
  ~Config();
  std::string get_configpath();

private:
  std::string get_dirname(const std::string &path);
  std::string get_basename(const std::string &path);
  std::string get_selfpath();
  std::string config_file_;
  //static const int PATH_MAX = 128;
};
}

#endif

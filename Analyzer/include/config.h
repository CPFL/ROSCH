#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace sched_analyzer
{
class Config
{
public:
  Config(const std::string &config_file = "analyzer_rosch.yaml", const std::string &spec_file = "hardware_spec.yaml");
  ~Config();
  std::string get_configpath();
  std::string get_specpath();

private:
  std::string get_dirname(const std::string &path);
  std::string get_basename(const std::string &path);
  std::string get_selfpath();
  std::string config_file_;
  std::string spec_file_;
  // static const int PATH_MAX = 128;
};
}

#endif

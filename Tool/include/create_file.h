#ifndef CREATE_FILE_H
#define CREATE_FILE_H

#include <stdlib.h>
#include "type.h"

namespace ROSCH
{
class Parser
{
public:
  Parser();
  ~Parser();
  void create_file(std::vector<node_info_t> infos, int mode);
  void preview_topics_depend(std::vector<node_info_t> infos);
  DependInfo get_depend_info(std::string topic);
  std::vector<std::string> get_node_list();

private:
  /* for Measurer */
  void create_yaml_file(std::string name,
                        int core,  //
                        std::vector<std::string> pub_topic, std::vector<std::string> sub_topic);

  /* for Analyzer */
  void create_yaml_file(std::string name, int core, std::vector<std::string> pub_topic,
                        std::vector<std::string> sub_topic, int deadline, int period, int run_time);

  /* for Scheduler */
  void create_yaml_file(std::string name, int core, std::vector<std::string> pub_topic,
                        std::vector<std::string> sub_topic, std::vector<SchedInfo> sched_infos);

  /* for Tracer */
  void create_yaml_file(std::string name, std::vector<std::string> pub_topic, std::vector<std::string> sub_topic,
                        int deadline);

  std::string get_topic(char *line);
  std::vector<std::string> split(std::string str, std::string delim);
  bool checkFileExistence(const std::string &str);
  void file_clear(int mode);
};
}
#endif  // CREATE_FILE_H

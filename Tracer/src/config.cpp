#include "config.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

using namespace SchedViz;

Config::Config(const std::string &config_file) : config_file_(config_file)
{
}
Config::~Config()
{
}

std::string Config::get_configpath()
{
  std::string self_path(get_selfpath());
  return get_dirname(self_path) + "/../../YAMLs/" + config_file_;
}

std::string Config::get_dirname(const std::string &path)
{
  return path.substr(0, path.find_last_of('/'));
}
std::string Config::get_basename(const std::string &path)
{
  return path.substr(path.find_last_of('/') + 1);
}
std::string Config::get_selfpath()
{
  char buff[128];
  ssize_t len = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
  if (len != -1)
  {
    buff[len] = '\0';
    return std::string(buff);
  }
  /* handle error condition */
  exit(1);
  return std::string("a");
}

#include "create_file.h"
#include <stdlib.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include "type.h"
#include "yaml-cpp/yaml.h"

using namespace ROSCH;

Parser::Parser()
{
}

Parser::~Parser()
{
}

/* for Measurer */
void Parser::create_yaml_file(std::string name, int core, std::vector<std::string> pub_topic,
                              std::vector<std::string> sub_topic)
{
  std::string file_name = "../../YAMLs/measurer_rosch.yaml";
  YAML::Emitter out;
  out << YAML::BeginSeq;
  out << YAML::BeginMap;
  out << YAML::Key << "nodename";
  out << YAML::Value << name;
  out << YAML::Key << "core";
  out << YAML::Value << core;

  out << YAML::Key << "sub_topic";
  if (sub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)sub_topic.size(); i++)
    {
      out << sub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "pub_topic";
  if (pub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)pub_topic.size(); i++)
    {
      out << pub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::EndMap;
  out << YAML::EndSeq;

  std::ofstream outputfile(file_name.c_str(), std::ios::app);
  outputfile << out.c_str() << "\n\n";
  outputfile.close();
}

/* for Analyzer */
void Parser::create_yaml_file(std::string name, int core, std::vector<std::string> pub_topic,
                              std::vector<std::string> sub_topic, int deadline, int period, int run_time)
{
  std::string file_name = "../../YAMLs/analyzer_rosch.yaml";
  YAML::Emitter out;
  out << YAML::BeginSeq;
  out << YAML::BeginMap;
  out << YAML::Key << "nodename";
  out << YAML::Value << name;
  out << YAML::Key << "core";
  out << YAML::Value << core;

  out << YAML::Key << "sub_topic";
  if (sub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)sub_topic.size(); i++)
    {
      out << sub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "pub_topic";
  if (pub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)pub_topic.size(); i++)
    {
      out << pub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "deadline";
  out << YAML::Value << deadline;
  out << YAML::Key << "period";
  out << YAML::Value << period;
  out << YAML::Key << "run_time";
  out << YAML::Value << run_time;

  out << YAML::EndMap;
  out << YAML::EndSeq;

  std::ofstream outputfile(file_name.c_str(), std::ios::app);
  outputfile << out.c_str() << "\n\n";
  outputfile.close();
}

/* for Scheduler*/
void Parser::create_yaml_file(std::string name, int core, std::vector<std::string> pub_topic,
                              std::vector<std::string> sub_topic, std::vector<SchedInfo> sched_infos)
{
  std::string file_name = "../../YAMLs/scheduler_rosch.yaml";
  YAML::Emitter out;
  out << YAML::BeginSeq;
  out << YAML::BeginMap;
  out << YAML::Key << "nodename";
  out << YAML::Value << name;
  out << YAML::Key << "core";
  out << YAML::Value << core;

  out << YAML::Key << "sub_topic";
  if (sub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)sub_topic.size(); i++)
    {
      out << sub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "pub_topic";
  if (pub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)pub_topic.size(); i++)
    {
      out << pub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "sched_info";
  for (int i(0); i < (int)sched_infos.size(); i++)
  {
    out << YAML::BeginSeq << YAML::BeginMap;
    out << YAML::Key << "core" << YAML::Value << sched_infos.at(i).core;
    out << YAML::Key << "priority" << YAML::Value << sched_infos.at(i).priority;
    out << YAML::Key << "run_time" << YAML::Value << sched_infos.at(i).runtime;
    out << YAML::EndMap << YAML::EndSeq;
  }
  out << YAML::EndMap;
  out << YAML::EndSeq;

  std::ofstream outputfile(file_name.c_str(), std::ios::app);
  outputfile << out.c_str() << "\n\n";
  outputfile.close();
}

/* for Tracer */
void Parser::create_yaml_file(std::string name, std::vector<std::string> pub_topic, std::vector<std::string> sub_topic,
                              int deadline)
{
  std::string file_name = "../../YAMLs/tracer_rosch.yaml";
  YAML::Emitter out;

  out << YAML::BeginSeq;
  out << YAML::BeginMap;
  out << YAML::Key << "nodename";
  out << YAML::Value << name;

  out << YAML::Key << "sub_topic";
  if (sub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)sub_topic.size(); i++)
    {
      out << sub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "pub_topic";
  if (pub_topic.size() == 0)
  {
    out << YAML::Value << "null";
  }
  else
  {
    out << YAML::Value << YAML::BeginSeq;
    for (int i(0); i < (int)pub_topic.size(); i++)
    {
      out << pub_topic.at(i);
    }
    out << YAML::EndSeq;
  }

  out << YAML::Key << "deadline";
  out << YAML::Value << deadline;
  out << YAML::EndMap;
  out << YAML::EndSeq;

  std::ofstream outputfile(file_name.c_str(), std::ios::app);
  outputfile << out.c_str() << "\n\n";

  outputfile.close();
}

std::vector<std::string> Parser::get_node_list()
{
  std::vector<std::string> topics;
  std::string cmd = "rosnode list";
  FILE *pid_fp;
  char buf[1024];

  if ((pid_fp = popen(cmd.c_str(), "r")) == NULL)
    exit(1);

  topics.resize(0);
  while (fgets(buf, sizeof(buf), pid_fp) != NULL)
  {
    buf[strlen(buf) - 1] = '\0';
    topics.push_back(buf);
  }

  pclose(pid_fp);

  return topics;
}

DependInfo Parser::get_depend_info(std::string topic)
{
  DependInfo depend_info;
  std::string topics;
  std::string cmd = "rosnode info ";
  FILE *pid_fp;
  char buf[1024];
  char sub[] = "Subscriptions";
  char pub[] = "Publications";
  char none[] = "None";
  char asterisk[] = "*";
  char *sp1;

  cmd += topic;

  if ((pid_fp = popen(cmd.c_str(), "r")) == NULL)
    exit(1);

  topics.resize(0);
  while (fgets(buf, sizeof(buf), pid_fp) != NULL)
  {
    buf[strlen(buf) - 1] = '\0';

    /* detect pubrication */
    if (sp1 = strstr(buf, pub))
    {
      /* if none */
      if (sp1 = strstr(sp1, none))
      {
        // std::cout << "detect non pub" << std::endl;
      }
      else
      {
        while (fgets(buf, sizeof(buf), pid_fp) != NULL)
        {
          buf[strlen(buf) - 1] = '\0';
          if (sp1 = strstr(buf, asterisk))
          {
            topics = get_topic(sp1);
            depend_info.pub_topic.push_back(topics);
          }
          else
          {
            break;
          }
        }
      }
    }

    /* detect subscripution */
    if (sp1 = strstr(buf, sub))
    {
      /* if none */
      if (sp1 = strstr(sp1, none))
      {
        // std::cout << "detect non sub" << std::endl;
        /* sp1 is null */
      }
      else
      {
        while (fgets(buf, sizeof(buf), pid_fp) != NULL)
        {
          buf[strlen(buf) - 1] = '\0';
          if (sp1 = strstr(buf, asterisk))
          {
            topics = get_topic(sp1);
            depend_info.sub_topic.push_back(topics);
          }
          else
          {
            break;
          }
        }
      }
    }
  }
  pclose(pid_fp);

  return depend_info;
}

void Parser::create_file(std::vector<node_info_t> infos, int mode)
{
  file_clear(mode);
  std::sort(infos.begin(), infos.end());

  for (int i(0); i < (int)infos.size(); i += 2)
  {
    switch (mode)
    {
      case 1:
        /* for measurer */
        create_yaml_file(infos[i].name, infos[i].core, infos[i].depend.pub_topic,
                         infos[i].depend.sub_topic);
        std::cout << "[Measurer] Add: " << infos[i].name.c_str() << std::endl;
        break;

      case 2:
        /* for Analyzer */
        create_yaml_file(infos[i].name, infos[i].core, infos[i].depend.pub_topic,
                         infos[i].depend.sub_topic, infos[i].deadline, 0, /* period */
                         0                                                /* run_time */
                         );
        std::cout << "[Analyzer] Add: " << infos[i].name.c_str() << std::endl;
        break;
      case 3:
        /* for scheduler*/
        create_yaml_file(infos[i].name, infos[i].core, infos[i].depend.pub_topic, infos[i].depend.sub_topic,
                         infos[i].sched_info);
        std::cout << "[Scheduler] Add: " << infos[i].name.c_str() << std::endl;
        break;
      case 4:
        /* for tracer */
        create_yaml_file(infos[i].name, infos[i].depend.pub_topic, infos[i].depend.sub_topic, infos[i].deadline);
        std::cout << "[Tracer] Add: " << infos[i].name.c_str() << std::endl;
        break;
      default:
        std::cout << "Please check mode" << std::endl;
        break;
    }
  }
}

void Parser::preview_topics_depend(std::vector<node_info_t> infos)
{
  std::ofstream outputfile("Graph.dot");
  outputfile << "digraph "
             << "Topics{"
             << "\n";

  std::sort(infos.begin(), infos.end());

  /* create nodes */
  for (int i(0); i < (int)infos.size(); i += 2)
    outputfile << infos[i].index << "[label=\"" << infos[i].name << "\"];"
               << "\n";

  /* create edges */
  for (int i(0); i < (int)infos.size(); i += 2)
  {
    for (int j(0); j < (int)infos.size(); j += 2)
    {
      for (int k(0); k < (int)infos[i].depend.sub_topic.size(); k++)
      {
        for (int l(0); l < (int)infos[j].depend.pub_topic.size(); l++)
        {
          if (infos[i].depend.sub_topic[k] == infos[j].depend.pub_topic[l])
          {
            outputfile << infos[j].index << "->" << infos[i].index << " ;"
                       << "\n";
          }
        }
      }
    }
  }

  outputfile << "}";
  outputfile.close();

  system("dot -Tpng Graph.dot -o Graph.png");
  system("eog Graph.png &");
}

std::string Parser::get_topic(char *line)
{
  std::vector<std::string> res;
  res = split(line, " ");
  return res.at(1);
}

std::vector<std::string> Parser::split(std::string str, std::string delim)
{
  std::vector<std::string> items;
  std::size_t dlm_idx;
  if (str.npos == (dlm_idx = str.find_first_of(delim)))
  {
    items.push_back(str.substr(0, dlm_idx));
  }
  while (str.npos != (dlm_idx = str.find_first_of(delim)))
  {
    if (str.npos == str.find_first_not_of(delim))
    {
      break;
    }
    items.push_back(str.substr(0, dlm_idx));
    dlm_idx++;
    str = str.erase(0, dlm_idx);
    if (str.npos == str.find_first_of(delim) && "" != str)
    {
      items.push_back(str);
      break;
    }
  }
  return items;
}

bool Parser::checkFileExistence(const std::string &str)
{
  std::ifstream ifs(str.c_str());
  return ifs.is_open();
}

void Parser::file_clear(int mode)
{
  std::string file_name;

  switch (mode)
  {
    case 1:
      file_name = "../../YAMLs/measurer_rosch.yaml";
      break;
    case 2:
      file_name = "../../YAMLs/analyzer_rosch.yaml";
      break;
    case 3:
      file_name = "../../YAMLs/scheduler_rosch.yaml";
      break;
    case 4:
      file_name = "../../YAMLs/tracer_rosch.yaml";
      break;
    default:
      break;
  }

  if (checkFileExistence(file_name))
  {
    std::ofstream ofs;
    ofs.open(file_name.c_str(), std::ios::out | std::ios::trunc);
    ofs.close();
  }
}

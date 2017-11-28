#include "ros_rosch/analyzer.hpp"
#include "ros_rosch/node_graph.hpp"
#include "ros_rosch/core_count_manager.hpp"
#include <ctime>
#include <float.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <resch/api.h>
#include <sstream>

#define _GNU_SOURCE 1

using namespace rosch;

std::string remove_begin_slash(std::string source,
                               const std::string &replace_source)
{
    std::string::size_type pos(source.find(replace_source));
    if (pos == 0)
    {
#if 0
        std::cout << "pos:" << pos << std::endl;
#endif
        source.erase(source.begin());
    }
#if 0
    std::cout << "pos:" << pos << "source:" << source <<  std::endl;
#endif

    return source;
}

std::string replace_all(std::string source,
                        const std::string &replace_source,
                        const std::string &replace_target)
{
    std::string::size_type pos(source.find(replace_source));

    while (pos != std::string::npos)
    {
        source.replace(pos, replace_source.length(), replace_target);
        pos = source.find(replace_source, pos + replace_target.length());
    }

    return source;
}

template <typename T>
std::string to_string(T value)
{
    //create an output string stream
    std::ostringstream os;

    //throw the value into the string stream
    os << value;

    //convert the string stream into a string and return
    return os.str();
}

Analyzer::Analyzer(const std::string &node_name,
                   const std::string &topic,
                   const unsigned int &max_times,
                   const unsigned int &ignore_times)
    : max_analyze_times_(max_times + ignore_times), ignore_times_(ignore_times), counter_(0), max_ms_(0), min_ms_(DBL_MAX), average_ms_(0), topic_(topic), node_name_(node_name), is_aleady_rt_(false)
{
    graph_analyzer_ = &SingletonNodeGraphAnalyzer::getInstance();
    core_count_manager_ = &SingletonCoreCountManager::getInstance();
    if (is_in_node_graph() && is_target_topic())
    {
        int index = graph_analyzer_->get_node_index(node_name_);
        core_count_manager_->set_core(graph_analyzer_->get_node_core(index));
        core_ = core_count_manager_->get_core();
        // Create dir.
        std::string rosch_dir_name("./rosch/");
        mkdir(rosch_dir_name.c_str(), 0755);
        std::string dir_name(node_name);
        dir_name = remove_begin_slash(dir_name, "/");
        dir_name = replace_all(dir_name, "/", "__");
        dir_name = rosch_dir_name + dir_name;
        mkdir(dir_name.c_str(), 0755);
				std::cout << dir_name << std::endl;
        // Create output file.
        open_output_file(true);
    }
}

Analyzer::~Analyzer()
{
    if (is_in_node_graph() && is_target_topic())
    {
        ofs_.close();
    }
}

void Analyzer::start_time()
{
    ExecTime::start_time();
}

void Analyzer::end_time()
{
    ExecTime::end_time();

    if (ignore_times_ <= counter_)
    {
        if (max_ms_ < get_exec_time_ms())
            max_ms_ = get_exec_time_ms();
        if (get_exec_time_ms() < min_ms_)
            min_ms_ = get_exec_time_ms();
        int core = graph_analyzer_->get_node_core(node_name_);
        ofs_ << get_exec_time_ms() << std::endl;
    }
    if (is_in_range())
        ++counter_;
}

double Analyzer::get_max_time_ms()
{
    return max_ms_;
}

double Analyzer::get_min_time_ms()
{
    return min_ms_;
}

double Analyzer::get_exec_time_ms()
{
    return ExecTime::get_exec_time_ms();
}

bool Analyzer::is_in_range()
{
    return counter_ < max_analyze_times_ ? true : false;
}

int Analyzer::get_target_index()
{
	/* Requires consideration 
	 * ->index or ->target */
    return graph_analyzer_->get_target_node()->target;
}

void Analyzer::update_graph()
{
    std::string str;
    std::ifstream ifs_inform_("inform_rosch_.txt");
    while (getline(ifs_inform_, str))
    {
        graph_analyzer_->finish_node(std::atoi(str.c_str()));
    }
    ifs_inform_.clear();
    ifs_inform_.seekg(0, std::ios_base::beg);
}

bool Analyzer::is_in_node_graph()
{
    int index = graph_analyzer_->get_node_index(node_name_);
    return graph_analyzer_->is_in_node_graph(index);
}

bool Analyzer::is_target_node()
{
    return get_target_index() == graph_analyzer_->get_node_index(node_name_)
               ? true
               : false;
}

bool Analyzer::is_target_topic()
{
    int index = graph_analyzer_->get_node_index(node_name_);
    std::vector<std::string> sub_topic(graph_analyzer_->get_node_subtopic(index));
    for (int i = 0; i < sub_topic.size(); ++i)
    {
        if (sub_topic.at(i) == topic_)
            return true;
    }
    return false;
}

bool Analyzer::is_target()
{
    if (is_target_node() && is_target_topic())
        return true;
    return false;
}

void Analyzer::finish_myself()
{
    int index = graph_analyzer_->get_node_index(node_name_);
    graph_analyzer_->finish_topic(index, topic_);
    if (graph_analyzer_->is_empty_topic_list(index))
    {
        std::cout << "finish[core:" << core_count_manager_->get_core() << "]:" << index << std::endl;
        if (core_count_manager_->get_core() == 1)
        { // analyzed in all cores
            graph_analyzer_->finish_node(index);
            std::ofstream ofs_inform_;
            ofs_inform_.open("inform_rosch_.txt", std::ios::app);
            ofs_inform_ << index << std::endl;
            ofs_inform_.close();
        }
        else
        { // analyze next cores
            core_count_manager_->decrement();
            set_affinity(core_count_manager_->get_core());
            graph_analyzer_->refresh_topic_list(index);
        }
    }
}

bool Analyzer::check_need_refresh()
{
    return core_ == core_count_manager_->get_core() ? false : true;
}
void Analyzer::core_refresh()
{
    core_ = core_count_manager_->get_core();
    counter_ = 0;
    open_output_file(false);
}

void Analyzer::open_output_file(bool init)
{
    if (!init)
    {
        ofs_.close();
    }
    std::string rosch_dir_name("./rosch/");
    std::string dir_name(node_name_);
    dir_name = remove_begin_slash(dir_name, "/");
    dir_name = replace_all(dir_name, "/", "__");
    dir_name = rosch_dir_name + dir_name;
    // Create output file.
    std::string file_name(topic_ + "__" + to_string(core_count_manager_->get_core()) + ".csv");
    file_name = remove_begin_slash(file_name, "/");
    file_name = replace_all(file_name, "/", "__");
    file_name = dir_name + "/" + file_name;
		std::cout << file_name << std::endl;
    ofs_.open(file_name.c_str());
}

bool Analyzer::set_affinity(int core)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (int i = 0; i < core; ++i)
    {
        CPU_SET(i, &mask);
    }
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1)
    {
        perror("Failed to set CPU affinity");
        return -1;
    }
    return 1;
}

unsigned int Analyzer::get_counter()
{
    return counter_;
}

void Analyzer::set_rt()
{
    // gurad
    if (is_aleady_rt_)
    {
        return;
    }
    is_aleady_rt_ = true;

		/* you can also set SCHED_EDF */
		ros_rt_set_scheduler(SCHED_FP);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    int index = graph_analyzer_->get_node_index(node_name_);
    set_affinity(core_count_manager_->get_core());
    int prio = 99;
    ros_rt_set_priority(prio);
}

void Analyzer::set_fair()
{
    if (is_aleady_rt_)
    {
        // gurad
        cpu_set_t mask;
        CPU_ZERO(&mask);
        int core_max = sysconf(_SC_NPROCESSORS_CONF);

        set_affinity(core_max);

        ros_rt_set_scheduler(SCHED_FAIR); /* you can also set SCHED_EDF. */
        is_aleady_rt_ = false;
    }
}

std::string Analyzer::get_node_name()
{
    return node_name_;
}

std::string Analyzer::get_topic_name()
{
    return topic_;
}

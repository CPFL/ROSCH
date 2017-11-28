#include "sched_analyzer.h"
#include <limits.h>
#include <algorithm>
#include <string>
#include "config.h"
#include "node.h"
#include "node_graph.h"
#include "spec.h"
#include "yaml-cpp/yaml.h"

using namespace sched_analyzer;

SchedAnalyzer::SchedAnalyzer()
  : node_graph_analyzer_(SingletonNodeGraphAnalyzer::getInstance()), config_(), makespan_(0)
{
  std::string filename(config_.get_specpath());
  load_spec_(filename);
  v_sched_cpu_task_.resize(get_spec_core());
}

SchedAnalyzer::~SchedAnalyzer()
{
}

void SchedAnalyzer::load_spec_(const std::string &filename)
{
  std::cout << filename << std::endl;
  try
  {
    YAML::Node spec;
    spec = YAML::LoadFile(filename);
    const YAML::Node core = spec["core"];
    const YAML::Node memory = spec["memory"];
    spec_.core = core.as<int>();
    spec_.memory = core.as<int>();
  }
  catch (YAML::Exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}

int SchedAnalyzer::run()
{
  /*compute laxity and sort by laxity*/
  node_graph_analyzer_.compute_laxity();
  node_graph_analyzer_.sched_node_queue(Queue);

  /*
   * node_queue_ : sorted nodes by inv_laxity
   * v_cpu_esc : each cpu's escapsed time
   * v_sched_cpu_task_ (Main) : scheduled node on each cpu
   */
  for (int i(0); (size_t)i < Queue.size(); ++i)
  {
    /* search vacancy core */
    std::vector<sort_esc_time_t> v_cpu_esc(get_spec_core());
    get_cpu_status(v_cpu_esc);
    std::sort(v_cpu_esc.begin(), v_cpu_esc.end());

    /* create sched_node */
    /* search min start time */
    int min_start_time(0);
    get_min_start_time(Queue.at(i).id_p, min_start_time);

    /* determine core that be set sched_(v_)node_t*/
    if (Queue.at(i).core_p > 1)
    {
      while (true)
      {
        std::vector<int> v_used_core;
        std::vector<int> v_used_start_time;
        std::vector<int> v_can_use_core;
        get_can_use_core(v_used_core, &v_can_use_core);

        sched_node_info_t sched_node_info;
        sched_v_node_info_t sched_vacancy_node_info;
        get_sched_node_info_in_selected_cores(min_start_time, sched_node_info, v_can_use_core);
        int is_vacancy_node = get_sched_vacancy_node_info_in_selected_cores(Queue.at(i).runtime_p, min_start_time,
                                                                            sched_vacancy_node_info, v_can_use_core);

        if (is_vacancy_node == 1 && sched_vacancy_node_info.start_time <= sched_node_info.start_time)
        {
          v_used_core.push_back(sched_vacancy_node_info.core_index);
          v_used_start_time.push_back(sched_vacancy_node_info.start_time);
        }
        else
        {
          v_used_core.push_back(sched_node_info.core_index);
          v_used_start_time.push_back(sched_node_info.start_time);
        }
        // std::cout << v_used_core.at(0) << std::endl;

        get_can_use_core(v_used_core, &v_can_use_core);

        int success(true);
        int next_min_start_time = INT_MAX;
        show_sched_cpu_tasks();

        for (int node_core(0); node_core < Queue.at(i).core_p - 1; ++node_core)
        {
          sched_node_info_t searched_sched_node_info;
          sched_v_node_info_t searched_sched_vacancy_node_info;
          get_sched_node_info_in_selected_cores(v_used_start_time.back(), searched_sched_node_info, v_can_use_core);
          int is_vacancy_node = get_sched_vacancy_node_info_in_selected_cores(
              Queue.at(i).runtime_p, v_used_start_time.back(), searched_sched_vacancy_node_info, v_can_use_core);

          std::cout << "start time:" << v_used_start_time.back() << std::endl;
          if (is_vacancy_node == 1 &&
              (searched_sched_vacancy_node_info.start_time <= searched_sched_node_info.start_time))
          {
            std::cout << "vacancy sched start time:" << searched_sched_vacancy_node_info.start_time << std::endl;
            if (min_start_time < searched_sched_vacancy_node_info.start_time &&
                searched_sched_vacancy_node_info.start_time <= next_min_start_time)
            {
              next_min_start_time = searched_sched_vacancy_node_info.start_time;
            }
            v_used_core.push_back(searched_sched_vacancy_node_info.core_index);
            if (searched_sched_vacancy_node_info.start_time != v_used_start_time.back())
            {
              success = false;
            }
          }
          else
          {
            std::cout << "sched start time:" << searched_sched_node_info.start_time << std::endl;
            if (min_start_time < searched_sched_node_info.start_time &&
                searched_sched_node_info.start_time <= next_min_start_time)
            {
              next_min_start_time = searched_sched_node_info.start_time;
            }
            v_used_core.push_back(searched_sched_node_info.core_index);
            if (searched_sched_node_info.start_time != v_used_start_time.back())
              success = false;
          }

          get_can_use_core(v_used_core, &v_can_use_core);
        }

        if (success)
        {
          std::cout << "success" << std::endl;
          break;
        }
        else
        {
          std::cout << "fail" << std::endl;
        }
        std::cout << next_min_start_time << std::endl;
        min_start_time = next_min_start_time;
      }

      std::vector<int> v_used_core;
      std::vector<int> v_can_use_core;
      show_sched_cpu_tasks();

      for (int node_core(0); node_core < Queue.at(i).core_p; ++node_core)
      {
        get_can_use_core(v_used_core, &v_can_use_core);

        sched_node_info_t searched_sched_node_info;
        sched_v_node_info_t searched_sched_vacancy_node_info;
        get_sched_node_info_in_selected_cores(min_start_time, searched_sched_node_info, v_can_use_core);
        int is_vacancy_node = get_sched_vacancy_node_info_in_selected_cores(
            Queue.at(i).runtime_p, min_start_time, searched_sched_vacancy_node_info, v_can_use_core);
        if (is_vacancy_node == 1 &&
            (searched_sched_vacancy_node_info.start_time <= searched_sched_node_info.start_time))
        {
          v_used_core.push_back(searched_sched_vacancy_node_info.core_index);
          int run_time = Queue.at(i).runtime_p;
          int index = Queue.at(i).id_p;
          set_sched_vacancy_node(index, run_time, searched_sched_vacancy_node_info);
        }
        else
        {
          v_used_core.push_back(searched_sched_node_info.core_index);
          if (searched_sched_node_info.need_empty)
          {
            sched_node_t sched_empty_node;
            int empty_start_time = 0;
            if (v_sched_cpu_task_.at(searched_sched_node_info.core_index).empty())
            {
              empty_start_time = 0;
            }
            else
            {
              empty_start_time = v_sched_cpu_task_.at(searched_sched_node_info.core_index).back().esc_time;
            }
            create_empty_node(empty_start_time, searched_sched_node_info.start_time, sched_empty_node);
            /* set sched_empty_node to v_sched_cpu_task_ */
            v_sched_cpu_task_.at(searched_sched_node_info.core_index).push_back(sched_empty_node);
          }
          sched_node_t sched_node;
          create_sched_node(Queue.at(i).id_p, searched_sched_node_info.start_time, Queue.at(i).runtime_p, sched_node);

          /* set sched_node to v_sched_cpu_task_ */
          v_sched_cpu_task_.at(searched_sched_node_info.core_index).push_back(sched_node);
        }
      }
      continue;
    }

    sched_node_info_t sched_node_info;
    sched_v_node_info_t sched_vacancy_node_info;
    get_sched_node_info(min_start_time, sched_node_info);
    int is_vacancy_node = get_sched_vacancy_node_info(Queue.at(i).runtime_p, min_start_time, sched_vacancy_node_info);

    /* push a node instead of vacancy node */
    if (is_vacancy_node == 1 && sched_vacancy_node_info.start_time <= sched_node_info.start_time)
    {
      int run_time = Queue.at(i).runtime_p;
      int index = Queue.at(i).id_p;
      set_sched_vacancy_node(index, run_time, sched_vacancy_node_info);
    }
    /* push back in v_sched_cpu_task_ */
    else
    {
      if (sched_node_info.need_empty)
      {
        sched_node_t sched_empty_node;
        int empty_start_time = 0;
        if (v_sched_cpu_task_.at(sched_node_info.core_index).empty())
        {
          empty_start_time = 0;
        }
        else
        {
          empty_start_time = v_sched_cpu_task_.at(sched_node_info.core_index).back().esc_time;
        }
        create_empty_node(empty_start_time, sched_node_info.start_time, sched_empty_node);
        /* set sched_empty_node to v_sched_cpu_task_ */
        v_sched_cpu_task_.at(sched_node_info.core_index).push_back(sched_empty_node);
      }
      sched_node_t sched_node;
      create_sched_node(Queue.at(i).id_p, sched_node_info.start_time, Queue.at(i).runtime_p, sched_node);

      /* set sched_node to v_sched_cpu_task_ */
      v_sched_cpu_task_.at(sched_node_info.core_index).push_back(sched_node);
    }
  }

  /* show tasks on each core*/
  compute_makespan(makespan_);
  return 0;
}

void SchedAnalyzer::get_can_use_core(const std::vector<int> &v_used_core, std::vector<int> *v_can_use)
{
  v_can_use->clear();
  for (int core(0); core < get_spec_core(); ++core)
  {
    bool skip(false);
    for (int used_core_i(0); (size_t)used_core_i < v_used_core.size(); ++used_core_i)
    {
      if (v_used_core.at(used_core_i) == core)
        skip = true;
    }
    if (skip)
      continue;
    v_can_use->push_back(core);
  }
}

int SchedAnalyzer::get_cpu_status(std::vector<sort_esc_time_t> &v_cpu_status)
{
  for (int i(0); i < get_spec_core(); ++i)
  {
    if (v_sched_cpu_task_.at(i).size() == 0)
    {
      sort_esc_time_t status = { i, 0 };
      v_cpu_status.at(i) = status;
    }
    else
    {
      sort_esc_time_t status = { i, v_sched_cpu_task_.at(i).back().esc_time };
      v_cpu_status.at(i) = status;
    }
  }
  return 0;
}

int SchedAnalyzer::compute_makespan(int &makespan)
{
  for (int i(0); i < get_spec_core(); ++i)
  {
    if (!v_sched_cpu_task_.at(i).empty())
    {
      if (makespan < v_sched_cpu_task_.at(i).back().esc_time)
        makespan = v_sched_cpu_task_.at(i).back().esc_time;
    }
  }
  return 0;
}

int SchedAnalyzer::show_sched_cpu_tasks()
{
  for (int i(0); i < get_spec_core(); ++i)
  {
    for (int j(0); (size_t)j < v_sched_cpu_task_.at(i).size(); ++j)
    {
      if (v_sched_cpu_task_.at(i).at(j).empty)
      {
        std::cout << "\x1b[36m(core" << i << ")\x1b[m\x1b[35mName:\x1b[m\x1b[31mempty\x1b[m "
                                             "\x1b[34m\t\t\tlaxity:"
                  << "NULL"
                  << "\x1b[m\t\x1b[31m[" << v_sched_cpu_task_.at(i).at(j).start_time << "~"
                  << v_sched_cpu_task_.at(i).at(j).end_time << "]\x1b[m" << std::endl;
      }
      else
      {
        std::vector<node_t> v_child;
        node_graph_analyzer_.sched_child_list(v_sched_cpu_task_.at(i).at(j).node_index, v_child);

        std::cout << "\x1b[36m(core" << i
                  << ")\x1b[m\x1b[35mName:" << g[desc[v_sched_cpu_task_.at(i).at(j).node_index]].name_p
                  << "\x1b[m\x1b[34m\t\t\tlaxity:" << g[desc[v_sched_cpu_task_.at(i).at(j).node_index]].laxity_p
                  << "\x1b[m\t[" << v_sched_cpu_task_.at(i).at(j).start_time << "~"
                  << v_sched_cpu_task_.at(i).at(j).end_time << "]";
        std::cout << "\x1b[m" << std::endl;
      }
    }
    if (!v_sched_cpu_task_.at(i).empty())
      std::cout << std::endl;
  }
  std::cout << "makespan:" << makespan_ << std::endl;

  return 0;
}

int SchedAnalyzer::get_cpu_taskset(std::vector<V_sched_node> &v_sched_cpu_task)
{
  v_sched_cpu_task = v_sched_cpu_task_;
  return 0;
}

int SchedAnalyzer::get_min_start_time(int index, int &min_start_time)
{
  IndexMap list = get(boost::vertex_index, g);
  boost::graph_traits<Graph>::edge_iterator ei, ei_end;
  min_start_time = g[desc[index]].period_p;

  for (int core(0); core < get_spec_core(); ++core)
  {
    for (int l(0); (size_t)l < v_sched_cpu_task_.at(core).size(); ++l)
    {
      for (tie(ei, ei_end) = edges(g); ei != ei_end; ei++)
      {
        if ((unsigned int)index == list[target(*ei, g)])
        {
          if (g[list[source(*ei, g)]].id_p == v_sched_cpu_task_.at(core).at(l).node_index)
          {
            min_start_time = min_start_time < v_sched_cpu_task_.at(core).at(l).esc_time ?
                                 v_sched_cpu_task_.at(core).at(l).esc_time :
                                 min_start_time;
          }
        }
      }
    }
  }
  return 0;
}

int SchedAnalyzer::get_sched_node_info(const int min_start_time, sched_node_info_t &sched_node_info)
{
  /* determine core that be set sched_node_t*/
  struct score_t
  {
    int score;
    int core_index;
  };
  score_t core_score = { INT_MAX, -1 };
  sched_node_info.start_time = -1;
  sched_node_info.core_index = -1;
  sched_node_info.need_empty = false;

  /* when esc_time is less than min_start_time */
  for (int core(0); (size_t)core < v_sched_cpu_task_.size(); ++core)
  {
    int core_esc_time(0);
    if (!v_sched_cpu_task_.at(core).empty())
      core_esc_time = v_sched_cpu_task_.at(core).back().esc_time;
    if (core_score.score > min_start_time - core_esc_time && min_start_time - core_esc_time >= 0)
    {
      core_score.score = min_start_time - core_esc_time;
      core_score.core_index = core;
    }
  }
  if (INT_MAX != core_score.score)
  {
    sched_node_info.start_time = min_start_time;
    if (0 != core_score.score)
      sched_node_info.need_empty = true;
  }
  /* when min_start_time is less than esc_time */
  else
  {
    for (int core(0); (size_t)core < v_sched_cpu_task_.size(); ++core)
    {
      int core_esc_time(0);
      if (!v_sched_cpu_task_.at(core).empty())
        core_esc_time = v_sched_cpu_task_.at(core).back().esc_time;
      if (core_score.score > core_esc_time - min_start_time && core_esc_time - min_start_time >= 0)
      {
        core_score.score = core_esc_time - min_start_time;
        core_score.core_index = core;
      }
    }
    sched_node_info.start_time = v_sched_cpu_task_.at(core_score.core_index).back().esc_time;
  }
  sched_node_info.core_index = core_score.core_index;
  return 0;
}

int SchedAnalyzer::create_empty_node(const int empty_start_time, const int empty_end_time,
                                     sched_node_t &sched_empty_node)
{
  int empty_run_time = empty_end_time - empty_start_time;
  sched_empty_node.empty = true;
  sched_empty_node.run_time = empty_run_time;
  sched_empty_node.node_index = -1;
  sched_empty_node.esc_time = empty_start_time + empty_run_time;
  sched_empty_node.start_time = empty_start_time;
  sched_empty_node.end_time = empty_start_time + empty_run_time;
  return 0;
}

int SchedAnalyzer::create_sched_node(const int index, const int start_time, const int run_time,
                                     sched_node_t &sched_node)
{
  sched_node_t sn = { false, run_time, index, start_time + run_time, start_time, start_time + run_time };
  sched_node = sn;
  return 0;
}

int SchedAnalyzer::get_sched_vacancy_node_info(const int run_time, const int min_start_time,
                                               sched_v_node_info_t &sched_vacancy_node_info)
{
  /* init sched_vacancy_node_info */
  sched_vacancy_node_info.start_time = -1;
  sched_vacancy_node_info.core_index = -1;
  sched_vacancy_node_info.empty_index = -1;
  sched_vacancy_node_info.need_before_empty = false;
  sched_vacancy_node_info.need_after_empty = false;

  struct score_t
  {
    int fin_time;
    int rem_time;
    int core;
    int empty_index;
  };
  score_t score = { INT_MAX, INT_MAX, -1, -1 };

  for (int core(0); core < get_spec_core(); ++core)
  {
    for (int l(0); (size_t)l < v_sched_cpu_task_.at(core).size(); ++l)
    {
      if (v_sched_cpu_task_.at(core).at(l).empty)
      {
        if (v_sched_cpu_task_.at(core).at(l).start_time <= min_start_time)
        {
          if (run_time <= v_sched_cpu_task_.at(core).at(l).end_time - min_start_time)
          {
            score_t this_score = { min_start_time + run_time,
                                   v_sched_cpu_task_.at(core).at(l).end_time - (min_start_time + run_time), core, l };
            if (this_score.fin_time < score.fin_time)
            {
              score = this_score;
            }
            else if (this_score.fin_time == score.fin_time && this_score.rem_time < score.rem_time)
            {
              score = this_score;
            }
          }
        }
        else
        {
          if (run_time <= v_sched_cpu_task_.at(core).at(l).run_time)
          {
            score_t this_score = { v_sched_cpu_task_.at(core).at(l).start_time + run_time,
                                   v_sched_cpu_task_.at(core).at(l).end_time -
                                       (v_sched_cpu_task_.at(core).at(l).start_time + run_time),
                                   core, l };
            if (this_score.fin_time < score.fin_time)
            {
              score = this_score;
            }
            else if (this_score.fin_time == score.fin_time && this_score.rem_time < score.rem_time)
            {
              score = this_score;
            }
          }
        }
      }
    }
  }
  if (score.fin_time != INT_MAX)
  {
    sched_vacancy_node_info.start_time = score.fin_time - run_time;
    sched_vacancy_node_info.core_index = score.core;
    sched_vacancy_node_info.empty_index = score.empty_index;
    if (v_sched_cpu_task_.at(score.core).at(score.empty_index).start_time < sched_vacancy_node_info.start_time)
    {
      sched_vacancy_node_info.need_before_empty = true;
    }
    if (score.fin_time < v_sched_cpu_task_.at(score.core).at(score.empty_index).end_time)
    {
      sched_vacancy_node_info.need_after_empty = true;
    }

    return 1;
  }
  return -1;
}
int SchedAnalyzer::get_sched_node_info_in_selected_cores(const int min_start_time, sched_node_info_t &sched_node_info,
                                                         std::vector<int> v_can_use_core)
{
  /* determine core that be set sched_node_t*/
  struct score_t
  {
    int score;
    int core_index;
  };
  score_t core_score = { INT_MAX, -1 };
  sched_node_info.start_time = -1;
  sched_node_info.core_index = -1;
  sched_node_info.need_empty = false;

  std::cout << "via " << std::endl;

  /* when esc_time is less than min_start_time */
  for (int core(0); (size_t)core < v_sched_cpu_task_.size(); ++core)
  {
    bool skip(true);
    for (int i(0); (size_t)i < v_can_use_core.size(); ++i)
    {
      if (core == v_can_use_core.at(i))
        skip = false;
    }
    if (skip)
      continue;

    int core_esc_time(0);
    if (!v_sched_cpu_task_.at(core).empty())
      core_esc_time = v_sched_cpu_task_.at(core).back().esc_time;

    if (core_score.score > min_start_time - core_esc_time && min_start_time - core_esc_time >= 0)
    {
      core_score.score = min_start_time - core_esc_time;
      core_score.core_index = core;
    }
  }
  if (INT_MAX != core_score.score)
  {
    sched_node_info.start_time = min_start_time;
    if (0 != core_score.score)
      sched_node_info.need_empty = true;
  }
  /* when min_start_time is less than esc_time */
  else
  {
    for (int core(0); (size_t)core < v_sched_cpu_task_.size(); ++core)
    {
      bool skip(true);
      for (int i(0); (size_t)i < v_can_use_core.size(); ++i)
      {
        if (core == v_can_use_core.at(i))
          skip = false;
      }
      if (skip)
        continue;

      int core_esc_time(0);
      if (!v_sched_cpu_task_.at(core).empty())
        core_esc_time = v_sched_cpu_task_.at(core).back().esc_time;
      if (core_score.score > core_esc_time - min_start_time && core_esc_time - min_start_time >= 0)
      {
        core_score.score = core_esc_time - min_start_time;
        core_score.core_index = core;
      }
    }
    sched_node_info.start_time = v_sched_cpu_task_.at(core_score.core_index).back().esc_time;
  }
  sched_node_info.core_index = core_score.core_index;
  std::cout << "core: " << core_score.core_index << std::endl;
  ;
  return 0;
}
int SchedAnalyzer::get_sched_vacancy_node_info_in_selected_cores(const int run_time, const int min_start_time,
                                                                 sched_v_node_info_t &sched_vacancy_node_info,
                                                                 std::vector<int> v_can_use_core)
{ /* init sched_vacancy_node_info */
  sched_vacancy_node_info.start_time = -1;
  sched_vacancy_node_info.core_index = -1;
  sched_vacancy_node_info.empty_index = -1;
  sched_vacancy_node_info.need_before_empty = false;
  sched_vacancy_node_info.need_after_empty = false;

  struct score_t
  {
    int fin_time;
    int rem_time;
    int core;
    int empty_index;
  };
  score_t score = { INT_MAX, INT_MAX, -1, -1 };

  for (int core(0); core < get_spec_core(); ++core)
  {
    bool skip(true);
    for (int i(0); (size_t)i < v_can_use_core.size(); ++i)
    {
      if (core == v_can_use_core.at(i))
        skip = false;
    }
    if (skip)
      continue;

    for (int l(0); (size_t)l < v_sched_cpu_task_.at(core).size(); ++l)
    {
      if (v_sched_cpu_task_.at(core).at(l).empty)
      {
        if (v_sched_cpu_task_.at(core).at(l).start_time <= min_start_time)
        {
          if (run_time <= v_sched_cpu_task_.at(core).at(l).end_time - min_start_time)
          {
            score_t this_score = { min_start_time + run_time,
                                   v_sched_cpu_task_.at(core).at(l).end_time - (min_start_time + run_time), core, l };
            if (this_score.fin_time < score.fin_time)
            {
              score = this_score;
            }
            else if (this_score.fin_time == score.fin_time && this_score.rem_time < score.rem_time)
            {
              score = this_score;
            }
          }
        }
        else
        {
          if (run_time <= v_sched_cpu_task_.at(core).at(l).run_time)
          {
            score_t this_score = { v_sched_cpu_task_.at(core).at(l).start_time + run_time,
                                   v_sched_cpu_task_.at(core).at(l).end_time -
                                       (v_sched_cpu_task_.at(core).at(l).start_time + run_time),
                                   core, l };
            if (this_score.fin_time < score.fin_time)
            {
              score = this_score;
            }
            else if (this_score.fin_time == score.fin_time && this_score.rem_time < score.rem_time)
            {
              score = this_score;
            }
          }
        }
      }
    }
  }
  if (score.fin_time != INT_MAX)
  {
    sched_vacancy_node_info.start_time = score.fin_time - run_time;
    sched_vacancy_node_info.core_index = score.core;
    sched_vacancy_node_info.empty_index = score.empty_index;
    if (v_sched_cpu_task_.at(score.core).at(score.empty_index).start_time < sched_vacancy_node_info.start_time)
    {
      sched_vacancy_node_info.need_before_empty = true;
    }
    if (score.fin_time < v_sched_cpu_task_.at(score.core).at(score.empty_index).end_time)
    {
      sched_vacancy_node_info.need_after_empty = true;
    }

    return 1;
  }
  return -1;
}

int SchedAnalyzer::set_sched_vacancy_node(const int index, const int run_time,
                                          const sched_v_node_info_t &sched_vacancy_node_info)
{
  if (index < 0 || run_time < 0 || sched_vacancy_node_info.core_index < 0)
    return -1;
  int core = sched_vacancy_node_info.core_index;
  int e_index = sched_vacancy_node_info.empty_index;

  V_sched_node v_swap;

  for (int swap_i = 0; swap_i != e_index; ++swap_i)
  {
    v_swap.push_back(v_sched_cpu_task_.at(core).at(swap_i));
  }
  if (sched_vacancy_node_info.need_before_empty)
  {
    sched_node_t sched_empty_node;
    create_empty_node(v_sched_cpu_task_.at(core).at(e_index).start_time, sched_vacancy_node_info.start_time,
                      sched_empty_node);
    v_swap.push_back(sched_empty_node);
  }
  sched_node_t sched_node;
  create_sched_node(index, sched_vacancy_node_info.start_time, run_time, sched_node);
  v_swap.push_back(sched_node);
  if (sched_vacancy_node_info.need_after_empty)
  {
    sched_node_t sched_empty_node;
    create_empty_node(sched_vacancy_node_info.start_time + run_time, v_sched_cpu_task_.at(core).at(e_index).end_time,
                      sched_empty_node);
    v_swap.push_back(sched_empty_node);
  }
  for (int swap_i = e_index + 1; (size_t)swap_i != v_sched_cpu_task_.at(core).size(); ++swap_i)
  {
    v_swap.push_back(v_sched_cpu_task_.at(core).at(swap_i));
  }
  v_sched_cpu_task_.at(core).swap(v_swap);
  return 1;
}

spec_t SchedAnalyzer::get_spec()
{
  return spec_;
}
int SchedAnalyzer::get_spec_core()
{
  return spec_.core;
}

int SchedAnalyzer::get_makespan()
{
  return makespan_;
}
int SchedAnalyzer::get_node_list_size()
{
  return node_graph_analyzer_.get_node_list_size();
}
int SchedAnalyzer::get_node_name(int index, std::string &node_name)
{
  node_name = node_graph_analyzer_.get_node_name(index);
  return 0;
}

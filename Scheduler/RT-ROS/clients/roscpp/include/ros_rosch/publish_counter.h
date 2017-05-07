#ifndef PUBLISH_COUNTER_H
#define PUBLISH_COUNTER_H

#include "type.h"
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

namespace rosch {
class SingletonSchedNodeManager {
private:
  SingletonSchedNodeManager();
  SingletonSchedNodeManager(const SingletonSchedNodeManager &);
  SingletonSchedNodeManager &operator=(const SingletonSchedNodeManager &);
  ~SingletonSchedNodeManager();
  NodeInfo node_info_;
  int poll_time_ms_;
  bool missed_deadline_;
  bool running_fail_safe_function_;
  bool ran_fail_safe_function_;
  bool publish_even_if_missed_deadline_;
  class PublishCounter {
  private:
    std::vector<std::string> v_remain_pubtopic_;
    SingletonSchedNodeManager *sched_node_manager_;

  public:
    PublishCounter(SingletonSchedNodeManager *sched_node_manager);
    size_t getRemainPubTopicSize();
    size_t getPubTopicSize();
    bool removeRemainPubTopic(const std::string &topic_name);
    bool isRemainPubTopic(const std::string &topic_name);
    void resetRemainPubTopic();
  };

  class SubscribeCounter {
  private:
    std::vector<std::string> v_remain_subtopic_;
    SingletonSchedNodeManager *sched_node_manager_;

  public:
    SubscribeCounter(SingletonSchedNodeManager *sched_node_manager);
    size_t getRemainSubTopicSize();
    size_t getSubTopicSize();
    bool removeRemainSubTopic(const std::string &topic_name);
    void resetRemainSubTopic();
  };

public:
  static SingletonSchedNodeManager &getInstance();
  PublishCounter publish_counter;
  SubscribeCounter subscribe_counter;
  NodeInfo getNodeInfo();
  void setNodeInfo(const NodeInfo &node_info);
  void subPollTime(int time_ms);
  int getPollTime();
  void resetPollTime();
  void init(const NodeInfo &node_info);
  void missedDeadline();
  bool isDeadlineMiss();
  void resetDeadlineMiss();
  std::vector<int> getUseCores();
  int getPriority();
  void runFailSafeFunction();
  bool isRunningFailSafeFunction();
  bool isRanFailSafeFunction();
  void resetFailSafeFunction();
  bool publishEvenIfMissedDeadline();
  void setPublishEvenIfMissedDeadline(bool can_publish);
  void (*func)(void);
  std::vector<pid_t> v_pid;
};
}

#endif // ANALYZER_HPP

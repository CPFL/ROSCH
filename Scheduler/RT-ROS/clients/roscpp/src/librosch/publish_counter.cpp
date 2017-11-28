#include "ros_rosch/publish_counter.h"
#include <iostream>
using namespace rosch;

SingletonSchedNodeManager::SingletonSchedNodeManager()
    : publish_counter(this), subscribe_counter(this), poll_time_ms_(0),
      missed_deadline_(false), func(NULL), ran_fail_safe_function_(false),
      running_fail_safe_function_(false),
      publish_even_if_missed_deadline_(false) {}
SingletonSchedNodeManager::~SingletonSchedNodeManager() {}

SingletonSchedNodeManager &SingletonSchedNodeManager::getInstance() {
  static SingletonSchedNodeManager inst;
  return inst;
}
void SingletonSchedNodeManager::setNodeInfo(const NodeInfo &node_info) {
  node_info_ = node_info;
}
void SingletonSchedNodeManager::resetPollTime() {
  if (node_info_.v_subtopic.size() > 0)
    poll_time_ms_ = node_info_.v_sched_info.at(0).run_time;
}
void SingletonSchedNodeManager::subPollTime(int time_ms) {
  poll_time_ms_ = (poll_time_ms_ - time_ms) < 0 ? 0 : (poll_time_ms_ - time_ms);
}
int SingletonSchedNodeManager::getPollTime() { return poll_time_ms_; }
NodeInfo SingletonSchedNodeManager::getNodeInfo() { return node_info_; }
void SingletonSchedNodeManager::init(const NodeInfo &node_info) {
  setNodeInfo(node_info);
  resetPollTime();
  publish_counter.resetRemainPubTopic();
  subscribe_counter.resetRemainSubTopic();
}
void SingletonSchedNodeManager::missedDeadline() { missed_deadline_ = true; }
bool SingletonSchedNodeManager::isDeadlineMiss() { return missed_deadline_; }
void SingletonSchedNodeManager::resetDeadlineMiss() {
  missed_deadline_ = false;
}
void SingletonSchedNodeManager::resetFailSafeFunction() {
  ran_fail_safe_function_ = false;
}

bool SingletonSchedNodeManager::publishEvenIfMissedDeadline() {
  return publish_even_if_missed_deadline_;
}
void SingletonSchedNodeManager::setPublishEvenIfMissedDeadline(
    bool can_publish) {
  publish_even_if_missed_deadline_ = can_publish;
}

void SingletonSchedNodeManager::runFailSafeFunction() {
  running_fail_safe_function_ = true;
  if (func)
    func();
  running_fail_safe_function_ = false;
  ran_fail_safe_function_ = true;
}
bool SingletonSchedNodeManager::isRunningFailSafeFunction() {
  return running_fail_safe_function_;
}
bool SingletonSchedNodeManager::isRanFailSafeFunction() {
  return ran_fail_safe_function_;
}
std::vector<int> SingletonSchedNodeManager::getUseCores() {
  std::vector<int> v_core;

	if(node_info_.is_single_process == false){ 
    for (int i = 0; i < (int)node_info_.v_sched_info.size(); ++i) {
      v_core.push_back(node_info_.v_sched_info.at(i).core);
	  }
	}
	else{
		v_core.push_back(node_info_.v_sched_info.at(node_info_.period_count).core);
		if(node_info_.v_sched_info.size() -1 > node_info_.period_count)
			node_info_.period_count++;
		else
			node_info_.period_count = 0;
	}
  return v_core;
}

int SingletonSchedNodeManager::getPriority() {
  if (node_info_.v_sched_info.size() > 0){
		if(node_info_.is_single_process == false)
	    return node_info_.v_sched_info.at(0).priority;
		else{
			return node_info_.v_sched_info.at(node_info_.period_count).priority;
		}
	}
  else
    return -1;
}

SingletonSchedNodeManager::PublishCounter::PublishCounter(
    SingletonSchedNodeManager *sched_node_manager)
    : sched_node_manager_(sched_node_manager) {}
void SingletonSchedNodeManager::PublishCounter::resetRemainPubTopic() {
  v_remain_pubtopic_ = sched_node_manager_->node_info_.v_pubtopic;
}
size_t SingletonSchedNodeManager::PublishCounter::getRemainPubTopicSize() {
  return v_remain_pubtopic_.size();
}
size_t SingletonSchedNodeManager::PublishCounter::getPubTopicSize() {
  return sched_node_manager_->node_info_.v_pubtopic.size();
}
bool SingletonSchedNodeManager::PublishCounter::removeRemainPubTopic(
    const std::string &topic_name) {
  std::vector<std::string>::iterator itr = v_remain_pubtopic_.begin();
  while (itr != v_remain_pubtopic_.end()) {
    if (*itr == topic_name) {
      itr = v_remain_pubtopic_.erase(itr);
      return true;
    } else {
      itr++;
    }
  }
  return false;
}

bool SingletonSchedNodeManager::PublishCounter::isRemainPubTopic(
    const std::string &topic_name) {
  std::vector<std::string>::iterator itr = v_remain_pubtopic_.begin();
  while (itr != v_remain_pubtopic_.end()) {
    if (*itr == topic_name) {
      return true;
    } else {
      itr++;
    }
  }
  return false;
}

SingletonSchedNodeManager::SubscribeCounter::SubscribeCounter(
    SingletonSchedNodeManager *sched_node_manager)
    : sched_node_manager_(sched_node_manager) {}
void SingletonSchedNodeManager::SubscribeCounter::resetRemainSubTopic() {
  v_remain_subtopic_ = sched_node_manager_->node_info_.v_subtopic;
}
size_t SingletonSchedNodeManager::SubscribeCounter::getRemainSubTopicSize() {
  return v_remain_subtopic_.size();
}
size_t SingletonSchedNodeManager::SubscribeCounter::getSubTopicSize() {
  return sched_node_manager_->node_info_.v_subtopic.size();
}
bool SingletonSchedNodeManager::SubscribeCounter::removeRemainSubTopic(
    const std::string &topic_name) {
  std::vector<std::string>::iterator itr = v_remain_subtopic_.begin();
  while (itr != v_remain_subtopic_.end()) {
    if (*itr == topic_name) {
      itr = v_remain_subtopic_.erase(itr);
      return true;
    } else {
      itr++;
    }
  }
  return false;
}

/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ros/subscription_queue.h"
#include "ros/message_deserializer.h"
#include "ros/subscription_callback_helper.h"
//#define ROSCH
//#ifdef ROSCH
#include <resch/api.h>
#include "ros_rosch/bridge.hpp"
//#include "ros_rosch/node_graph.hpp"
//#include <ctime>
//#include <sched.h>
//#endif
#define ROSCHEDULER
#ifdef ROSCHEDULER
#include "ros_rosch/event_notification.hpp"
#include "ros_rosch/publish_counter.h"
#include "ros_rosch/task_attribute_processer.h"
#include "ros_rosch/type.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/timer/timer.hpp>
#include <poll.h>
#include <ros/ros.h>
#endif

namespace ros {

SubscriptionQueue::SubscriptionQueue(const std::string &topic,
                                     int32_t queue_size,
                                     bool allow_concurrent_callbacks)
    : topic_(topic), size_(queue_size), full_(false), queue_size_(0),
      allow_concurrent_callbacks_(allow_concurrent_callbacks)
#ifdef ROSCH
      ,
      analyzer(rosch::get_node_name(), topic)
#endif
#ifdef ROSCHEDULER
      ,
      sched_node_manager_(rosch::SingletonSchedNodeManager::getInstance())
#endif
{
}

SubscriptionQueue::~SubscriptionQueue() {}

void SubscriptionQueue::push(const SubscriptionCallbackHelperPtr &helper,
                             const MessageDeserializerPtr &deserializer,
                             bool has_tracked_object,
                             const VoidConstWPtr &tracked_object,
                             bool nonconst_need_copy, ros::Time receipt_time,
                             bool *was_full) {
  boost::mutex::scoped_lock lock(queue_mutex_);

  if (was_full) {
    *was_full = false;
  }

  if (fullNoLock()) {
    queue_.pop_front();
    --queue_size_;

    if (!full_) {
      ROS_DEBUG("Incoming queue full for topic \"%s\".  Discarding oldest "
                "message (current queue size [%d])",
                topic_.c_str(), (int)queue_.size());
    }

    full_ = true;

    if (was_full) {
      *was_full = true;
    }
  } else {
    full_ = false;
  }

  Item i;
  i.helper = helper;
  i.deserializer = deserializer;
  i.has_tracked_object = has_tracked_object;
  i.tracked_object = tracked_object;
  i.nonconst_need_copy = nonconst_need_copy;
  i.receipt_time = receipt_time;
  queue_.push_back(i);
  ++queue_size_;
}

void SubscriptionQueue::clear() {
  boost::recursive_mutex::scoped_lock cb_lock(callback_mutex_);
  boost::mutex::scoped_lock queue_lock(queue_mutex_);

  queue_.clear();
  queue_size_ = 0;
}

CallbackInterface::CallResult SubscriptionQueue::call() {
  // The callback may result in our own destruction.  Therefore, we may need
  // to
  // keep a reference to ourselves
  // that outlasts the scoped_try_lock
  boost::shared_ptr<SubscriptionQueue> self;
  boost::recursive_mutex::scoped_try_lock lock(callback_mutex_,
                                               boost::defer_lock);

  if (!allow_concurrent_callbacks_) {
    lock.try_lock();
    if (!lock.owns_lock()) {
      return CallbackInterface::TryAgain;
    }
  }

  VoidConstPtr tracker;
  Item i;

  {
    boost::mutex::scoped_lock lock(queue_mutex_);

    if (queue_.empty()) {
      return CallbackInterface::Invalid;
    }

    i = queue_.front();

    if (queue_.empty()) {
      return CallbackInterface::Invalid;
    }

    if (i.has_tracked_object) {
      tracker = i.tracked_object.lock();

      if (!tracker) {
        return CallbackInterface::Invalid;
      }
    }

    queue_.pop_front();
    --queue_size_;
  }

  VoidConstPtr msg = i.deserializer->deserialize();

  // msg can be null here if deserialization failed
  if (msg) {
    try {
      self = shared_from_this();
    } catch (boost::bad_weak_ptr
                 &) // For the tests, where we don't create a shared_ptr
    {
    }
    SubscriptionCallbackHelperCallParams params;
    params.event = MessageEvent<void const>(
        msg, i.deserializer->getConnectionHeader(), i.receipt_time,
        i.nonconst_need_copy, MessageEvent<void const>::CreateFunction());

    std::cout << "-------------------------------------" << std::endl;
    if (sched_node_manager_.subscribe_counter.removeRemainSubTopic(topic_)) {
      boost::thread app_th(
          boost::bind(&ros::SubscriptionQueue::appThread, this, i, params));
      waitAppThread();
    } else {
      i.helper->call(params);
    }
    std::cout << "-------------------------------------" << std::endl;
    if (0 == sched_node_manager_.subscribe_counter.getRemainSubTopicSize()) {
      sched_node_manager_.subscribe_counter.resetRemainSubTopic();
      sched_node_manager_.publish_counter.resetRemainPubTopic();
      sched_node_manager_.resetDeadlineMiss();
      sched_node_manager_.resetPollTime();
      sched_node_manager_.resetFailSafeFunction();
      std::cout
          << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*"
          << std::endl;
    }
  }

  return CallbackInterface::Success;
}

// ROSCHEDULER
void SubscriptionQueue::waitAppThread() {
  boost::timer::cpu_timer timer;
  int ret;

  ret = event_notification.update(sched_node_manager_.getPollTime());
  //  std::cout << "Poll time: " << sched_node_manager_.getPollTime() <<
  //  std::endl;


#ifndef USE_LINUX_SYSTEM_CALL
#ifdef HYPERPERIOD_MODE
	cpu_set_t mask;
	CPU_ZERO(&mask);

	set_affinity(sched_node_manager_.getUseCores());
	ros_rt_set_priority(sched_node_manager_.getPriority());
#endif
#endif
	
  if (ret != 1) {
		std::cout << "PID = " << getpid() << std::endl;
    std::cout << "==== Deadline miss!! ====" << std::endl;
    std::cout << "Remain Topics: "
              << sched_node_manager_.publish_counter.getRemainPubTopicSize()
              << "/" << sched_node_manager_.publish_counter.getPubTopicSize()
              << std::endl;
    sched_node_manager_.missedDeadline();
    if (!sched_node_manager_.isRanFailSafeFunction())
      sched_node_manager_.runFailSafeFunction();
    std::cout << "========================" << std::endl;
    rosch::TaskAttributeProcesser task_attr_proc;
    task_attr_proc.setDefaultScheduling(sched_node_manager_.v_pid);
    ret = event_notification.update(-1);
    task_attr_proc.setCoreAffinity(sched_node_manager_.getUseCores());
  } else {
    std::cout << "==== Finished before the deadline ====" << std::endl;
    std::cout << "Remain Topics: "
              << sched_node_manager_.publish_counter.getRemainPubTopicSize()
              << "/" << sched_node_manager_.publish_counter.getPubTopicSize()
              << std::endl;
    std::cout << "========================" << std::endl;
  }

  int elapsed_time_ms = (int)((double)timer.elapsed().wall) / 1000000.0;
  sched_node_manager_.subPollTime(elapsed_time_ms);
  std::cout << "Elapsed time: " << elapsed_time_ms << std::endl
            << "Remain poll time:" << sched_node_manager_.getPollTime()
            << std::endl;
}

void SubscriptionQueue::appThread(Item i,
                                  SubscriptionCallbackHelperCallParams params) {
  i.helper->call(params);
  event_notification.signal();
}

bool SubscriptionQueue::ready() { return true; }

bool SubscriptionQueue::full() {
  boost::mutex::scoped_lock lock(queue_mutex_);
  return fullNoLock();
}

bool SubscriptionQueue::fullNoLock() {
  return (size_ > 0) && (queue_size_ >= (uint32_t)size_);
}
}

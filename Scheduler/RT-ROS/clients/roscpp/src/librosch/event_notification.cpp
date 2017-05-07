/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2016, Yukihiro Saito.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "ros_rosch/event_notification.hpp"

#include <boost/bind.hpp>

#include <fcntl.h>
#include <iostream>
#include <poll.h> // should get cmake to explicitly check for poll.h?
#include <sys/poll.h>

namespace rosch {

EventNotification::EventNotification() {
  if (createSignalPair(signal_pipe_) != 0) {
    std::cerr << "create_signal_pair() failed" << std::endl;
    exit(-1);
  }
}

EventNotification::~EventNotification() {
  //  close_signal_pair(signal_pipe_);
  ::close(signal_pipe_[0]);
  ::close(signal_pipe_[1]);
}

int EventNotification::createSignalPair(int signal_pair[2]) {
  signal_pair[0] = -1;
  signal_pair[1] = -1;

  if (pipe(signal_pair) != 0) {
    std::cerr << "pipe() failed" << std::endl;
    return -1;
  }
  if (fcntl(signal_pair[0], F_SETFL, O_NONBLOCK) == -1) {
    std::cerr << "fcntl() failed" << std::endl;
    return -1;
  }
  if (fcntl(signal_pair[1], F_SETFL, O_NONBLOCK) == -1) {
    std::cerr << "fcntl() failed" << std::endl;
    return -1;
  }
  return 0;
}

void EventNotification::signal() {
  boost::mutex::scoped_try_lock lock(signal_mutex_);

  if (lock.owns_lock()) {
    char b = '0';
    //    if (write_signal(signal_pipe_[1], &b, 1) < 0)
    if (write(signal_pipe_[1], &b, 1) < 0) {
      // do nothing... this prevents warnings on gcc 4.3
    }
  }
}

int EventNotification::update(int poll_timeout) {
  // Poll across the sockets we're servicing
  int ret;
  int events = POLLIN;
  struct pollfd poll_fd = {signal_pipe_[0], events, 0};

  if ((ret = poll(&poll_fd, 1, poll_timeout)) < 0) {
    std::cerr << "poll failed with error " << std::endl;
    return -1;
  } else if (ret == 0) {
    return 0;
  } else if (ret > 0) // ret = 0 implies the poll timed out, nothing to do
  {
    // We have one or more sockets to service
    if (poll_fd.revents == 0) {
      return -1;
    }

    // If these are registered events for this socket, OR the events are
    // ERR/HUP/NVAL,
    // call through to the registered function
    int revents = poll_fd.revents;
    if (((events & revents) || (revents & POLLERR) || (revents & POLLHUP) ||
         (revents & POLLNVAL))) {
      poll_fd.revents = 0;
      if (revents & (POLLNVAL | POLLERR | POLLHUP)) {
        return -1;
      }
      onLocalPipeEvents(revents & (events | POLLERR | POLLHUP | POLLNVAL));
    }
  }
  return 1;
}

void EventNotification::onLocalPipeEvents(int events) {
  if (events & POLLIN) {
    char b;
    //    while(read_signal(signal_pipe_[0], &b, 1) > 0)
    while (read(signal_pipe_[0], &b, 1) > 0) {
      // std::cout << "read bite : " << b << std::endl;
      // do nothing keep draining
    };
  }
}
}

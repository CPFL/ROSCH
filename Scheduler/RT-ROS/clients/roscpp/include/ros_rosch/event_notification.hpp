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

#ifndef EVENT_NOTIFICATON_HPP
#define EVENT_NOTIFICATON_HPP

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace rosch {

class EventNotification {
public:
  EventNotification();
  ~EventNotification();

  /**
   * \brief Process all socket events
   *
   * This function will actually call poll() on the available sockets, and allow
   * them to do their processing.
   *
   * update() may only be called from one thread at a time
   *
   * \param poll_timeout The time, in milliseconds, for the poll() call to
   * timeout after
   * if there are no events.  Note that this does not provide an upper bound for
   * the entire
   * function, just the call to poll()
   */
  int update(int poll_timeout);

  /**
   * \brief Signal our poll() call to finish if it's blocked waiting (see the
   * poll_timeout
   * option for update()).
   */
  void signal();

private:
  /**
   *
   */
  int createSignalPair(int signal_pair[2]);

  /**
   * \brief Called when events have been triggered on our signal pipe
   */
  void onLocalPipeEvents(int events);

  boost::mutex signal_mutex_;
  int signal_pipe_[2];
};
}

#endif // EVENT_NOTIFICATON_HPP

#include "ros_rosch/exec_time.hpp"
#include <ctime>

using namespace rosch;

ExecTime::ExecTime()
    :start_time_ms(0),
      end_time_ms(0),
      exec_time_ms(0)
{
}

ExecTime::~ExecTime()
{
}

void ExecTime::start_time() {
    start_time_ms = get_current_time_ms();
}

void ExecTime::end_time() {
    end_time_ms = get_current_time_ms();
    exec_time_ms = end_time_ms>start_time_ms ? end_time_ms-start_time_ms : 0;
}

double ExecTime::get_exec_time_ms() {
    return exec_time_ms;
}

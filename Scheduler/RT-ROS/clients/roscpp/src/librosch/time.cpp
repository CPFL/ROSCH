#include "ros_rosch/time.hpp"
#include <ctime>

using namespace rosch;

double Time::get_current_time_ms() {
    timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return (double)time.tv_sec*1000L + (double)time.tv_nsec/1000000L;
}

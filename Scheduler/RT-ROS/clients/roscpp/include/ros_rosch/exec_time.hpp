#ifndef EXEC_TIME_HPP
#define EXEC_TIME_HPP

#include "ros_rosch/time.hpp"

namespace rosch {
class ExecTime : Time {
public:
    ExecTime();
    ~ExecTime();
    void start_time();
    void end_time();
    double get_exec_time_ms();
private:
    double start_time_ms;
    double end_time_ms;
    double exec_time_ms;
};
}


#endif // EXEC_TIME_HPP

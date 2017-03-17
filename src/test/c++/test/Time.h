#pragma once

#include <ctime>
#include <chrono>

namespace tbp
{
namespace log
{
namespace test
{

inline std::chrono::nanoseconds Now() 
{
   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   return std::chrono::nanoseconds(1000000000UL * ts.tv_sec + ts.tv_nsec);
}

}
}
}


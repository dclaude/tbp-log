#pragma once

#include <cstdint>

namespace tbp
{
namespace log
{

enum class Level : std::uint8_t
{
   none,
   debug,
   info,
   warn,
   error,
   critical,
};

}
}


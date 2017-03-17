#pragma once

#include "tbp/log/CategoryId.h"
#include "tbp/log/Level.h"
#include <string>

namespace tbp
{
namespace log
{

/*
this interface is not used on the critical path
it is used to be able to handle different types of logger in the same Loggers
*/
class ILogger
{
public:
   virtual ~ILogger() {}
   //
   virtual const std::string& GetName() const = 0;
   virtual void SetLevel(CategoryId id, Level level) = 0;
};

}
}


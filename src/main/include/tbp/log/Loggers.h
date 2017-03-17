#pragma once

#include "tbp/log/Level.h"
#include "tbp/log/Categories.h"
#include "tbp/log/Config.h"
#include <map>
#include <mutex>
#include <memory>

namespace tbp
{
   namespace log
   {
      class ILogger;
      class Injector;
   }
}

namespace tbp
{
namespace log
{

/*
- the same Loggers object can be used by any thread (to change the log level, create a new logger, ...)
- resources
https://github.com/gabime/spdlog
https://github.com/KjellKod/g3log
*/
class Loggers
{
public:
   Loggers(const Categories& categories, const Config& config, const Injector& injector);
   //
   void AddLogger(ILogger& logger);
   bool RemoveLogger(const ILogger& logger);
   const Categories& GetCategories() const { return m_categories; }
   bool SetLevel(const Category& category, Level level);
   bool SetLevels(Level level);
   const Config& GetConfig() const { return m_config; }
   const Injector& GetInjector() const { return m_injector; }

private:
   bool SetLevelImpl(const Category& category, Level level);
   //
   std::mutex m_mutex;
   std::map<std::string, ILogger*> m_loggers; // must only be used to change "std::atomic" properties in the ILogger (log level, ...)
   Categories m_categories;
   std::map<CategoryId, Level> m_levels;
   Config m_config;
   const Injector& m_injector;

};

}
}


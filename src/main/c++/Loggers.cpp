#include "tbp/log/Loggers.h"
#include "tbp/log/ILogger.h"
#include "tbp/common/ConfigurationException.h"
#include <sstream>
#include <cassert>

using std::string;

namespace tbp
{
namespace log
{

Loggers::Loggers(const Categories& categories, const Config& config, const Injector& injector)
   : m_categories(categories), m_config(config), m_injector(injector)
{
   m_categories.ForEach([this](CategoryId id, const Category& category)
   {
      m_levels.emplace(id, category.GetInitialLevel());
   });
}

void Loggers::AddLogger(ILogger& logger)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   //
   const string& loggerName = logger.GetName();
   auto iter = m_loggers.find(loggerName);
   if (iter != m_loggers.end())
   {
      std::ostringstream oss;
      oss << "Loggers::CreateThreadLocalLogger Logger with name[" << loggerName << "] already exists";
      throw common::ConfigurationException(oss.str());
   }
   for (const auto& p : m_levels)
   {
      logger.SetLevel(p.first, p.second);
   }
   m_loggers.emplace(loggerName, &logger);
}

bool Loggers::RemoveLogger(const ILogger& logger)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   //
   auto count = m_loggers.erase(logger.GetName());
   assert(count == 1);
   return count == 1 ? true : false;
}

bool Loggers::SetLevelImpl(const Category& category, Level level)
{
   CategoryId id = m_categories.FindCategoryId(category);
   if (id <= 0)
   {
      assert(false);
      return false;
   }
   auto iter = m_levels.find(id);
   if (iter == m_levels.end())
   {
      assert(false);
      return false;
   }
   iter->second = level;
   for (auto& p : m_loggers)
   {
      p.second->SetLevel(id, level); // std::atomic
   }
   return true;
}

bool Loggers::SetLevel(const Category& category, Level level)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   //
   return SetLevelImpl(category, level);
}

bool Loggers::SetLevels(Level level)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   //
   bool res = true;
   m_categories.ForEach([this, &res, level](CategoryId /*id*/, const Category& category)
   {
      res &= SetLevelImpl(category, level);
   });
   return res;
}

}
}


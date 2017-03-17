#pragma once

#include "tbp/log/ILogger.h"
#include "tbp/log/CategoryId.h"
#include "tbp/log/Level.h"
#include "tbp/log/Loggers.h"
#include "tbp/log/Categories.h"
#include "tbp/log/Injector.h"
#include <time.h>
#include <string>
#include <vector>
#include <atomic>
#include <cassert>

namespace tbp
{
namespace log
{

class Categories;
class Category;
class Config;

/*
- "theoretically" the same Logger could be used for different threads
- Logger::m_sink is in charge to handle the different logging policies
- when used through a ThreadLocalLogger, the Logger dtor will be called:
either by the shared_ptr dtor in ThreadLocalLogger 
or by the shared_ptr dtor in Loggers
*/
template <typename Sink>
class Logger : public ILogger
{
public:
   Logger(std::string name, const Loggers& loggers, Sink sink);
   //
   virtual const std::string& GetName() const override { return m_name; }
   Level GetLevel(CategoryId id) const { return m_categories[GetIndex(id)].GetLevel(); }
   virtual void SetLevel(CategoryId id, Level level) override { m_categories[GetIndex(id)].SetLevel(level); }
   template <typename... Args> void Log(CategoryId id, Level level, common::SigNum signal, const char* fmt, Args&&... args);
   bool ShouldLog(CategoryId id, Level level) const { return level >= GetLevel(id); }

private:
   struct CategoryData
   {
      CategoryData() : m_level(Level::none) {}
      //
      Level GetLevel() const { return m_level.load(std::memory_order_relaxed); }
      void SetLevel(Level val) { m_level.store(val, std::memory_order_relaxed); }
      //
      const Category* m_category = nullptr;
      std::atomic<Level> m_level; // can be changed by Loggers
   };
   //
   std::size_t GetIndex(CategoryId id) const;
   //
   Sink m_sink;
   std::vector<CategoryData> m_categories; // vector index is CategoryId - 1
   std::string m_name;

};

template <typename Sink>
Logger<Sink>::Logger(std::string name, const Loggers& loggers, Sink sink)
   : m_sink(std::move(sink)), m_categories(loggers.GetCategories().GetSize()), m_name(std::move(name))
{
   /*
   ATTENTION mock objects do not support copy/move
   so the expectations on the mock object must be set 
   after the Sink move ctor call above
   and not in ThreadLocalLogger just after the "standard" Sink ctor call 
   */
   loggers.GetInjector().OnNewLoggerSink(m_sink);
   //
   loggers.GetCategories().ForEach([this](CategoryId id, const Category& category)
   {
      std::size_t index = GetIndex(id);
      auto& data = m_categories[index];
      data.SetLevel(category.GetInitialLevel());
      data.m_category = &category;
   });
}

template <typename Sink>
inline std::size_t Logger<Sink>::GetIndex(CategoryId id) const
{
   assert(0 < id);
   std::size_t index = id - 1;
   assert(index < m_categories.size());
   return index;
}

template <typename Sink>
template <typename... Args> 
inline void /*TBP_NOINLINE*/ Logger<Sink>::Log(CategoryId id, Level level, common::SigNum signal, const char* fmt, Args&&... args)
{
   // first take the timestamp
   timespec now;
   ::clock_gettime(CLOCK_REALTIME, &now);
   //
   const Category& cat = *m_categories[GetIndex(id)].m_category;
   m_sink.Log(cat, level, now, signal, fmt, std::forward<Args>(args)...);
}

}
}


#pragma once

#include "tbp/log/Level.h"
#include "tbp/log/Logger.h"
#include "tbp/log/Loggers.h"
#include "tbp/log/Injector.h"
#include "tbp/common/ConfigurationException.h"
#include <memory>
#include <string>

namespace tbp
{
namespace log
{

/*
- a ThreadLocalLogger is in charge of cleaning the thread_local variable
- a ThreadLocalLogger must be used by exactly "one" thread
*/
template <typename Sink>
class ThreadLocalLogger
{
public:
   ThreadLocalLogger() = default;
   ThreadLocalLogger(std::shared_ptr<Loggers> loggers, std::string name, Sink sink);
   ~ThreadLocalLogger();
   ThreadLocalLogger(ThreadLocalLogger&&) = default;
   ThreadLocalLogger& operator=(ThreadLocalLogger&&) = default;
   //
   void Reset();
   static Logger<Sink>* Get() { return g_threadLocalLogger; }

private:
   static thread_local Logger<Sink>* g_threadLocalLogger;
   std::unique_ptr<Logger<Sink>> m_logger;
   std::shared_ptr<Loggers> m_loggers; // shared_ptr to ensure that m_loggers is valid until the last Logger is destroyed

};

template <typename Sink>
thread_local Logger<Sink>* ThreadLocalLogger<Sink>::g_threadLocalLogger = nullptr;

template <typename Sink>
ThreadLocalLogger<Sink>::ThreadLocalLogger(std::shared_ptr<Loggers> loggers, std::string name, Sink sink)
   : m_loggers(loggers)
{
   if (g_threadLocalLogger)
   {
      throw common::ConfigurationException("ThreadLocalLogger already created");
   }
   auto logger = std::make_unique<Logger<Sink>>(std::move(name), *m_loggers, std::move(sink));
   m_loggers->AddLogger(*logger);
   m_logger = std::move(logger);
   g_threadLocalLogger = m_logger.get();
}

template <typename Sink>
ThreadLocalLogger<Sink>::~ThreadLocalLogger()
{
   Reset();
}

template <typename Sink>
void ThreadLocalLogger<Sink>::Reset()
{
   if (m_logger)
   {
      g_threadLocalLogger = nullptr;
      m_loggers->RemoveLogger(*m_logger);
      m_loggers.reset();
      m_logger.reset();
   }
}

// use of a macro to avoid arguments evaluation if Logger::ShouldLog() is false
#define TBP_LOG(Sink, category, level, ...)                  \
   do                                                        \
   {                                                         \
      using LocalLogger = tbp::log::ThreadLocalLogger<Sink>; \
      auto logger = LocalLogger::Get();                      \
      if (logger->ShouldLog(category, level))                \
      {                                                      \
         logger->Log(category, level, 0, __VA_ARGS__);       \
      }                                                      \
   }                                                         \
   while (0)

}
}


#include "tbp/log/Categories.h"
#include "tbp/log/Level.h"
#include "tbp/log/ThreadLocalLogger.h"
#include "tbp/log/SyncSink.h"
#include "tbp/log/SignalManager.h"
#include "tbp/log/Injector.h"
#include "tbp/tools/Config.h"
#include "test/Context.h"
#include "test/Time.h"
#include <gtest/gtest.h>

#ifdef TBP_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace tbp
{
namespace log
{

namespace
{

CategoryId g_logCat1 = 0;
CategoryId g_logCat2 = 0;

#define LOG_SYNC(category, level, ...)             \
   TBP_LOG(SyncSink, category, level, __VA_ARGS__)

}

TEST(SyncLoggerPerfTest, Tbp)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   std::size_t nbIter = 10;
   log::Config logConfig(config.GetOutputDir(), "tbp");
   //
   Categories categories;
   g_logCat1 = categories.AddCategory(Category("category1", Level::info));
   g_logCat2 = categories.AddCategory(Category("category2", Level::info));
   Injector injector;
   auto loggers = std::make_shared<Loggers>(categories, logConfig, injector);
   SyncSink sink(loggers->GetConfig(), common::ThreadGetId(), nullptr);
   ThreadLocalLogger<SyncSink> threadLocalLogger(loggers, "tbpLogger", std::move(sink));
   //
   auto start = test::Now();
   for (std::size_t i = 0; i < nbIter; ++i)
   {
      LOG_SYNC(g_logCat1, Level::info, "msg {}", i);
      LOG_SYNC(g_logCat2, Level::info, "msg {}", i);
   }
   auto nanos = test::Now() - start;
   LOG_SYNC(g_logCat1, Level::info, "nanos per log call: {}", static_cast<double>(nanos.count()) / (2 * nbIter));
}

#ifdef TBP_SPDLOG
TEST(SyncLoggerPerfTest, SpdLog)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   auto logger = spdlog::daily_logger_st("SyncSpdLogger", config.GetOutputDir() + "/spdlog", 2, 30);
   std::size_t nbIter = 10;
   auto start = test::Now();
   for (std::size_t i = 0; i < nbIter; ++i)
   {
      logger->info("msg {}", i);
      logger->info("msg {}", i);
   }
   auto nanos = test::Now() - start;
   logger->info("nanos per log call: {}", static_cast<double>(nanos.count()) / (2 * nbIter));
}
#endif

}
}


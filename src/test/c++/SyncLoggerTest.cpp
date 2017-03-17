#include "tbp/log/Loggers.h"
#include "tbp/log/Categories.h"
#include "tbp/log/Config.h"
#include "tbp/log/SyncSink.h"
#include "tbp/log/ThreadLocalLogger.h"
#include "tbp/log/SignalManager.h"
#include "tbp/log/Injector.h"
#include "tbp/tools/Config.h"
#include "FileWriterHelper.h"
#include "test/Context.h"
#include "test/UserDefinedLoggable.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using tbp::log::test::UserDefinedLoggable;
using testing::Invoke;
using testing::_;
using testing::Return;
using std::string;
using std::make_shared;

namespace tbp
{
namespace log
{

namespace
{

log::Config BuildLogConfig(const tools::Config& config, const string& subDir)
{
   return log::Config(config.GetOutputDir() + "/SyncLoggerTest/" + subDir, "logfile");
}

Categories BuildCategories(CategoryId* id)
{
   Categories categories;
   *id = categories.AddCategory(Category("category1", Level::info));
   return categories;
}

class InjectorMock : public Injector
{
public:
   MOCK_CONST_METHOD1(OnNewLoggerSink, void(log::SyncSink& sink));
};

class SyncSinkMock : public SyncSink
{
public:
   SyncSinkMock(const Config& config, common::ThreadId tid, SignalManager* signals) : SyncSink(config, tid, signals) {}
   SyncSinkMock(SyncSinkMock&& rhs) : SyncSink(std::move(rhs)) {}
   SyncSinkMock& operator=(SyncSinkMock&& rhs) = delete;
   //
   MOCK_CONST_METHOD1(OnWrite, void(const fmt::MemoryWriter& writer));
   MOCK_METHOD1(ExitWithDefaultSignalHandler, void(common::SigNum signalNumber));
};

// let the user of the logger api define its own macros:
#define LOG_SYNC_MOCK(category, level, ...)            \
   TBP_LOG(SyncSinkMock, category, level, __VA_ARGS__)

void ConfigureInjector(const InjectorMock& injector, SyncSinkMock** sinkMock)
{
   auto onNewLoggerSink = [sinkMock](log::SyncSink& sink)
   {
      *sinkMock = dynamic_cast<SyncSinkMock*>(&sink);
   };
   EXPECT_CALL(injector, OnNewLoggerSink(_)).WillOnce(Invoke(onNewLoggerSink));
}

void ConfigureSinkExpectation(SyncSinkMock& sink, const string& expectedMsg)
{
   auto onWrite = [&sink, expectedMsg](const fmt::MemoryWriter& writer)
   {
      string logMsg = GetLogMsg(writer);
      EXPECT_EQ(logMsg, expectedMsg);
   };
   EXPECT_CALL(sink, OnWrite(_)).WillOnce(Invoke(onWrite));
}

// global variables to be used as argument of the log macro
CategoryId g_logCat1 = 0;
CategoryId g_logCat2 = 0;

class ComponentConfig
{
public:
   void AddCategories(Categories& categories)
   {
      Category cat1(m_category1, Level::info);
      g_logCat1 = categories.AddCategory(cat1);
      Category cat2(m_category2, Level::debug);
      g_logCat2 = categories.AddCategory(cat2);
   }
   const string& GetCategory1() const { return m_category1; }
   const string& GetCategory2() const { return m_category2; }

private:
   string m_category1 = "category1";
   string m_category2 = "category2";

};

}

TEST(SyncLoggerTest, Categories)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   log::Config logConfig = BuildLogConfig(config, "Categories");
   Categories allComponentsCategories;
   ComponentConfig componentConfig;
   componentConfig.AddCategories(allComponentsCategories);
   //
   InjectorMock injector;
   auto loggers = make_shared<Loggers>(allComponentsCategories, logConfig, injector);
   SyncSinkMock* sinkPtr = nullptr;
   ConfigureInjector(injector, &sinkPtr);
   SyncSinkMock sink(loggers->GetConfig(), common::ThreadGetId(), nullptr);
   ThreadLocalLogger<SyncSinkMock> threadLocalLogger(loggers, "testLogger", std::move(sink));
   //
   ConfigureSinkExpectation(*sinkPtr, "[info][category1] message category1");
   LOG_SYNC_MOCK(g_logCat1, Level::info, "message category{}", 1);
   ConfigureSinkExpectation(*sinkPtr, "[info][category2] message category2");
   LOG_SYNC_MOCK(g_logCat2, Level::info, "message category{}", 2);
   //
   threadLocalLogger.Reset(); // not mandatory: ThreadLocalLogger dtor does the same
}

TEST(SyncLoggerTest, FormatOnly)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   CategoryId catId = 0;
   InjectorMock injector;
   auto loggers = make_shared<Loggers>(BuildCategories(&catId), BuildLogConfig(config, "UserDefinedLoggable"), injector);
   SyncSinkMock* sinkPtr = nullptr;
   ConfigureInjector(injector, &sinkPtr);
   SyncSinkMock sink(loggers->GetConfig(), common::ThreadGetId(), nullptr);
   ThreadLocalLogger<SyncSinkMock> threadLocalLogger(loggers, "testLogger", std::move(sink));
   //
   ConfigureSinkExpectation(*sinkPtr, "[info][category1] message with format only");
   LOG_SYNC_MOCK(catId, Level::info, "message with format only");
}

TEST(SyncLoggerTest, UserDefinedLoggable)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   CategoryId catId = 0;
   InjectorMock injector;
   auto loggers = make_shared<Loggers>(BuildCategories(&catId), BuildLogConfig(config, "UserDefinedLoggable"), injector);
   SyncSinkMock* sinkPtr = nullptr;
   ConfigureInjector(injector, &sinkPtr);
   SyncSinkMock sink(loggers->GetConfig(), common::ThreadGetId(), nullptr);
   ThreadLocalLogger<SyncSinkMock> threadLocalLogger(loggers, "testLogger", std::move(sink));
   //
   ConfigureSinkExpectation(*sinkPtr, "[info][category1] message number 2012-12-9");
   LOG_SYNC_MOCK(catId, Level::info, "message number {}", UserDefinedLoggable(2012, 12, 9));
}

TEST(SyncLoggerTest, Level)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   Categories categories;
   Category category("category1", Level::warn);
   CategoryId catId = categories.AddCategory(category);
   InjectorMock injector;
   auto loggers = make_shared<Loggers>(categories, BuildLogConfig(config, "Level"), injector);
   SyncSinkMock* sinkPtr = nullptr;
   ConfigureInjector(injector, &sinkPtr);
   SyncSinkMock sink(loggers->GetConfig(), common::ThreadGetId(), nullptr);
   ThreadLocalLogger<SyncSinkMock> threadLocalLogger(loggers, "testLogger", std::move(sink));
   //
   ConfigureSinkExpectation(*sinkPtr, "[warn][category1] msg1");
   LOG_SYNC_MOCK(catId, Level::warn, "msg{}", 1);
   LOG_SYNC_MOCK(catId, Level::info, "msg{}", 2);
   //
   loggers->SetLevel(category, Level::info);
   ConfigureSinkExpectation(*sinkPtr, "[info][category1] msg3");
   LOG_SYNC_MOCK(catId, Level::info, "msg{}", 3);
   LOG_SYNC_MOCK(catId, Level::debug, "msg{}", 4);
   //
   loggers->SetLevels(Level::error);
   ConfigureSinkExpectation(*sinkPtr, "[error][category1] msg5");
   LOG_SYNC_MOCK(catId, Level::error, "msg{}", 5);
   LOG_SYNC_MOCK(catId, Level::info, "msg{}", 6);
}

void TBP_NOINLINE SyncFunc2(common::SigNum signal)
{
   raise(signal);
}

void TBP_NOINLINE SyncFunc1(common::SigNum signal)
{
   SyncFunc2(signal);
}


TEST(SyncLoggerTest, FatalError)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   //
   CategoryId catId = 0;
   InjectorMock injector;
   SyncSinkMock* sinkPtr = nullptr;
   ConfigureInjector(injector, &sinkPtr);
   auto loggers = make_shared<Loggers>(BuildCategories(&catId), BuildLogConfig(config, "UserDefinedLoggable"), injector);
   SignalManager signals;
   signals.InstallSignalHandler<SyncSinkMock>(catId);
   SyncSinkMock sink(loggers->GetConfig(), common::ThreadGetId(), &signals);
   ThreadLocalLogger<SyncSinkMock> threadLocalLogger(loggers, "testLogger", std::move(sink));
   auto exitWithDefaultSignalHandler = [&signals](common::SigNum signalNumber)
   {
      bool doNotKill = true; // only for tests
      signals.ExitWithDefaultSignalHandler(signalNumber, doNotKill);
   };
   EXPECT_CALL(*sinkPtr, ExitWithDefaultSignalHandler(_)).WillOnce(Invoke(exitWithDefaultSignalHandler));
   EXPECT_CALL(*sinkPtr, OnWrite(_)).WillRepeatedly(Return());
   //
   LOG_SYNC_MOCK(catId, Level::info, "message {}", 1);
   SyncFunc1(SIGSEGV);
}

}
}


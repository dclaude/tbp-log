#include "tbp/log/Encoder.h"
#include "tbp/log/AsyncSink.h"
#include "tbp/log/Loggers.h"
#include "tbp/log/Categories.h"
#include "tbp/log/Config.h"
#include "tbp/log/ThreadLocalLogger.h"
#include "tbp/log/DefaultTypes.h"
#include "tbp/log/Formatter.h"
#include "tbp/log/AsyncLogger.h"
#include "tbp/log/FileWriter.h"
#include "tbp/log/BufferAllocator.h"
#include "tbp/log/SignalManager.h"
#include "tbp/log/Injector.h"
#include "tbp/common/Compiler.h"
#include "tbp/tools/ScopedThread.h"
#include "tbp/tools/OS.h"
#include "tbp/tools/spsc/Queue1.h"
#include "tbp/tools/mpsc/Queue1.h"
#include "tbp/tools/Config.h"
#include "tbp/tools/AffinityManager.h"
#include "FileWriterHelper.h"
#include "test/Context.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <memory>
#include <thread>

using testing::_;
using testing::InSequence;
using testing::Invoke;
using std::make_shared;
using std::string;

namespace tbp
{
namespace log
{

namespace
{

CategoryId g_logCat1 = 0;
using Allocator = BufferAllocator<tools::spsc::Queue1<char*>>;
using MyQueue = tools::spsc::Queue1<Msg<Allocator>>;
using MySink = AsyncSink<DefaultTypeId, MyQueue, tools::mpsc::Queue1, Allocator>;
using MyLogger = AsyncLogger<DefaultTypeId, MyQueue, tools::mpsc::Queue1, Allocator>;

class InjectorMock : public Injector
{
public:
   MOCK_CONST_METHOD2(CreateFileWriter, std::unique_ptr<log::FileWriter>(const log::Config& config, common::ThreadId tid));
};

class FileWriterMock1 : public FileWriter
{
public:
   FileWriterMock1(const Config& config, common::ThreadId tid) : FileWriter(config, tid) {}
   //
   MOCK_CONST_METHOD1(OnWrite, void(const fmt::MemoryWriter& writer));
};

class FileWriterMock2 : public FileWriter
{
public:
   FileWriterMock2(const Config& config, common::ThreadId tid) : FileWriter(config, tid) {}
   //
   MOCK_CONST_METHOD1(OnFileWritten, void(std::ofstream& file));
};

}

TEST(AsyncLoggerTest, Formatter)
{
   const char* fmt = "[{:0>2}:{:0>2}:{:0>2}.{:0>6}][{}][{}] ";
   //
   string str1;
   string str2;
   //
   {
      fmt::MemoryWriter writer;
      writer.write(fmt, 12, 59, 31, 123456789, "debug", "cat1");
      str1.assign(writer.data(), writer.size());
   }
   //
   {
      fmt::MemoryWriter writer;
      int index = 1;
      auto func = [&index](const char* fmt, fmt::MemoryWriter& writer)
      {
         switch (index)
         {
            case 1:
               writer.write(fmt, 12);
               break;
            case 2:
               writer.write(fmt, 59);
               break;
            case 3:
               writer.write(fmt, 31);
               break;
            case 4:
               writer.write(fmt, 123456789);
               break;
            case 5:
               writer.write(fmt, "debug");
               break;
            case 6:
               writer.write(fmt, "cat1");
               break;
         }
         ++index;
      };
      //
      Formatter formatter;
      formatter.Format(fmt, writer, func);
      str2.assign(writer.data(), writer.size());
      //
      EXPECT_EQ(str1, str2);
   }
}

// let the user of the logger api define its own macros:
#define LOG_ASYNC(category, level, ...)          \
   TBP_LOG(MySink, category, level, __VA_ARGS__)

TEST(AsyncLoggerTest, AsyncSink)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   log::Config logConfig(config.GetOutputDir() + "/AsyncLoggerTest_AsyncSink", "logfile");
   Categories categories;
   Category cat1("category1", Level::info);
   g_logCat1 = categories.AddCategory(cat1);
   InjectorMock injector;
   auto createFileWriter = [](const log::Config& config, common::ThreadId tid)
   {
      auto fw = std::make_unique<FileWriterMock1>(config, tid);
      //
      {
         InSequence s;
         EXPECT_CALL(*fw, OnWrite(_)).WillOnce(Invoke(
         [](const fmt::MemoryWriter& writer)
         {
            string logMsg = GetLogMsg(writer);
            EXPECT_EQ(logMsg, "[info][category1] withFormat 1");
         }));
         EXPECT_CALL(*fw, OnWrite(_)).WillOnce(Invoke(
         [](const fmt::MemoryWriter& writer)
         {
            string logMsg = GetLogMsg(writer);
            EXPECT_EQ(logMsg, "[info][category1] withoutFormat");
         }));
      }
      return fw;
   };
   EXPECT_CALL(injector, CreateFileWriter(_, _)).WillOnce(Invoke(createFileWriter));
   auto loggers = make_shared<Loggers>(categories, logConfig, injector);
   //
   MyLogger asyncLogger(logConfig, injector);
   std::atomic<bool> done(false);
   auto affinity = affinities.GetSlowCpu();
   tools::ScopedThread loggerThread(std::thread([&asyncLogger, &done, affinity]
   {
      tools::ThreadSetAffinity(affinity);
      bool stop = false;
      //
      while (!stop)
      {
         stop = done.load();
         asyncLogger.LogMessages();
      }
   }));
   MySink sink(asyncLogger, std::make_unique<MyQueue>(), 
         std::make_unique<Allocator>(Allocator::BufferSizes({ 64, 128, 256, 1024 }), 10), common::ThreadGetId());
   ThreadLocalLogger<MySink> threadLocalLogger(loggers, "testLogger", std::move(sink));
   LOG_ASYNC(g_logCat1, Level::info, "withFormat {}", 1);
   LOG_ASYNC(g_logCat1, Level::info, "withoutFormat");
   //
   done.store(true);
   // join loggerThread, queue is destroyed after the loggerThread
}

void TBP_NOINLINE AsyncFunc2(common::SigNum signal)
{
   raise(signal);
}

void TBP_NOINLINE AsyncFunc1(common::SigNum signal)
{
   AsyncFunc2(signal);
}

TEST(AsyncLoggerTest, FatalError)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   log::Config logConfig(config.GetOutputDir() + "/AsyncLoggerTest_FatalError", "logfile");
   Categories categories;
   Category cat1("category1", Level::info);
   g_logCat1 = categories.AddCategory(cat1);
   InjectorMock injector;
   std::atomic<bool> flushed(false);
   auto createFileWriter = [&flushed](const log::Config& config, common::ThreadId tid)
   {
      auto fw = std::make_unique<FileWriterMock2>(config, tid);
      auto onFileWritten = [&flushed](std::ofstream& file)
      {
         file.flush();
         flushed.store(true);
      };
      EXPECT_CALL(*fw, OnFileWritten(_)).WillRepeatedly(Invoke(onFileWritten));
      return fw;
   };
   EXPECT_CALL(injector, CreateFileWriter(_, _)).WillOnce(Invoke(createFileWriter));
   auto loggers = make_shared<Loggers>(categories, logConfig, injector);
   //
   SignalManager signals;
   signals.InstallSignalHandler<MySink>(g_logCat1);
   //
   MyLogger asyncLogger(logConfig, injector);
   std::atomic<bool> done(false);
   auto affinity = affinities.GetSlowCpu();
   tools::ScopedThread loggerThread(std::thread([&asyncLogger, &done, &signals, affinity]
   {
      tools::ThreadSetAffinity(affinity);
      bool stop = false;
      //
      while (!stop)
      {
         stop = done.load();
         common::SigNum signalNumber = asyncLogger.LogMessages();
         if (unlikely(signalNumber))
         {
            bool doNotKill = true; // only for tests
            signals.ExitWithDefaultSignalHandler(signalNumber, doNotKill);
         }
      }
   }));
   MySink sink(asyncLogger, std::make_unique<MyQueue>(), 
         std::make_unique<Allocator>(Allocator::BufferSizes({ 64 }), 10), common::ThreadGetId());
   ThreadLocalLogger<MySink> threadLocalLogger(loggers, "testLogger", std::move(sink));
   //
   LOG_ASYNC(g_logCat1, Level::info, "message {}", 1);
   while (!flushed.load()) {}
   LOG_ASYNC(g_logCat1, Level::info, "message {}", 2);
   AsyncFunc1(SIGSEGV);
   //
   done.store(true);
   // join loggerThread, queue is destroyed after the loggerThread
}

}
}


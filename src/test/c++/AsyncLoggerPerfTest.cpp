#include "tbp/log/Categories.h"
#include "tbp/log/Level.h"
#include "tbp/log/ThreadLocalLogger.h"
#include "tbp/log/AsyncSink.h"
#include "tbp/log/AsyncLogger.h"
#include "tbp/log/DefaultTypes.h"
#include "tbp/log/BufferAllocator.h"
#include "tbp/log/Injector.h"
#include "tbp/tools/ScopedThread.h"
#include "tbp/tools/spsc/Queue1.h"
#include "tbp/tools/spsc/Queue2.h"
#include "tbp/tools/mpsc/Queue1.h"
#include "tbp/tools/OS.h"
#include "tbp/tools/Config.h"
#include "tbp/tools/AffinityManager.h"
#include "tbp/common/Compiler.h"
#include "test/Context.h"
#include "test/Time.h"
#include <gtest/gtest.h>
#include <thread>
#include <future>
#include <math.h>

#ifdef TBP_SPDLOG
#include <spdlog/spdlog.h>
#endif


namespace tbp
{
namespace log
{

namespace
{

class Allocator1
{
public:
   using Handle = char*;
   //
   Handle Alloc(std::size_t size) { return static_cast<Handle>(malloc(size)); }
   void Free(Handle& h) { free(h); }
};

using Allocator2 = BufferAllocator<tools::spsc::Queue1<char*>>;

CategoryId g_logCat1 = 0;
CategoryId g_logCat2 = 0;

std::size_t GetNbQueueItems(std::size_t nbIter)
{
   /*
   IMPORTANT
   the queue of log::Msg and the queue of char* are prepared/filled with enough elements to avoid any allocation
   it is needed because the log producer thread is much faster than the log consumer thread 
   so the queues items are quickly exhausted in this example if nbIter is big
   in real life a small amount of items prepared/filled is enough because the producer won't generally be much faster than the consumer
   */
   std::size_t nbQueueItems = 2 * nbIter + 1;
   return nbQueueItems;
}

std::size_t GetNbProducers()
{
   return std::max(tools::HardwareConcurrency() - 1, 1UL);
}

}

class AsyncLoggerPerfTest : public testing::Test
{
public:
   template <typename SpscQueue, typename Allocator>
   void Start(const tools::Config& config, const tools::AffinityManager& affinities,
         std::size_t nbIter, std::function<std::unique_ptr<SpscQueue>()> createQueue, std::function<std::unique_ptr<Allocator>()> createAllocator)
   {
      using MySink = AsyncSink<DefaultTypeId, SpscQueue, tools::mpsc::Queue1, Allocator>;
      using MyLogger = AsyncLogger<DefaultTypeId, SpscQueue, tools::mpsc::Queue1, Allocator>;
      #define LOG_ASYNC(category, level, ...)          \
         TBP_LOG(MySink, category, level, __VA_ARGS__)
      //
      log::Config logConfig(config.GetOutputDir(), "tbp");
      //
      Categories categories;
      g_logCat1 = categories.AddCategory(Category("category1", Level::info));
      g_logCat2 = categories.AddCategory(Category("category2", Level::info));
      Injector injector;
      auto loggers = std::make_shared<Loggers>(categories, logConfig, injector);
      //
      std::size_t nbProducers = GetNbProducers();
      MyLogger asyncLogger(logConfig, injector);
      std::promise<void> go;
      std::shared_future<void> ready(go.get_future());
      std::vector<std::promise<void>> producersReady(nbProducers);
      std::promise<void> consumerReady;
      std::vector<tools::ScopedThread> producers;
      std::atomic<std::size_t> nbStoppedProducers(0);
      auto cpuIds = affinities.MakeFastCpus(nbProducers);
      auto cpuIter = cpuIds.begin();
      for (std::size_t producerIndex = 0; producerIndex < nbProducers; ++producerIndex)
      {
         auto& producerReady = producersReady[producerIndex];
         auto affinity = *cpuIter++;
         producers.emplace_back(std::thread([affinity, &producerReady, ready, &nbStoppedProducers, producerIndex, nbIter, &asyncLogger, createQueue, createAllocator, &loggers]
         {
            tools::ThreadSetAffinity(affinity);
            MySink sink(asyncLogger, std::move(createQueue()), std::move(createAllocator()), common::ThreadGetId());
            ThreadLocalLogger<MySink> threadLocalLogger(loggers, "tbpLogger_" + std::to_string(producerIndex), std::move(sink));
            producerReady.set_value();
            ready.wait();
            //
            auto start = test::Now();
            for (std::size_t i = 0; i < nbIter; ++i)
            {
               LOG_ASYNC(g_logCat1, Level::info, "msg {}", i);
               LOG_ASYNC(g_logCat2, Level::info, "msg {} {}", i, i / 3.);
            }
            auto nanos = test::Now() - start;
            LOG_ASYNC(g_logCat1, Level::info, "producer: {}, nanos per log call: {}", producerIndex, static_cast<double>(nanos.count()) / (2 * nbIter));
            nbStoppedProducers.fetch_add(1);
         }));
      }
      auto loggerAffinity = affinities.GetSlowCpu();
      tools::ScopedThread consumerThread(std::thread([loggerAffinity, &consumerReady, ready, &asyncLogger, &nbStoppedProducers, nbProducers]
      {
         tools::ThreadSetAffinity(loggerAffinity);
         consumerReady.set_value();
         ready.wait();
         //
         bool stop = false;
         std::uint64_t count = 0;
         while (!stop)
         {
            ++count;
            if (unlikely(count > 10000))
            {
               std::size_t stoppedProducers = nbStoppedProducers.load();
               if (stoppedProducers == nbProducers)
               {
                  stop = true;
               }
            }
            asyncLogger.LogMessages();
         }
      }));
      //
      consumerReady.get_future().wait();
      for (auto& producerReady : producersReady)
      {
         producerReady.get_future().wait();
      }
      go.set_value();
      //
      // join consumerThread, queue is destroyed after the consumerThread
      #undef LOG_ASYNC
   }

};

TEST_F(AsyncLoggerPerfTest, TbpNoAllocator)
{
   const auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   using MyQueue = tools::spsc::Queue1<Msg<Allocator1>>;
   std::size_t nbIter = 10;
   //
   auto createQueue = [nbIter]()
   {
      std::size_t nbQueueItems = GetNbQueueItems(nbIter);
      auto queue = std::make_unique<MyQueue>();
      queue->Reserve(nbQueueItems);
      return queue;
   };
   auto createAllocator = []()
   {
      return std::make_unique<Allocator1>();
   };
   Start<MyQueue, Allocator1>(config, affinities, nbIter, createQueue, createAllocator);
}

TEST_F(AsyncLoggerPerfTest, BoundedQueue)
{
   const auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   using MyQueue = tools::spsc::Queue2<Msg<Allocator2>>;
   std::size_t nbIter = 10;
   //
   std::size_t nbQueueItems = GetNbQueueItems(nbIter);
   auto createQueue = [nbQueueItems]()
   {
      int exponent = static_cast<int>(log2(nbQueueItems)) + 1;
      size_t size = std::pow(2, exponent); // queue size must be power of 2
      return std::make_unique<MyQueue>(size);
   };
   auto createAllocator = [nbQueueItems]()
   {
      return std::make_unique<Allocator2>(
         Allocator2::BufferSizes({ 64 }),
         nbQueueItems);
   };
   Start<MyQueue, Allocator2>(config, affinities, nbIter, createQueue, createAllocator);
}

TEST_F(AsyncLoggerPerfTest, Tbp)
{
   const auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   using MyQueue = tools::spsc::Queue1<Msg<Allocator2>>;
   std::size_t nbIter = 10;
   //
   std::size_t nbQueueItems = GetNbQueueItems(nbIter);
   auto createQueue = [nbQueueItems]()
   {
      auto queue = std::make_unique<MyQueue>();
      queue->Reserve(nbQueueItems);
      return queue;
   };
   auto createAllocator = [nbQueueItems]()
   {
      return std::make_unique<Allocator2>(
         Allocator2::BufferSizes({ 64 }), // our test log messages is less than 64 bytes once encoded
         nbQueueItems);
   };
   /*
   comparison with SpdLog
   IMPORTANT
   - with SpdLog the google test total time is much smaller than with tbp AsyncLogger
   when we build the SpdLog FlameGraph, we can see that the load is almost equally dispatched among the producers and the consumer thread
   so with SpdLog the test uses all the available cores during the complete test duration
   - whereas with tbp the producer threads do not do much work, which is what we want
   (all the work of the producers is off-loaded to the single consumer thread)
   and then the consumer thread (AsyncLogger) does all the work which takes much more time to finish
   - and if we run a 'perf stat' we can see that the sum of the cycles spent on all the cores is almost 2 times lower with tbp than with SpdLog
   */
   Start<MyQueue, Allocator2>(config, affinities, nbIter, createQueue, createAllocator);
}

#ifdef TBP_SPDLOG
TEST_F(AsyncLoggerPerfTest, SpdLog)
{
   auto& context = test::Context::Get();
   const auto& config = context.GetToolsConfig();
   const auto& affinities = context.GetAffinityManager();
   //
   std::size_t nbIter = 10;
   std::size_t nbQueueItems = GetNbQueueItems(nbIter);
   int exponent = static_cast<int>(log2(nbQueueItems)) + 1;
   size_t size = std::pow(2, exponent); // queue size must be power of 2
   spdlog::set_async_mode(size);
   auto logger = spdlog::daily_logger_st("AsyncSpdLogger", config.GetOutputDir() + "/spdlog", 2, 30);
   //
   std::size_t nbProducers = GetNbProducers();
   std::promise<void> go;
   std::shared_future<void> ready(go.get_future());
   std::vector<std::promise<void>> producersReady(nbProducers);
   std::vector<tools::ScopedThread> producers;
   std::atomic<std::size_t> nbStoppedProducers(0);
   auto cpuIds = affinities.MakeFastCpus(nbProducers);
   auto cpuIter = cpuIds.begin();
   for (std::size_t producerIndex = 0; producerIndex < nbProducers; ++producerIndex)
   {
      auto& producerReady = producersReady[producerIndex];
      auto affinity = *cpuIter++;
      producers.emplace_back(std::thread([affinity, &producerReady, ready, &nbStoppedProducers, producerIndex, nbIter, logger, size]
      {
         tools::ThreadSetAffinity(affinity);
         producerReady.set_value();
         ready.wait();
         //
         auto start = test::Now();
         for (std::size_t i = 0; i < nbIter; ++i)
         {
            logger->info("msg {}", i);
            logger->info("msg {} {}", i, i / 3.);
         }
         auto nanos = test::Now() - start;
         logger->info("producer {}, nanos per log call: {} size {}", producerIndex, static_cast<double>(nanos.count()) / (2 * nbIter), size);
         nbStoppedProducers.fetch_add(1);
      }));
   }
   for (auto& producerReady : producersReady)
   {
      producerReady.get_future().wait();
   }
   go.set_value();
}
#endif

}
}


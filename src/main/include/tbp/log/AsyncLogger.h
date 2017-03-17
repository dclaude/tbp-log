#pragma once

#include "tbp/log/AsyncLogger.fwd.h"
#include "tbp/log/Msg.h"
#include "tbp/log/Formatter.h"
#include "tbp/log/FileWriter.h"
#include "tbp/log/Decoder.h"
#include "tbp/log/DefaultTypes.h"
#include "tbp/log/Injector.h"
#include "tbp/log/ActionVariant.h"
#include "tbp/common/Compiler.h"
#include "tbp/common/OS.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace tbp
{
   namespace log
   {
      class Config;
      class FileWriter;
      class Injector;
   }
}

namespace tbp
{
namespace log
{

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
class AsyncLogger
{
public:
   using AddMsg = AsyncLoggerAddMsg<SpscQueue, Allocator>;
   using RemoveMsg = AsyncLoggerRemoveMsg<SpscQueue>;
   //
   AsyncLogger(const Config& config, const Injector& injector) : m_injector(injector), m_config(config) {}
   ~AsyncLogger() {}
   //
   common::SigNum LogMessages();
   void AddQueue(AddMsg msg);
   void RemoveQueue(RemoveMsg msg);
   //
   void OnAddQueue(AddMsg& msg);
   void OnRemoveQueue(const RemoveMsg& msg);

private:
   struct QueueData
   {
      std::unique_ptr<Allocator> m_allocator;
      std::unique_ptr<SpscQueue> m_queue;
      common::ThreadId m_tid = 0;
      /*
      - it is not straightforward to order log messages (Msg) by timestamp (Msg::m_time)
      anything can happen between the point where the timestamp is taken and the call to Enqueue()
      if only one log file is used, a "newer" Msg can be logged even before an "older" Msg is enqueued
      - currently each queue writes in a different file, the user has to merge/sort them if needed
      */
      std::unique_ptr<FileWriter> m_fileWriter;
   };
   using Action = ActionVariant<SpscQueue, Allocator>;
   struct Node : public MpscQueue::Node
   {
      Action m_msg;
   };
   //
   std::vector<QueueData> m_queues;
   Formatter m_formatter;
   const Injector& m_injector;
   const Config& m_config;
   MpscQueue m_actions;
   std::vector<RemoveMsg> m_toRemove;

};

namespace details
{

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
class Visitor
{
public:
   using Logger = AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>;
   Visitor(Logger& logger) : m_logger(logger) {}
   //
   void operator()(typename Logger::AddMsg& msg)
   {
      m_logger.OnAddQueue(msg);
   }
   void operator()(const typename Logger::RemoveMsg& msg)
   {
      m_logger.OnRemoveQueue(msg);
   }

private:
   Logger& m_logger;

};

}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline common::SigNum AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>::LogMessages()
{
   common::SigNum signal = 0;
   while (Node* node = static_cast<Node*>(m_actions.Dequeue()))
   {
      details::Visitor<TypeId, SpscQueue, MpscQueue, Allocator> visitor(*this);
      node->m_msg.ApplyVisitor(visitor);
      //
      delete node;
   }
   //
   for (auto& data : m_queues)
   {
      auto& queue = *data.m_queue.get();
      auto& allocator = *data.m_allocator.get();
      auto& fileWriter = *data.m_fileWriter.get();
      auto& writer = fileWriter.GetWriter();
      Msg<Allocator> msg;
      while (queue.Dequeue(msg))
      {
         common::SigNum sig = msg.GetSignal();
         if (unlikely(sig && !signal))
         {
            signal = sig;
         }
         fileWriter.WriteHeader(msg.GetTime(), data.m_tid, msg.GetLevel(), msg.GetCategory());
         const auto& msgBuffer = msg.GetBuffer();
         if (msgBuffer.Get())
         {
            Decoder<TypeId, Allocator> decoder(msg.GetBuffer());
            m_formatter.Format(msg.GetFormat(), writer, [&decoder](const char* fmt, fmt::MemoryWriter& writer)
            {
               assert(decoder.HasNext() == true);
               auto p = decoder.Next();
               TypeId typeId = p.first;
               auto& buffer = *p.second;
               switch (typeId)
               {
                  case TypeId::INT:
                  {
                     int v = 0;
                     Type<int, TypeId, Allocator>::Decode(buffer, v);
                     writer.write(fmt, v);
                  }
                  break;
                  case TypeId::UINT64:
                  {
                     std::uint64_t v = 0;
                     Type<std::uint64_t, TypeId, Allocator>::Decode(buffer, v);
                     writer.write(fmt, v);
                  }
                  break;
                  case TypeId::INT64:
                  {
                     std::int64_t v = 0;
                     Type<std::int64_t, TypeId, Allocator>::Decode(buffer, v);
                     writer.write(fmt, v);
                  }
                  break;
                  case TypeId::DOUBLE:
                  {
                     double v = 0.;
                     Type<double, TypeId, Allocator>::Decode(buffer, v);
                     writer.write(fmt, v);
                  }
                  break;
                  case TypeId::STRING:
                  {
                     std::string v;
                     Type<std::string, TypeId, Allocator>::Decode(buffer, v);
                     writer.write(fmt, v);
                  }
                  break;
                  case TypeId::NONE:
                  {
                     assert(false);
                  }
                  break;
               }
            });
            assert(decoder.HasNext() == false);
         }
         else
         {
            writer.write(msg.GetFormat());
         }
         fileWriter.WriteToFile();
         //
         msg.Recycle(allocator);
      }
      fileWriter.Flush();
   }
   //
   if (unlikely(!m_toRemove.empty()))
   {
      for (auto& msg : m_toRemove)
      {
         auto end = m_queues.end();
         auto first = std::remove_if(m_queues.begin(), end, [&msg](const QueueData& data)
         {
            return data.m_queue.get() == msg.m_queue;
         });
         assert(std::distance(first, end) == 1U);
         m_queues.erase(first, end);
      }
      m_toRemove.clear();
   }
   return signal;
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline void AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>::AddQueue(AddMsg msg)
{
   auto node = new Node;
   node->m_msg.Set(std::move(msg));
   m_actions.Enqueue(node);
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline void AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>::RemoveQueue(RemoveMsg msg)
{
   auto node = new Node;
   node->m_msg.Set(std::move(msg));
   m_actions.Enqueue(node);
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline void AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>::OnAddQueue(AddMsg& msg)
{
   QueueData data;
   data.m_queue = std::move(msg.m_queue);
   data.m_tid = msg.m_tid;
   data.m_fileWriter = m_injector.CreateFileWriter(m_config, msg.m_tid);
   data.m_allocator = std::move(msg.m_allocator);
   m_queues.emplace_back(std::move(data));
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline void AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>::OnRemoveQueue(const RemoveMsg& msg)
{
   // only remove from m_queues once all the log messages have been dequeued
   m_toRemove.emplace_back(msg);
}

}
}


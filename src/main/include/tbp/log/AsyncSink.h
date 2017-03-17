#pragma once

#include "tbp/log/Category.h"
#include "tbp/log/Level.h"
#include "tbp/log/Encoder.h"
#include "tbp/log/Msg.h"
#include "tbp/log/AsyncLogger.h"
#include "tbp/common/OS.h"
#include <time.h>
#include <memory>

namespace tbp
{
namespace log
{

class Config;

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
class AsyncSink
{
public:
   using Logger = AsyncLogger<TypeId, SpscQueue, MpscQueue, Allocator>;
   //
   AsyncSink(Logger& asyncLogger, std::unique_ptr<SpscQueue> queue, std::unique_ptr<Allocator> allocator, common::ThreadId tid);
   ~AsyncSink();
   AsyncSink(AsyncSink&& rhs);
   AsyncSink& operator=(AsyncSink&& rhs) = delete;
   //
   /*
   IMPORTANT: 
   the 'fmt' argument is not "copied" in the log::Buffer
   it is meant to be used with a string literal 
   but can also be a std::string as long as it is const and remains available in memory until the AsynLogger is done with this log line
   */
   template <typename... Args> void Log(const Category& category, Level level, const timespec& now, common::SigNum signal, const char* fmt, Args&&... args);

private:
   SpscQueue* m_queue = nullptr;
   Logger& m_asyncLogger;
   Encoder<TypeId, Allocator> m_encoder;
   Allocator* m_allocator = nullptr;

};

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline AsyncSink<TypeId, SpscQueue, MpscQueue, Allocator>::AsyncSink(Logger& asyncLogger, std::unique_ptr<SpscQueue> queue, std::unique_ptr<Allocator> allocator, common::ThreadId tid)
   : m_asyncLogger(asyncLogger)
{
   typename Logger::AddMsg msg;
   msg.m_queue = std::move(queue);
   msg.m_tid = tid;
   msg.m_allocator = std::move(allocator);
   //
   m_queue = msg.m_queue.get();
   m_allocator = msg.m_allocator.get();
   m_asyncLogger.AddQueue(std::move(msg));
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline AsyncSink<TypeId, SpscQueue, MpscQueue, Allocator>::~AsyncSink()
{
   if (m_queue)
   {
      typename Logger::RemoveMsg msg;
      msg.m_queue = m_queue;
      m_asyncLogger.RemoveQueue(std::move(msg));
   }
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
inline AsyncSink<TypeId, SpscQueue, MpscQueue, Allocator>::AsyncSink(AsyncSink&& rhs)
   : m_queue(rhs.m_queue), m_asyncLogger(rhs.m_asyncLogger), m_encoder(std::move(rhs.m_encoder)), m_allocator(rhs.m_allocator)
{
   rhs.m_queue = nullptr; // IMPORTANT: to call AsyncLogger::RemoveQueue() only once
}

template <typename TypeId, typename SpscQueue, typename MpscQueue, typename Allocator>
template <typename... Args> 
inline void /*TBP_NOINLINE*/ AsyncSink<TypeId, SpscQueue, MpscQueue, Allocator>::Log(const Category& category, Level level, const timespec& now, common::SigNum signal,
      const char* fmt, // cf comment before the method declaration
      Args&&... args)
{
   Buffer<Allocator> buffer = m_encoder.Encode(*m_allocator, std::forward<Args>(args)...);
   buffer.Reset();
   Msg<Allocator> msg(now, level, category, fmt, std::move(buffer), signal);
   while (!m_queue->Enqueue(std::move(msg))) {}
}

}
}


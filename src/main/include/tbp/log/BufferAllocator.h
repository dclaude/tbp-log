#pragma once

#include "tbp/log/BufferHandle.h"
#include "tbp/common/ConfigurationException.h"
#include "tbp/common/CpuCache.h"
#include <vector>
#include <set>
#include <stdlib.h>
#include <cassert>

namespace tbp
{
namespace log
{

template <typename SpscQueue>
class BufferAllocator
{
public:
   using Handle = BufferHandle<SpscQueue>;
   using BufferSizes = std::set<std::size_t>;
   //
   BufferAllocator(const BufferSizes& bufferSizes, std::size_t nbItemsPerQueue);
   ~BufferAllocator();
   //
   Handle Alloc(std::size_t size);
   void Free(Handle& h);

private:
   struct QueueData
   {
      SpscQueue m_queue;
      std::size_t m_size = 0;
   };
   //
   char* AlignedAlloc(std::size_t size);
   char* AllocSlow(std::size_t size);
   //
   std::vector<QueueData> m_queues;

};

template <typename SpscQueue>
inline typename BufferAllocator<SpscQueue>::Handle /*TBP_NOINLINE*/ BufferAllocator<SpscQueue>::Alloc(std::size_t size)
{
   for (auto& data : m_queues)
   {
      if (size <= data.m_size)
      {
         char* buffer = nullptr;
         if (data.m_queue.Dequeue(buffer))
         {
            return Handle(buffer, &data.m_queue);
         }
         /*
         if the queue is empty:
         - we can either allocate a new "aligned" buffer to be enqueued later in this queue
         this strategy only works if the queue used is unbounded
         otherwise the AsyncLogger could be not able to Enqueue this new buffer and it could lead to a deadlock
         this can be done by uncommenting the line below
         - or we can allocate a new "custom buffer" which will not be recycled in the queues
         but will be freed by the AsyncLogger
         this is what is currenly done by the last line of this function
         */
         //return Handle(AlignedAlloc(data.m_size)), queue);
      }
   }
   return Handle(AllocSlow(size), nullptr);
}

template <typename SpscQueue>
inline void BufferAllocator<SpscQueue>::Free(Handle& h)
{
   if (h.m_buffer)
   {
      if (h.m_queue)
      {
         while (!h.m_queue->Enqueue(h.m_buffer)) {}
      }
      else
      {
         free(h.m_buffer);
      }
      h.m_buffer = nullptr;
   }
}

template <typename SpscQueue>
BufferAllocator<SpscQueue>::BufferAllocator(const BufferSizes& bufferSizes, std::size_t nbItemsPerQueue) : m_queues(bufferSizes.size())
{
   std::size_t i = 0;
   for (const auto& size : bufferSizes)
   {
      bool valid = size % common::CpuCacheGetLineSize() == 0;
      if (!valid)
      {
         throw common::ConfigurationException("BufferAllocator invalid buffer size");
      }
      QueueData& data = m_queues[i];
      data.m_size = size;
      for (std::size_t j = 0; j < nbItemsPerQueue; ++j)
      {
         if (!data.m_queue.Enqueue(AlignedAlloc(data.m_size)))
         {
            throw common::ConfigurationException("BufferAllocator queue is full");
         }
      }
      //
      ++i;
   }
}

template <typename SpscQueue>
BufferAllocator<SpscQueue>::~BufferAllocator()
{
   for (auto& data : m_queues)
   {
      char* buffer;
      while (data.m_queue.Dequeue(buffer))
      {
         free(buffer);
      }
   }
}

template <typename SpscQueue>
char* BufferAllocator<SpscQueue>::AlignedAlloc(std::size_t size)
{
   char* buffer;
   int error = posix_memalign((void**)&buffer, common::CpuCacheGetLineSize(), size);
   if (error)
   {
      assert(false);
      return nullptr;
   }
   return buffer;
}

template <typename SpscQueue>
char* BufferAllocator<SpscQueue>::AllocSlow(std::size_t size)
{
   return static_cast<char*>(malloc(size));
}

}
}


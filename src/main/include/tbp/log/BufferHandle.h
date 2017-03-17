#pragma once

#include <cassert>
#include <cstdlib>

namespace tbp
{
namespace log
{

template <typename SpscQueue> class BufferAllocator;

template <typename SpscQueue>
class BufferHandle
{
public:
   BufferHandle() = default;
   BufferHandle(char* buffer, SpscQueue* queue) : m_buffer(buffer), m_queue(queue) {}
   ~BufferHandle();
   BufferHandle(const BufferHandle& rhs) = delete;
   BufferHandle& operator=(const BufferHandle& rhs) = delete;
   BufferHandle(BufferHandle&& rhs);
   BufferHandle& operator=(BufferHandle&& rhs);
   //
   explicit operator char*() { return m_buffer; }

private:
   friend class BufferAllocator<SpscQueue>;
   void Destroy();
   //
   char* m_buffer = nullptr;
   SpscQueue* m_queue = nullptr;

};

template <typename SpscQueue>
inline void BufferHandle<SpscQueue>::Destroy()
{
   /*
   since BufferAllocator uses a spsc, only the AsyncLogger thread can enqueue the buffer to recycle
   so the call to Enqueue() is not done implicitly in log::Buffer dtor, but is done explicitly in AsyncLogger thread
   the free below is just there as a fallback
   */
   assert(m_buffer == nullptr);
   free(m_buffer);
}

template <typename SpscQueue>
inline BufferHandle<SpscQueue>::~BufferHandle()
{
   Destroy();
}

template <typename SpscQueue>
inline BufferHandle<SpscQueue>::BufferHandle(BufferHandle&& rhs) : m_buffer(rhs.m_buffer), m_queue(rhs.m_queue)
{
   rhs.m_buffer = nullptr;
}

template <typename SpscQueue>
inline BufferHandle<SpscQueue>& BufferHandle<SpscQueue>::operator=(BufferHandle&& rhs)
{
   Destroy();
   //
   m_buffer = rhs.m_buffer;
   m_queue = rhs.m_queue;
   //
   rhs.m_buffer = nullptr;
   return *this;
}

}
}


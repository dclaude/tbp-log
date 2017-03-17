#pragma once

#include <memory>
#include <cstring>

namespace tbp
{
namespace log
{

template <typename Allocator>
class Buffer
{
public:
   Buffer() = default;
   Buffer(typename Allocator::Handle buffer);
   //
   void Write(const void* data, std::size_t size)
   {
      memcpy(m_cursor, data, size);
      m_cursor += size;
   }
   void Read(void* data, std::size_t size)
   {
      memcpy(data, m_cursor, size);
      m_cursor += size;
   }
   const char* Get() const { return m_cursor; }
   void Advance(std::size_t size) { m_cursor += size; }
   void Reset() { m_cursor = static_cast<char*>(m_buffer); }
   void Recycle(Allocator& allocator);

private:
   typename Allocator::Handle m_buffer;
   char* m_cursor = nullptr;

};

template <typename Allocator>
inline Buffer<Allocator>::Buffer(typename Allocator::Handle buffer) : m_buffer(std::move(buffer))
{
   Reset();
}

template <typename Allocator>
inline void Buffer<Allocator>::Recycle(Allocator& allocator)
{
   allocator.Free(m_buffer);
}

}
}


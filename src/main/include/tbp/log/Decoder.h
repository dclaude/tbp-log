#pragma once

#include "tbp/log/Buffer.h"
#include <utility>

namespace tbp
{
namespace log
{

template <typename TypeId, typename Allocator>
class Decoder
{
public:
   using Buf = Buffer<Allocator>;
   Decoder(Buf& buffer);
   //
   bool HasNext() const { return m_current < m_nbFields; }
   std::pair<TypeId, Buf*> Next();

private:
   Buf& m_buffer;
   std::size_t m_nbFields = 0;
   std::size_t m_current = 0;

};

template <typename TypeId, typename Allocator>
inline Decoder<TypeId, Allocator>::Decoder(Buf& buffer) : m_buffer(buffer)
{
   m_buffer.Read(&m_nbFields, sizeof(m_nbFields));
}

template <typename TypeId, typename Allocator>
inline std::pair<TypeId, Buffer<Allocator>*> Decoder<TypeId, Allocator>::Next()
{
   TypeId typeId = TypeId::NONE;
   m_buffer.Read(&typeId, sizeof(typeId));
   ++m_current;
   return std::make_pair(typeId, &m_buffer);
}

}
}


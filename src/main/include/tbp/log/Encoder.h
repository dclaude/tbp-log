#pragma once

#include "tbp/log/Type.h"
#include "tbp/log/Buffer.h"
#include <cstddef>

namespace tbp
{
namespace log
{

template <typename TypeId, typename Allocator>
class Encoder
{
public:
   using Buf = Buffer<Allocator>;
   template<typename... Targs>
   Buf /*TBP_NOINLINE*/ Encode(Allocator& allocator, const Targs&... args)
   {
      std::size_t nbFields = sizeof...(args);
      if (!nbFields)
      {
         return Buf();
      }
      std::size_t size = sizeof(nbFields); // number of fields encoded in the buffer
      size += nbFields * sizeof(TypeId); // TypeId of each field
      size += Sizeof(args...); // not constexpr because of variable length (std::string, ...)
      //
      Buf buffer(std::move(allocator.Alloc(size)));
      buffer.Write(&nbFields, sizeof(nbFields));
      EncodeFields(buffer, args...);
      return buffer;
   }

private:
   template <typename Head, typename... Tail>
   std::size_t Sizeof(const Head& head, const Tail&... tail)
   {
      return Type<Head, TypeId, Allocator>::Sizeof(head) + Sizeof(tail...);
   }
   std::size_t Sizeof() { return 0; }
   template<typename Head, typename... Tail>
   void EncodeFields(Buf& buffer, const Head& head, const Tail&... tail)
   {
      auto typeId = Type<Head, TypeId, Allocator>::Id();
      buffer.Write(&typeId, sizeof(typeId));
      Type<Head, TypeId, Allocator>::Encode(buffer, head);
      EncodeFields(buffer, tail...);
   }
   void EncodeFields(Buf&) {}

};

}
}


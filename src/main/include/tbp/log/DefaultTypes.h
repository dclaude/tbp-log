#pragma once

#include "tbp/log/Type.h"
#include "tbp/log/Buffer.h"
#include <cstdint>
#include <string>
#include <type_traits>

namespace tbp
{
namespace log
{

enum class DefaultTypeId : std::uint8_t
{
   NONE,
   INT,
   UINT64,
   INT64,
   DOUBLE,
   STRING,
};

template <typename T, typename Allocator, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
struct ArythmeticType
{
   static constexpr std::size_t Sizeof(T v) { return sizeof(v); }
   static void Encode(Buffer<Allocator>& buffer, T v)
   {
      buffer.Write(&v, sizeof(v));
   }
   static void Decode(Buffer<Allocator>& buffer, T& v)
   {
      buffer.Read(&v, sizeof(v));
   }
};

template <typename TypeId, typename Allocator>
struct Type<int, TypeId, Allocator> : public ArythmeticType<int, Allocator>
{
   static TypeId Id() { return TypeId::INT; }
};

template <typename TypeId, typename Allocator>
struct Type<std::uint64_t, TypeId, Allocator> : public ArythmeticType<std::uint64_t, Allocator>
{
   static TypeId Id() { return TypeId::UINT64; }
};

template <typename TypeId, typename Allocator>
struct Type<std::int64_t, TypeId, Allocator> : public ArythmeticType<std::int64_t, Allocator>
{
   static TypeId Id() { return TypeId::INT64; }
};

template <typename TypeId, typename Allocator>
struct Type<double, TypeId, Allocator> : public ArythmeticType<double, Allocator>
{
   static TypeId Id() { return TypeId::DOUBLE; }
};

template <typename TypeId, typename Allocator>
struct Type<std::string, TypeId, Allocator>
{
   static TypeId Id() { return TypeId::STRING; }
   static std::size_t Sizeof(const std::string& v) { return sizeof(std::size_t) + v.size(); } // not constexpr
   static void Encode(Buffer<Allocator>& buffer, const std::string& v)
   {
      std::size_t size = v.size();
      buffer.Write(&size, sizeof(size));
      buffer.Write(v.c_str(), size);
   }
   static void Decode(Buffer<Allocator>& buffer, std::string& v)
   {
      std::size_t size = 0;
      buffer.Read(&size, sizeof(size));
      v.assign(buffer.Get(), size);
      buffer.Advance(size);
   }
};

}
}


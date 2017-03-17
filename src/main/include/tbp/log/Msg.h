#pragma once

#include "tbp/log/Buffer.h"
#include "tbp/log/Level.h"
#include "tbp/common/OS.h"
#include <time.h>

namespace tbp
{
namespace log
{

class Category;

template <typename Allocator>
class Msg
{
public:
   using Buf = Buffer<Allocator>;
   //
   Msg() = default;
   Msg(const timespec& time, Level level, const Category& category, const char* fmt, Buf buffer, common::SigNum signal)
      : m_buffer(std::move(buffer)), m_time(time), m_category(&category), m_fmt(fmt), m_signal(signal), m_level(level)
   {
   }
   Msg(Msg&&) = default;
   Msg& operator=(Msg&&) = default;
   //
   Buf& GetBuffer() { return m_buffer; }
   const Buf& GetBuffer() const { return m_buffer; }
   const char* GetFormat() const { return m_fmt; }
   const timespec& GetTime() const { return m_time; }
   Level GetLevel() const { return m_level; }
   const Category& GetCategory() const { return *m_category; }
   void Recycle(Allocator& allocator) { m_buffer.Recycle(allocator); }
   common::SigNum GetSignal() const { return m_signal; }

private:
   Buf m_buffer;
   timespec m_time;
   const Category* m_category = nullptr;
   const char* m_fmt = nullptr;
   common::SigNum m_signal = 0;
   Level m_level = Level::none;

};

}
}


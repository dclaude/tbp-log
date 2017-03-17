#pragma once

#include "tbp/log/Buffer.h"
#include <ostream>

namespace tbp
{
namespace log
{
namespace test
{

class UserDefinedLoggable 
{ 
public: 
   UserDefinedLoggable() = default;
   UserDefinedLoggable(int year, int month, int day) : m_year(year), m_month(month), m_day(day) {} 
   //
   friend std::ostream& operator<<(std::ostream& os, const UserDefinedLoggable& d) 
   { 
      return os << d.m_year << '-' << d.m_month << '-' << d.m_day; 
   } 
   template <typename Allocator> void Encode(log::Buffer<Allocator>& buffer) const
   {
      buffer.Write(this, sizeof(*this));
   }
   template <typename Allocator> void Decode(log::Buffer<Allocator>& buffer)
   {
      buffer.Read(this, sizeof(*this));
   }
   bool Equals(const UserDefinedLoggable& rhs)
   {
      return m_year == rhs.m_year && m_month == rhs.m_month && m_day == rhs.m_day;
   }

private:
   int m_year = 0;
   int m_month = 0;
   int m_day = 0;

}; 

}
}
}


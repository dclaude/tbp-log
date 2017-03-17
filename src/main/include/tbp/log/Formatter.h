#pragma once

#include <cppformat/format.h>
#include <string>

namespace tbp
{
namespace log
{

class Formatter
{
public:
   Formatter()
   {
      m_format.reserve(1024);
   }
   //
   template <typename FUNC>
   void /*TBP_NOINLINE*/ Format(const char* fmt, fmt::MemoryWriter& writer, FUNC func)
   {
      // from cppformat / format.h / template <typename Char> void BasicFormatter<Char>::format(BasicCStringRef<Char> format_str)
      const char* s = fmt;
      const char* start = s;
      while (*s)
      {
         char c = *s++;
         if (c != '{' && c != '}') continue;
         if (*s == c)
         {
            /*
            from https://github.com/cppformat/cppformat/blob/master/doc/syntax.rst
            If you need to include a brace character in the literal text, it can be escaped by doubling: {{ and }}
            */
            Write(writer, start, s);
            start = ++s;
            continue;
         }
         if (c == '}')
         {
            throw "unmatched '}' in format string";
         }
         //
         // at this point we know that c == '{' and that it is not an escaped curly brace
         while (*s++ != '}') {}
         m_format.assign(start, s - start);
         //
         // the switch/case below must be replaced with the call to the 'decode functor' of the previous unit test
         func(m_format.c_str(), writer);
         //
         start = s;
      }
      Write(writer, start, s);
   }

private:
   // from cppformat / format.h / template <typename Char> void write(BasicWriter<Char> &w, const Char *start, const Char *end)
   void Write(fmt::MemoryWriter& w, const char *start, const char *end)
   {
      if (start != end)
      {
         w << fmt::BasicStringRef<char>(start, static_cast<unsigned char>(end - start));
      }
   }    
   //
   std::string m_format;

};

}
}


#pragma once

namespace tbp
{
namespace log
{

inline std::string GetLogMsg(const fmt::MemoryWriter& writer)
{
   std::string msg(writer.data(), writer.size());
   std::size_t timestampEndPos = msg.find(']');
   std::size_t threadIdEndPos = msg.find(']', timestampEndPos + 1);
   return msg.substr(threadIdEndPos + 1);
}

}
}


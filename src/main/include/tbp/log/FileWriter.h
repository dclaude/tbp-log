#pragma once

#include "tbp/log/Category.h"
#include "tbp/log/Level.h"
#include "tbp/common/OS.h"
#include "tbp/common/Definitions.h"
#include <cppformat/format.h>
#include <fstream>
#include <ctime>
#include <time.h>

namespace tbp
{
namespace log
{

namespace
{

const char* ToString(Level val)
{
   switch (val)
   {
      case Level::debug:
         return "debug";
      case Level::info:
         return "info";
      case Level::warn:
         return "warn";
      case Level::error:
         return "error";
      case Level::critical:
         return "critical";
      case Level::none:
         return "none";
   }
   return "";
}

}

class Config;

class FileWriter
{
public:
   FileWriter(const Config& config, common::ThreadId tid);
   FileWriter(FileWriter&&) = default;
   FileWriter& operator=(FileWriter&&) = default;
   MOCK_NPERF_VIRTUAL ~FileWriter();
   //
   void WriteHeader(const timespec& time, common::ThreadId tid, Level level, const Category& category);
   fmt::MemoryWriter& GetWriter() { return m_writer; }
   void WriteToFile();
   void Flush();

MOCK_PROTECTED:
   MOCK_NPERF_VIRTUAL void OnWrite(const fmt::MemoryWriter& /*writer*/) const {}
   MOCK_NPERF_VIRTUAL void OnFileWritten(std::ofstream& /*file*/) const {}

private:
   std::ofstream m_file;
   fmt::MemoryWriter m_writer;

};

inline void FileWriter::WriteHeader(const timespec& time, common::ThreadId tid, Level level, const Category& category)
{
   std::tm curr;
   std::tm* currPtr = localtime_r(&time.tv_sec, &curr);
   m_writer.write("[{:0>2}:{:0>2}:{:0>2}.{:0>9}][{}][{}][{}] ", 
         // each field of the timestamp is right aligned (>) with zero-padding (0>) at the beginning
         currPtr->tm_hour, currPtr->tm_min, currPtr->tm_sec, time.tv_nsec,
         tid, ToString(level), category.GetLabel());
}

inline void FileWriter::WriteToFile()
{
   OnWrite(m_writer);
   m_writer.write("\n");
   m_file.write(m_writer.data(), m_writer.size());
   OnFileWritten(m_file);
   m_writer.clear();
}

inline void FileWriter::Flush()
{
   m_file.flush();
}

}
}


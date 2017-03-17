#include "tbp/log/FileWriter.h"
#include "tbp/log/Config.h"
#include "tbp/common/ConfigurationException.h"
#include <experimental/filesystem>
#include <sstream>
#include <iomanip>

// do not use boost::filesystem to avoid a dependency on boost
namespace fs = std::experimental::filesystem;

namespace tbp
{
namespace log
{

namespace
{

std::string LocalTimeToString(const char* fmt)
{
   std::time_t epoch = std::time(nullptr);
   std::tm* tm = std::localtime(&epoch);
   std::ostringstream oss;
   oss << std::put_time(tm, fmt);
   return oss.str();
}

}

FileWriter::FileWriter(const Config& config, common::ThreadId tid)
{
   std::ostringstream oss;
   oss << config.GetFilePrefix();
   oss << "_" << LocalTimeToString("%Y%m%d-%H%M%S");
   oss << "-" + std::to_string(tid);
   oss << ".log";
   fs::path outDir = config.GetOutputDir();
   if (!fs::exists(outDir))
   {
      fs::create_directories(outDir);
   }
   fs::path logFile = outDir;
   logFile /= oss.str();
   m_file.open(logFile.string());
   if (!m_file.is_open())
   {
      std::ostringstream oss;
      oss << "FileWriter cannot open file[" << logFile << "]";
      throw common::ConfigurationException(oss.str());
   }
}

FileWriter::~FileWriter()
{
   if (m_file.is_open())
   {
      m_file.close();
   }
}

}
}


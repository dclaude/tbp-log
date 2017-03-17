#pragma once

#include <string>

namespace tbp
{
namespace log
{

class Config
{
public:
   Config(std::string outputDir, std::string filePrefix)
      : m_outputDir(std::move(outputDir)), m_filePrefix(std::move(filePrefix))
   {}
   //
   const std::string& GetOutputDir() const { return m_outputDir; }
   const std::string& GetFilePrefix() const { return m_filePrefix; }

private:
   std::string m_outputDir;
   std::string m_filePrefix;

};

}
}


#include "tbp/log/Injector.h"
#include "tbp/log/FileWriter.h"

namespace tbp
{
namespace log
{

Injector::~Injector()
{
}

std::unique_ptr<FileWriter> Injector::CreateFileWriter(const log::Config& config, common::ThreadId tid) const
{
   return std::make_unique<FileWriter>(config, tid);
}

}
}

